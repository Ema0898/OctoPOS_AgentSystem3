#include "os/agent3/Agent.h"
#include "os/agent3/AgentRPCClient.h"
#include "os/agent3/AbstractConstraint.h"
#include "os/agent3/AgentConstraint.h"
#include "os/agent3/ActorConstraint.h"
#include "os/agent3/ActorConstraintSolver.h"

extern "C"
{
	uintptr_t os_agent_agentsystem_no_init __attribute__((weak)) = 0;
}

uint8_t os::agent::AgentInstance::get_slot_no_of_claim_no(uint32_t claim_no) const
{
	kassert(claim_no != os::agent::AgentClaim::INVALID_CLAIM);

	uint8_t ret = 0, pseudo_ret = 0;
	bool exists = false;
	/* Find slot corresponding to claim_no. */
	for (ret = 0; ret < MAX_REQUESTS_PER_AGENT; ++ret)
	{
		if (myRequests[ret].active && myRequests[ret].claim.getUcid() == claim_no)
		{
			break;
		}

		if (myRequests[ret].claim.getUcid() == claim_no)
		{
			exists = true;
			pseudo_ret = ret;
		}
	}

	if (MAX_REQUESTS_PER_AGENT == ret)
	{
		DBG(SUB_AGENT, "get_slot_no_of_claim_no: no active slot found for claim number %" PRIu32 ". Maybe the claim has already been retreated?\n", claim_no);
		if (exists)
		{
			DBG(SUB_AGENT, "get_slot_no_of_claim_no: but it exists in slot %" PRIu8 " in inactive state with stage %" PRIuMAX "\n", pseudo_ret, static_cast<uintmax_t>(myRequests[pseudo_ret].stage));
		}
	}

	return (ret);
}

/* For documentation of this function, check the header file. */
uint8_t os::agent::AgentInstance::reinvade_retreat(uint32_t claim_no, uint8_t newSlot)
{
	uint8_t slot = get_slot_no_of_claim_no(claim_no);

	if (MAX_REQUESTS_PER_AGENT == slot)
	{
		panic("reinvade_retreat: no active slot found for specified claim number. Probably already retreated and now invalid.");
	}

	DBG(SUB_AGENT, "Reinvade Retreat Agent %p Claim %" PRIu32 ", Slot %" PRIu8 "\n", this, claim_no, slot);

	/*
	 * This is a reinvade retreat.
	 * Thus we know that an invade call will follow right away.
	 */

	//int vipg_nCores_block = 1;
	os::agent::AgentSystem::lockBargaining(&myRequests[slot].claim);

	/*
	 * If we are sticky
	 * then keep one PE per tile in the claim for the invade
	 *      and put the rest into the pool for grabs.
	 * Otherwise put everything into the pool,
	 *      except we are running within the claim.
	 */
	bool sticky = myRequests[slot].agent_constraints->isSticky();

	os::agent::AgentClaim oldResources = new AgentClaim();

	/* Transfer all resources that are NOT reserved to my resource pool. */
	os::agent::ResourceID res;
	int prev_tileID = -1;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!myRequests[slot].claim.containsTile(res.tileID))
		{
			continue;
		}
		//We need to handle both the cases of a StickyConstraint, and a tile being sticky on a per-agent-basis
		bool stickyTile = sticky || sticky_tiles.get(res.tileID);
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (!myRequests[slot].claim.contains(res))
			{
				continue;
			}

			if (myResources[res.tileID][res.resourceID].ReservedForAgent != NULL &&
					myResources[res.tileID][res.resourceID].ReservedForAgent != this)
			{
				continue;
			}

			/* We hold an unreserved resource. */
			myResources[res.tileID][res.resourceID].ReservedForAgent = this;
			myResources[res.tileID][res.resourceID].ReservationRating = 999999;

			/*
	     * If the StickyConstraint is set or the tile is sticky in the agent, keep one resource per tile
	     */
			if (stickyTile && prev_tileID < res.tileID)
			{
				/*
				 * Do not put this into the resource pool,
				 * so the following invade is guaranteed to take it from here.
				 */
				DBG(SUB_AGENT, "reinvade_retreat: Keep resource (%d/%d) in agent %p slot %d due to stickiness.\n", res.tileID, res.resourceID, this, slot);
				prev_tileID = res.tileID;
				continue;
			}

			myRequests[slot].claim.remove(res);
			myResourcePool.add(res);
			oldResources.add(res);
			DBG(SUB_AGENT, "reinvade_retreat: Put resource (%d/%d) into pool of agent %p.\n", res.tileID, res.resourceID, this);
		}
	}
	kassert(!sticky || check_sticky_invariant(slot));

	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!myRequests[slot].claim.containsTile(res.tileID))
		{
			continue;
		}
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (!myRequests[slot].claim.contains(res))
			{
				continue;
			}

			if (myResources[res.tileID][res.resourceID].ReservedForAgent != this)
			{
				DBG(SUB_AGENT, "reinvade_retreat: Not transfering resource (%d/%d) from slot %d to slot %d because not reserved for me (%p) but for %p\n", res.tileID, res.resourceID, slot, newSlot, this, myResources[res.tileID][res.resourceID].ReservedForAgent);
				continue;
			}

			/*
			 * This resource must be transfered from the retreated
			 * to this reinvaded claim.
			 */
			DBG(SUB_AGENT, "reinvade_retreat: Transfering resource (%d/%d) from slot %d to slot %d\n", res.tileID, res.resourceID, slot, newSlot);
			myRequests[newSlot].claim.add(res);
			myResources[res.tileID][res.resourceID].ReservedForAgent = NULL;
			myResources[res.tileID][res.resourceID].ReservationRating = 0;
		}
	}

	/* check that we have all sticky tiles covered */
	kassert(check_sticky_invariant(newSlot));

	// Save complete old claim in old slot to restore it in case the reinvade fails.
	// Set slot inactive so that resources aren't blocked for the invade.
	myRequests[slot].claim.addClaim(oldResources);
	myRequests[slot].claim.print();
	myRequests[slot].active = false;

	/* Ensure nobody gives us more resources into this slot from now on. */
	myRequests[slot].pending = false;
	myRequests[slot].improvable = false;
	myRequests[slot].stage = reinvading;

	return slot;
}

/* For documentation of this function, check the header file. */
bool os::agent::AgentInstance::pure_retreat(uint32_t claim_no)
{
	uint8_t slot = get_slot_no_of_claim_no(claim_no);

	if (MAX_REQUESTS_PER_AGENT == slot)
	{
		panic("pure_retreat: no active slot found for specified claim number. Probably already retreated and now invalid.");
	}

	DBG(SUB_AGENT, "Pure Retreat Agent %p Claim %" PRIu32 ", Slot %" PRIu8 "\n", this, claim_no, slot);

	/* Actual retreating work. */
	os::agent::AgentSystem::unlockBargaining(&myRequests[slot].claim);

	/* Move resources that are reserved for other agents to them. */
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!myRequests[slot].claim.containsTile(res.tileID))
		{
			continue;
		}
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			// Also give away blocked resources if the claim contains resources of the SAME tile.
			if (!myRequests[slot].claim.contains(res) && !myRequests[slot].blockedResources.contains(res))
			{
				continue;
			}

			if (myResources[res.tileID][res.resourceID].ReservedForAgent == NULL)
			{
				if (myRequests[slot].claim.contains(res))
				{
					myRequests[slot].claim.remove(res);
				}
				else
				{
					myRequests[slot].blockedResources.remove(res);
				}
				myResourcePool.add(res);
				this->transferResource(res, os::agent::AgentSystem::idleAgent);
				continue;
			}

			if (myResources[res.tileID][res.resourceID].ReservedForAgent == this)
			{
				continue;
			}

			if (myResources[res.tileID][res.resourceID].ReservedForAgent->getRating(res) > this->getRating(res))
			{
				DBG(SUB_AGENT, "Losing resource (%d/%d) in pure_retreat because of reservation from %p\n", res.tileID, res.resourceID, myResources[res.tileID][res.resourceID].ReservedForAgent);
				myRequests[slot].claim.remove(res);
				os::agent::AgentSystem::unsetOwner(res, this);
				DBG(SUB_AGENT, "pure_retreat: move resource\n");
				myResources[res.tileID][res.resourceID].ReservedForAgent->gainReservedResource(res);
				DBG(SUB_AGENT, "pure_retreat: unsetting ReservedForAgent for resource\n");
				myResources[res.tileID][res.resourceID].ReservedForAgent = NULL;
			}
			else
			{
				DBG(SUB_AGENT, "Reserved resource no longer useful for requesting agent...\n");
				myResources[res.tileID][res.resourceID].ReservedForAgent->giveUpReservation(res);
				myResources[res.tileID][res.resourceID].ReservedForAgent = NULL;
			}
		}
	}

	/* This operation looks innocent, but is actually VERY important. Do not remove it.
	 * Moving resources from one claim to another, that is NOT part of an active slot,
	 * implicitly obsoletes any entries in the agent's private reservations list.
	 * This seems to be an integral part of keeping the system state inconsistent.
	 * FIXME: I doubt it should be actually necessary to force consistence by moving
	 *        resources, so something is probably broken.
	 */
	if (!myRequests[slot].claim.isEmpty())
	{
		DBG(SUB_AGENT, "pure_retreat: moving remaining resources to pool\n");
		myRequests[slot].claim.moveAllResourcesToOtherClaim(myResourcePool);
		myRequests[slot].blockedResources.moveAllResourcesToOtherClaim(myResourcePool);
	}

	myRequests[slot].claim.reset();
	myRequests[slot].blockedResources.reset();
	myRequests[slot].dispatch_claim = os::res::DispatchClaim(255);
	myRequests[slot].stage = idle;
	myRequests[slot].pending = false;
	delete myRequests[slot].solver;
	myRequests[slot].solver = NULL;
	myRequests[slot].agent_constraints = NULL;
	myRequests[slot].actor_constraint = NULL;
	myRequests[slot].active = false;

	return (checkAlive());
}

/* For documentation of this function, check the header file. */
void os::agent::AgentInstance::fullRetreat(void)
{
	/* Clear all request slots. */
	for (uint8_t slot = 0; slot < MAX_REQUESTS_PER_AGENT; ++slot)
	{
		if (!myRequests[slot].active)
		{
			continue;
		}

		/*
		 * I am aware that pure_retreat() will map the claim number
		 * back to a slot by iterating over all slots, which essentially
		 * is unnecessary overhead.
		 * The alternative is an ugly interface for accepting both
		 * claim numbers and slots, though.
		 * Let's not be ugly.
		 */
		pure_retreat(myRequests[slot].claim.getUcid());
	}
}

os::agent::AgentClaim os::agent::AgentInstance::getClaim(int request_no)
{
	DBG(SUB_AGENT, "GetClaim %d from Agent %p\n", request_no, this);

	if (!myRequests[request_no].active)
	{
		panic("getClaim: Requesting AgentClaim from invalid AgentRequest!");
	}

	return myRequests[request_no].claim;
}

os::agent::AgentClaim os::agent::AgentInstance::invade(os::agent::AgentConstraint *constraints)
{
	panic("AgentInstance::invade SHOULD NO LONGER BE USED!");
	os::agent::AgentClaim dummy;
	return dummy;
}

/* @Returns   whether slot-claim contains at least one resource for each sticky tiles */
bool os::agent::AgentInstance::check_sticky_invariant(uint8_t slot) const
{
	ResourceID res;
	const AgentClaim *claim = &myRequests[slot].claim;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
	{
		if (!sticky_tiles.get(res.tileID))
			continue;
		bool found = false;
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
		{
			if (claim->contains(res))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			DBG(SUB_AGENT, "Tile %d is sticky for agent %p, but we have no res in slot %d\n", res.tileID, this, slot);
			return false;
		}
		DBG(SUB_AGENT, "Tile %d is sticky and is covered by %d\n", res.tileID, res.resourceID);
	}
	return true;
}

// MultiStage Invade
uint8_t os::agent::AgentInstance::invade_regioncheck(os::agent::AbstractConstraint *constraints)
{
	uint8_t slot = 0;
	for (slot = 0; slot < MAX_REQUESTS_PER_AGENT; ++slot)
	{
		if (!myRequests[slot].active)
		{
			break;
		}
	}

	if (MAX_REQUESTS_PER_AGENT == slot)
	{
		// no more slots available.
		DBG(SUB_AGENT, "Agent %p NO MORE SLOTS %d\n", this, slot);
		panic("invade_regioncheck: Out of Slots!");
		// TODO: Unlock Request-Thingy
		return slot;
	}

	DBG(SUB_AGENT, "Agent %p Invading in Slot %d\n", this, slot);

	myRequests[slot].claim.reset();
	myRequests[slot].claim.setOwningAgent(this);
	myRequests[slot].dispatch_claim = os::res::DispatchClaim(255);
	if (constraints->isAgentConstraint())
	{
		myRequests[slot].agent_constraints = static_cast<os::agent::AgentConstraint *>(constraints);
		myRequests[slot].actor_constraint = NULL;
	}
	else
	{
		myRequests[slot].agent_constraints = NULL;
		myRequests[slot].actor_constraint = static_cast<os::agent::ActorConstraint *>(constraints);
	}

	myRequests[slot].blockedResources.reset();
	myRequests[slot].blockedResources.setOwningAgent(this);
	myRequests[slot].pending = true;
	myRequests[slot].improvable = false;
	myRequests[slot].active = true;
	myRequests[slot].stage = idle;

	// In this small world, we use the whole chip as a region..
	myRequests[slot].stage = fetchmonitors;

	return slot;
}

uint8_t os::agent::AgentInstance::invade_fetchmonitors(uint8_t slot, uint8_t oldSlot)
{
	if (!myRequests[slot].active)
	{
		panic("invade_fetchmonitors: Nullpointer in internal Agent Structures!");
		return 0;
	}

	if (fetchmonitors != myRequests[slot].stage)
	{
		panic("invade_fetchmonitors: Agent Data-Structures inconsistent");
		return 0;
	}

	// Fetching monitors for target region
	AgentSystem::enterAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());
	DBG(SUB_AGENT, "Agent %p Fetch Monitors in Slot %d\n", this, slot);
	myRequests[slot].stage = bargain;
	AgentSystem::leaveAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());

	// now all we have to do is to enqueue the next step..
	// OR. we could just call it ;-)
	return invade_bargain(slot, oldSlot);
}

/* Returns true if the resource should not be taken from this agent instance,
 * because it is a sticky claim's last PE for a tile.
 */
bool os::agent::AgentInstance::stayCauseSticky(const os::agent::ResourceID &res, const os::agent::AgentClaim *pseudo_claim) const
{
	for (std::size_t slot = 0; slot < os::agent::MAX_REQUESTS_PER_AGENT; ++slot)
	{
		if (!myRequests[slot].active)
		{
			DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR " inactive, skipping...\n", slot);
			continue;
		}
		else
		{
			DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR " active.\n", slot);
		}

		if (!myRequests[slot].claim.contains(res))
		{
			DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR " does not claim resource (%d/%d), skipping...\n", slot, res.tileID, res.resourceID);
			continue;
		}

		if (myRequests[slot].agent_constraints)
		{
			if (myRequests[slot].agent_constraints->isSticky())
			{
				DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR " has non-sticky constraint. Returning free.\n", slot);
				return (false); /* claim not sticky */
			}
		}
		os::agent::ResourceID other;
		other.tileID = res.tileID;
		for (other.resourceID = 0; other.resourceID < os::agent::MAX_RES_PER_TILE; ++other.resourceID)
		{
			if (res.resourceID == other.resourceID)
			{
				DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR ": skipping same resource (%d/%d)\n", slot, res.tileID, res.resourceID);
				continue;
			}

			const AgentInstance *reserved_agent = myResources[other.tileID][other.resourceID].ReservedForAgent;
			if ((reserved_agent != this) && (reserved_agent != NULL))
			{
				DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR ": skipping resource (%d/%d) because reserved for agent %p -- we're %p\n", slot, other.tileID, other.resourceID, reserved_agent, this);
				continue;
			}

			if (myRequests[slot].claim.contains(other, true))
			{
				/*
				 * We found a second PE in the same tile, thus we could give the first one away.
				 * If it isn't contained in the pseudo_claim we might have been passed, that is.
				 * This is specific to a "dry-run" invade possibility check.
				 */
				if (pseudo_claim && (pseudo_claim->contains(other, true)))
				{
					DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR ": resource (%d/%d) contained in pseudo claim %p, skipping.\n", slot, other.tileID, other.resourceID, pseudo_claim);
					continue;
				}

				DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR ": resource (%d/%d) contained in claim %p, returning free.\n", slot, other.tileID, other.resourceID, &myRequests[slot].claim);
				return (false);
			}
		}

		/* could not find a second PE in the same tile */
		// TODO should we check the other slots?
		DBG(SUB_AGENT_NOISY, "stayCauseSticky: slot %" PRIuPTR ": no other PE in tile available, returning sticky.\n", slot);
		return (true);
	}

	/*
	 * Apparently my application does not use res.
	 * Might still be in the pool or reserved,
	 * but we do not care about those.
	 */
	DBG(SUB_AGENT_NOISY, "stayCauseSticky: resource (%d/%d) available/non-sticky.\n", res.tileID, res.resourceID);
	return (false);
}

os::agent::AgentMalleabilityClaim *os::agent::AgentInstance::buildMalleabilityClaim(int slot)
{
	os::agent::MalleabilityConstraint *mc = static_cast<os::agent::MalleabilityConstraint *>(
			myRequests[slot].agent_constraints->searchConstraint(ConstrType::MALLEABILITY));
	if (!mc || !mc->isMalleable())
	{
		DBG(SUB_AGENT, "not malleable!\n");
		return NULL;
	}

	os::agent::PEQuantityConstraint *peqc = static_cast<os::agent::PEQuantityConstraint *>(
			myRequests[slot].agent_constraints->searchConstraint(ConstrType::PEQUANTITY));
	uint8_t numAbdicableResources[HWTypes];
	for (int i = 0; i < HWTypes; ++i)
	{
		if (peqc)
		{
			numAbdicableResources[i] = myRequests[slot].claim.getResourceCount(NULL, NULL, (ResType)i) - peqc->getMin((ResType)i);
		}
		else
		{
			numAbdicableResources[i] = 0;
		}
	}
	os::agent::AgentMalleabilityClaim *mClaim = NULL;
	myRequests[slot].claim.createMalleabilityClaim(&mClaim, slot, numAbdicableResources);

	/*
	 * here, we could technically use the strip()-function on the AgentMalleabilityClaim to rule out
	 * some resources based on our Constraints.
	 */

	return mClaim;
}

uint8_t os::agent::AgentInstance::getMalleabilityClaims(os::agent::AgentMalleabilityClaim **buffer,
																												os::agent::AgentMalleabilityClaim **end, os::agent::AgentInstance *callingAgent, int slot)
{
	uint8_t numClaims = 0;

	for (int i = 0; i < MAX_REQUESTS_PER_AGENT; ++i)
	{
		if (!myRequests[i].active)
		{
			continue;
		}
		if (callingAgent == this && i == slot)
		{
			DBG(SUB_AGENT, "that would be a call to ourselves\n");
			continue;
		}

		DBG(SUB_AGENT, "call buildMalleabilityClaim and store into index %d\n", numClaims);
		buffer[numClaims] = buildMalleabilityClaim(i);
		if (buffer[numClaims])
		{
			numClaims++;
		}

		// don't reach behind the buffer
		if (&buffer[numClaims] == end)
		{
			break;
		}
	}

	return numClaims;
}

std::pair<bool, os::agent::AgentClaim> os::agent::AgentInstance::invade_fulfillable(const uint8_t slot) const
{
	typedef os::agent::AgentSystem AS;

	kassert(myRequests[slot].active);
	kassert(bargain == myRequests[slot].stage);

	os::agent::AgentClaim pseudo_claim;
	bool ret = false;

	os::agent::ResourceID res;

	// AGENT CONSTRAINT
	if (myRequests[slot].agent_constraints)
	{
		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			if (!(is_tile_usable(slot, res.tileID)))
			{
				DBG(SUB_AGENT, "tile %d not usable\n", res.tileID);
				continue;
			}

			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				os::agent::PEQuantityConstraint *pec = static_cast<os::agent::PEQuantityConstraint *>(
						myRequests[slot].agent_constraints->searchConstraint(ConstrType::PEQUANTITY));
				if (pec && !pec->isResourceUsable(res))
				{
					continue;
				}

				const AgentInstance *reserved_agent = myResources[res.tileID][res.resourceID].ReservedForAgent;
				if ((reserved_agent != NULL) && (reserved_agent != this))
				{
					DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) reserved for agent %p\n", res.tileID, res.resourceID, reserved_agent);
					continue;
				}

				const AgentInstance *res_owner = AS::getOwner(res);
				if (res_owner)
				{
					if (res_owner != this)
					{
						DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) currently owned by agent %p\n", res.tileID, res.resourceID, res_owner);

						if (res_owner->stayCauseSticky(res, &pseudo_claim))
						{
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) unavailable - sticky to owning agent %p\n", res.tileID, res.resourceID, res_owner);
							continue;
						}
						else
						{
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) non-sticky to owning agent %p\n", res.tileID, res.resourceID, res_owner);

							if (res_owner->is_part_of_active_request(res))
							{
								DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) part of active slot - skipping.\n", res.tileID, res.resourceID);
								continue;
							}
						}
					}
					else
					{
						/* Could be either usable if or not. Depending upon which slot "owns" the resource. */
						const AgentClaim *res_claim = getClaim(res);

						if (!res_claim)
						{
							/* This can happen due to resource pooling. It is not part of an active slot, but owned by us. */
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) owned by us (%p) and part of no other claim (pooled?)\n", res.tileID, res.resourceID, this);
						}
						else if (res_claim != &myRequests[slot].claim)
						{
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) owned by us (%p), but used by other claim\n", res.tileID, res.resourceID, this);
							continue;
						}

						/*
						 * Otherwise the resource is part of the current slot and usable.
						 * This should only be true in the reinvade case.
						 */
					}
				}
				else
				{
					DBG(SUB_AGENT, "WARNING: invade_fulfillable: resource (%d/%d) has no owner. Agent system state potentially broken. Ignoring and further examining resource.\n", res.tileID, res.resourceID);
				}

				/* Add to pseudo claim. */
				pseudo_claim.add(res);
			}
		}

		DBG(SUB_AGENT, "Printing pseudo_claim\n");
		pseudo_claim.print();
		ret = myRequests[slot].agent_constraints->fulfilledBy(pseudo_claim, NULL, false);

		return (std::make_pair(ret, pseudo_claim));
	}

	// ACTOR CONSTRAINT
	else if (myRequests[slot].actor_constraint)
	{
		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			// MODIFIED FOR ACTOR CONSTRAINT
			if (!(myRequests[slot].actor_constraint->is_tile_usable(res.tileID)))
			{
				continue;
			}

			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				// MODIFIED FOR ACTOR CONSTRAINT
				if (!(myRequests[slot].actor_constraint->is_resource_usable(res)))
				{
					continue;
				}

				const AgentInstance *reserved_agent = myResources[res.tileID][res.resourceID].ReservedForAgent;
				if ((reserved_agent != NULL) && (reserved_agent != this))
				{
					DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) reserved for agent %p\n", res.tileID, res.resourceID, reserved_agent);
					continue;
				}
				const AgentInstance *res_owner = AS::getOwner(res);
				if (res_owner)
				{
					if (res_owner != this)
					{
						DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) currently owned by agent %p\n", res.tileID, res.resourceID, res_owner);

						if (res_owner->stayCauseSticky(res, &pseudo_claim))
						{
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) unavailable - sticky to owning agent %p\n", res.tileID, res.resourceID, res_owner);
							continue;
						}
						else
						{
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) non-sticky to owning agent %p\n", res.tileID, res.resourceID, res_owner);

							if (res_owner->is_part_of_active_request(res))
							{
								DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) part of active slot - skipping.\n", res.tileID, res.resourceID);
								continue;
							}
						}
					}
					else
					{
						/* Could be either usable if or not. Depending upon which slot "owns" the resource. */
						const AgentClaim *res_claim = getClaim(res);

						if (!res_claim)
						{
							/* This can happen due to resource pooling. It is not part of an active slot, but owned by us. */
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) owned by us (%p) and part of no other claim (pooled?)\n", res.tileID, res.resourceID, this);
						}
						else if (res_claim != &myRequests[slot].claim)
						{
							DBG(SUB_AGENT_NOISY, "invade_fulfillable: resource (%d/%d) owned by us (%p), but used by other claim\n", res.tileID, res.resourceID, this);
							continue;
						}

						/*
						 * Otherwise the resource is part of the current slot and usable.
						 * This should only be true in the reinvade case.
						 */
					}
				}
				else
				{
					DBG(SUB_AGENT, "WARNING: invade_fulfillable: resource (%d/%d) has no owner. Agent system state potentially broken. Ignoring and further examining resource.\n", res.tileID, res.resourceID);
				}

				/* Add to pseudo claim. */
				pseudo_claim.add(res);
			}
		}

		//TODO
		// return from type bool to uint8_t
		// MODIFIED FOR ACTOR CONSTRAINT
		std::pair<bool, AgentClaim> result = os::agent::AgentSystem::solver->solve((*(myRequests[slot].actor_constraint)), pseudo_claim);
		return result;
	}

	else
	{
		panic("invade_fulfillable: Constraint is neither agent nor actor constraint");
		return (std::make_pair(ret, pseudo_claim));
	}
}

uint8_t os::agent::AgentInstance::invade_bargain(uint8_t slot, uint8_t oldSlot)
{

	// AGENT CONSTRAINT
	if (myRequests[slot].agent_constraints)
	{
		DBG(SUB_AGENT, "invade_bargain: agent constraint\n");
		kassert(myRequests[slot].agent_constraints);
		return (invade_bargain_dcop(slot, oldSlot));
	}

	// ACTOR CONSTRAINT
	else
	{
		DBG(SUB_AGENT, "invade_bargain: actor constraint\n");
		kassert(myRequests[slot].actor_constraint);
		return (invade_bargain_new(slot));
	}
}

// This invade method is newer than invade_bargain_old & invade_bargain_new. These are probably obsolete for normal constraints (AgentConstraints)
uint8_t os::agent::AgentInstance::invade_bargain_dcop(uint8_t slot, uint8_t oldSlot)
{
	namespace A = os::agent;
	typedef A::AgentSystem AS;

	kassert(myRequests[slot].active);
	kassert(bargain == myRequests[slot].stage);
	kassert(myRequests[slot].agent_constraints);

	AS::enterAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());
	DBG(SUB_AGENT, "Agent %p Bargain in Slot %d\n", this, slot);

	AgentClaim pool;
	pool.reset();
	pool.setOwningAgent(this);
	fillPoolWithMyFreeResources(pool);

	DBG(SUB_AGENT, "Creating ConstraintSolver\n");
	myRequests[slot].solver = new A::ConstraintSolver(this, slot, myRequests[slot].agent_constraints, pool,
																										&myRequests[slot].claim);
	DBG(SUB_AGENT, "Agent %p created ConstraintSolver. Now calling solve on %p\n", this, myRequests[slot].solver);

	bool found = myRequests[slot].solver->solve();

	if (found)
	{
		DBG(SUB_AGENT, "ConstraintSolver says he found a claim:\n");
		myRequests[slot].claim.print();
		DBG(SUB_AGENT, "Claiming ownership of resources\n");
		if (!claimResources(slot))
		{
			/*
			 * TODO better than to crash would be to release all resources we got
			 * and start resolving the Constraints again. This situation may sometimes
			 * happen when a concurrent invade got our resources first
			 */
			panic("Couldn't claim resources that the Solver promised me");
		}
		myResourcePool.removeClaim(myRequests[slot].claim);

		myRequests[slot].stage = octoinvade;
		AS::lockBargaining(&myRequests[slot].claim);

		if (oldSlot != INVALID_REQUEST_SLOT)
		{
			myRequests[oldSlot].claim.reset(false);
		}

		// If this invade is part of a reinvade, restore old claim.
	}
	else if (oldSlot != INVALID_REQUEST_SLOT)
	{
		myRequests[slot].claim.reset(false);
		myRequests[slot].claim.setOwningAgent(this);
		myRequests[slot].claim.addClaim(myRequests[oldSlot].claim);
		myRequests[oldSlot].claim.reset(false);
		myRequests[slot].agent_constraints = myRequests[oldSlot].agent_constraints;

		myResourcePool.removeClaim(myRequests[slot].claim);

		os::agent::AndConstraintList *aList = new os::agent::AndConstraintList();
		os::agent::PEQuantityConstraint *peq = new os::agent::PEQuantityConstraint(aList);
		aList->addConstraint(peq);
		myRequests[oldSlot].agent_constraints = aList;

		myRequests[slot].stage = octoinvade;
		AS::lockBargaining(&myRequests[slot].claim);
	}
	else
	{
		DBG(SUB_AGENT, "ConstraintSolver didn't found anything\n");

		myRequests[slot].stage = idle;
		myRequests[slot].active = false;
		myRequests[slot].pending = false;
		myRequests[slot].claim.reset();
		myRequests[slot].dispatch_claim = os::res::DispatchClaim(255);
	}

	// set old slot active again, so that it can be fully retreated after this invade.
	if (oldSlot != INVALID_REQUEST_SLOT)
	{
		myRequests[oldSlot].active = true;
	}

	AS::leaveAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());

	if (AS::AGENT_TILE == myRequests[slot].RPCAnswerDesc.getParent().getTID())
	{
		myRequests[slot].RPCAnswerDesc.signalLocally(myRequests[slot].claim);
	}
	else
	{
		myRequests[slot].RPCAnswerDesc.sendReply(myRequests[slot].claim);
	}

	return found;
}

bool os::agent::AgentInstance::callResizeHandlers(int slot)
{
	AgentRequest_s *request = &myRequests[slot];
	AgentMalleabilityClaim **claims = NULL;

	const size_t tile_count = hw::hal::Tile::getTileCount();
	const size_t res_per_tile = os::agent::MAX_RES_PER_TILE;

	uint8_t numClaims = request->solver->createMalleabilityClaims(&claims);
	for (uint8_t i = 0; i < numClaims; ++i)
	{
		AgentInstance *partner = claims[i]->getAgent();
		int partnersSlot = claims[i]->getSlot();
		AgentRequest_s *partnerRequest = &partner->myRequests[partnersSlot];
		DBG(SUB_AGENT, "Taking resources from agent %p and its slot %d\n", partner, partnersSlot);

		//fetching the MalleabilityConstraint of the other side
		os::agent::AgentConstraint *c = partnerRequest->agent_constraints;
		os::agent::MalleabilityConstraint *mc = static_cast<os::agent::MalleabilityConstraint *>(
				c->searchConstraint(ConstrType::MALLEABILITY));

		if (!mc->isMalleable())
		{
			panic("callResizeHandlers: other side is not malleable (what are you doin'?)");
		}

		AgentClaim lossClaim = claims[i]->buildLossClaim();
		os::agent::Agent::run_resize_handler(partnerRequest->dispatch_claim, mc->getResizeEnvPointer(),
																				 lossClaim, mc->getResizeHandler(), tile_count, res_per_tile);
		partner->yieldResources(slot, lossClaim);

		// finally, make the agent yield the resources
		AgentClaim remainClaim = claims[i]->buildRemainClaim();

		os::agent::Agent::update_claim_structures(partnerRequest->dispatch_claim,
																							partnerRequest->claim.getAgentOctoClaim(), remainClaim);
	}

	if (numClaims > 0)
	{
		request->solver->cleanup();
	}

	return true;
}

bool os::agent::AgentInstance::claimResources(int slot)
{
	//1. malleable resources
	if (!callResizeHandlers(slot))
	{
		return false;
	}

	//2. claim the resources
	return myRequests[slot].agent_constraints->claimResources(myRequests[slot].claim, this, slot);
}

uint8_t os::agent::AgentInstance::invade_bargain_new(uint8_t slot)
{
	typedef os::agent::AgentSystem AS;
	typedef std::pair<bool, AgentClaim> fulfillable_t;

	kassert(myRequests[slot].active);
	kassert(bargain == myRequests[slot].stage);

	AS::enterAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());
	DBG(SUB_AGENT, "Agent %p Bargain in Slot %d\n", this, slot);

	/* First, try to use resources from our own resource pool to avoid costly bargaining. */
	os::agent::AgentClaim from_resource_pool = fetch_resources_from_resource_pool(slot);
	DBG(SUB_AGENT, "Moved resources from myResourcePool into myRequests[slot].claim\n");

	fulfillable_t fulfillable = invade_fulfillable(slot);

	bool satisfied = fulfillable.first;
	bool need_malleability = false;
	AgentClaim malleable_candidates;
	// AGENT CONSTRAINT
	if (myRequests[slot].agent_constraints)
	{
		if (!satisfied)
		{
			DBG(SUB_AGENT, "invade_bargain: current invade not satisfiable with spare system resources - invoking malleability functionality.\n");
			DBG(SUB_AGENT, "invade_bargain: fetching all malleable resources.\n");

			malleable_candidates = invade_bargain_get_malleable_candidates(slot);

			/*
			 * If all candidates cannot satisfy the current constraints, it doesn't make sense to
			 * disturb malleable applications by taking resources away unnecessarily.
			 */
			AgentClaim pseudo_claim = fulfillable.second;
			pseudo_claim.addClaim(malleable_candidates);

			if (myRequests[slot].agent_constraints->fulfilledBy(pseudo_claim, NULL, false))
			{
				DBG(SUB_AGENT, "invade_bargain: current constraints satisfiable with malleable candidates.\n");
				satisfied = true;
				need_malleability = true;
			}
			else
			{
				DBG(SUB_AGENT, "invade_bargain: current constraints NOT satisfiable with malleable candidates.\n");
			}
		}

		if (satisfied)
		{
			satisfied = move_resources_to_request(slot, fulfillable.second, &from_resource_pool);
			DBG(SUB_AGENT, "Agent Constraint: invade_bargain_new: current claim after moving non-malleable resources:\n");
			myRequests[slot].claim.print();

			if (need_malleability)
			{
				satisfied = move_resources_to_request(slot, malleable_candidates, NULL, true);
			}
		}

		if (satisfied)
		{
			myRequests[slot].claim.print();
			DBG(SUB_AGENT, "Returning claim. Slot %d, agent: %p\n", slot, this);
			myRequests[slot].stage = octoinvade;
			AS::lockBargaining(&myRequests[slot].claim);
			check_sticky_invariant(slot);
		}
		else
		{
			DBG(SUB_AGENT, "invade_bargain: resource acquisition unsuccessful: constraints not fulfilled. Moving resources back to Agent's (%p) resource pool.\n", this);
			myRequests[slot].stage = idle;
			myRequests[slot].active = false;
			myRequests[slot].claim.moveAllResourcesToOtherClaim(this->myResourcePool);
			myRequests[slot].claim.reset();
			myRequests[slot].dispatch_claim = os::res::DispatchClaim(255);
		}
	}

	// ACTOR CONSTRAINT
	else if (myRequests[slot].actor_constraint)
	{
		DBG(SUB_AGENT, "ACTOR CONSTRAINT\n");

		if (!satisfied)
		{
			DBG(SUB_AGENT, "invade_bargain: current invade not satisfiable with spare system resources - invoking malleability functionality.\n");
			DBG(SUB_AGENT, "invade_bargain: fetching all malleable resources.\n");

			malleable_candidates = invade_bargain_get_malleable_candidates(slot);

			/*
			* If all candidates cannot satisfy the current constraints, it doesn't make sense to
			* disturb malleable applications by taking resources away unnecessarily.
			*/
			AgentClaim pseudo_claim = fulfillable.second;
			pseudo_claim.addClaim(malleable_candidates);

			if ((os::agent::AgentSystem::solver->solve((*(myRequests[slot].actor_constraint)), pseudo_claim)).first)
			{
				DBG(SUB_AGENT, "invade_bargain: current constraints satisfiable with malleable candidates.\n");
				satisfied = true;
				need_malleability = true;
			}
			else
			{
				DBG(SUB_AGENT, "invade_bargain: current constraints NOT satisfiable with malleable candidates.\n");
			}
		}

		if (satisfied)
		{
			DBG(SUB_AGENT, "Before move_resources_to_request(...)\n");
			fulfillable.second.print();
			satisfied = move_resources_to_request(slot, fulfillable.second, &from_resource_pool);
			DBG(SUB_AGENT, "Actor Constraint: invade_bargain_new: current claim after moving non-malleable resources:\n");
			myRequests[slot].claim.print();

			if (need_malleability)
			{
				satisfied = move_resources_to_request(slot, malleable_candidates, NULL, true);
			}
		}

		if (satisfied)
		{
			DBG(SUB_AGENT, "Return claim. Slot %d, agent: %p\n", slot, this);
			myRequests[slot].claim.print();
			myRequests[slot].stage = octoinvade;
			AS::lockBargaining(&myRequests[slot].claim);
			check_sticky_invariant(slot);
		}
		else
		{
			DBG(SUB_AGENT, "invade_bargain: resource acquisition unsuccessful: constraints not fulfilled. Moving resources back to Agent's (%p) resource pool.\n", this);
			myRequests[slot].stage = idle;
			myRequests[slot].active = false;
			myRequests[slot].claim.moveAllResourcesToOtherClaim(this->myResourcePool);
			myRequests[slot].claim.reset();
			myRequests[slot].dispatch_claim = os::res::DispatchClaim(255);
		}

		if (satisfied)
		{

			myRequests[slot].claim.setOperatingPointIndex(fulfillable.second.getOperatingPointIndex());
		}
	}

	AS::leaveAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());

	if (AS::AGENT_TILE == myRequests[slot].RPCAnswerDesc.getParent().getTID())
	{
		myRequests[slot].RPCAnswerDesc.signalLocally(myRequests[slot].claim);
	}
	else
	{
		myRequests[slot].RPCAnswerDesc.sendReply(myRequests[slot].claim);
	}

	return (idle == myRequests[slot].stage ? 0 : 1);
}

uint8_t os::agent::AgentInstance::invade_bargain_old(uint8_t slot)
{
	kassert(myRequests[slot].active);
	kassert(bargain == myRequests[slot].stage);

	AgentSystem::enterAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());
	DBG(SUB_AGENT, "Agent %p Bargain in Slot %d\n", this, slot);

	// Checking resource pool
	bool noSuccess = false;
	os::agent::ResourceID res;
	os::agent::AgentClaim skiplist;

	// First, try to use resources from our own resource pool to avoid costly bargaining.
	{
		os::agent::AgentClaim from_resource_pool = fetch_resources_from_resource_pool(slot);
		skiplist.addClaim(from_resource_pool);
	}
	DBG(SUB_AGENT, "Moved resources from myResourcePool into myRequests[slot].claim\n");

	while ((myRequests[slot].pending || myRequests[slot].improvable) && !noSuccess)
	{

		// TODO: Actual Bargaining

		noSuccess = true;

		// greedily search for resource that improves the rating

		// TODO: alle rauspicken, bewerten und dann sortieren! Das hier ist super ineffizient

		os::agent::ResourceID best;
		best.resourceID = 0;
		best.tileID = 0;
		ResourceRating bestRating = 0;

		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			if (myRequests[slot].agent_constraints->isTileAllowed(res.tileID))
			{
				DBG(SUB_AGENT_NOISY, "invade_bargain: request constraints do not list tile %d as eligible, skipping...\n", res.tileID);
				continue;
			}

			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				DBG(SUB_AGENT_NOISY, "invade_bargain: checking resource (%d/%d)\n", res.tileID, res.resourceID);
				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "invade_bargain: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				if (skiplist.contains(res))
				{
					DBG(SUB_AGENT_NOISY, "invade_bargain: resource (%d/%d) in skiplist, skipping...\n", res.tileID, res.resourceID);
					continue;
				}

				const AgentInstance *reserved_agent = myResources[res.tileID][res.resourceID].ReservedForAgent;
				if ((reserved_agent != NULL) && (reserved_agent != this))
				{
					DBG(SUB_AGENT_NOISY, "invade_bargain: resource (%d/%d) reserved for agent %p, not us (%p)\n", res.tileID, res.resourceID, reserved_agent, this);
					continue;
				}

				/* A resource owner can be NULL following an unsetOwner() call. */
				const os::agent::AgentInstance *owner = os::agent::AgentSystem::getOwner(res);
				if (owner && owner->stayCauseSticky(res))
				{
					DBG(SUB_AGENT_NOISY, "invade_bargain: resource (%d/%d) unavailable - sticky to owning agent %p\n", res.tileID, res.resourceID, owner);
					continue;
				}

				if (!owner)
				{
					DBG(SUB_AGENT, "WARNING: invade_bargain: resource (%d/%d) has no owner. Agent system state potentially broken. Ignoring and further examining resource.\n", res.tileID, res.resourceID);
				}

				ResourceRating rating = myRequests[slot].agent_constraints->rateAdditionalResource(
						myRequests[slot].claim, res);
				DBG(SUB_AGENT_NOISY, "invade_bargain: resource (%d/%d) rating: %" PRIuMAX "; bestrating so far: %" PRIuMAX "\n", res.tileID, res.resourceID, static_cast<uintmax_t>(rating), static_cast<uintmax_t>(bestRating));
				if (rating > bestRating)
				{
					bestRating = rating;
					best = res;
				}
			}
		}

		if (bestRating > 0)
		{
			skiplist.add(best);
			noSuccess = false;
			(void)invade_bargain_transfer_resource(best);
		}

		DBG(SUB_AGENT, "Pending: %d, Improvable: %d, noSuccess: %d\n", myRequests[slot].pending, myRequests[slot].improvable, noSuccess);
	}

	if (myRequests[slot].agent_constraints->fulfilledBy(myRequests[slot].claim))
	{
		myRequests[slot].claim.print();
		DBG(SUB_AGENT, "Returning claim. Slot %d, agent: %p\n", slot, this);
		myRequests[slot].stage = octoinvade;
		os::agent::AgentSystem::lockBargaining(&myRequests[slot].claim);
		check_sticky_invariant(slot);
	}
	else
	{
		DBG(SUB_AGENT, "invade_bargain: resource acquisition unsuccessful: constraints not fulfilled. Moving resources back to Agent's (%p) resource pool.\n", this);
		myRequests[slot].stage = idle;
		myRequests[slot].active = false;
		myRequests[slot].claim.moveAllResourcesToOtherClaim(this->myResourcePool);
		myRequests[slot].claim.reset();
		myRequests[slot].dispatch_claim = os::res::DispatchClaim(255);
	}

	AgentSystem::leaveAgent(myRequests[slot].RPCAnswerDesc.getParent().getTID());

	if (myRequests[slot].RPCAnswerDesc.getParent().getTID() == os::agent::AgentSystem::AGENT_TILE)
	{
		myRequests[slot].RPCAnswerDesc.signalLocally(myRequests[slot].claim);
	}
	else
	{
		myRequests[slot].RPCAnswerDesc.sendReply(myRequests[slot].claim);
	}

	return (idle == myRequests[slot].stage ? 0 : 1);
}

os::agent::AgentClaim os::agent::AgentInstance::fetch_resources_from_resource_pool(const uint8_t slot)
{
	os::agent::AgentClaim ret;

	os::agent::ResourceID res;

	// AGENT CONSTRAINT
	if (myRequests[slot].agent_constraints)
	{
		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			if (!(is_tile_usable(slot, res.tileID)))
			{
				DBG(SUB_AGENT, "tile not usable\n");
				continue;
			}

			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (vipgBlockedPool.contains(res))
				{
					vipgBlockedPool.remove(res);
					myResourcePool.add(res);
					myResources[res.tileID][res.resourceID].ReservedForAgent = this;
					myResources[res.tileID][res.resourceID].ReservationRating = 0;
					DBG(SUB_AGENT, "fetch_resources_from_resource_pool: ViPG unblocking resource T:%d R:%d at INVADE\n", res.tileID, res.resourceID);
				}

				if (!myResourcePool.contains(res))
				{
					DBG(SUB_AGENT_NOISY, "fetch_resources_from_resource_pool: resource (%d/%d) not contained in resource pool\n", res.tileID, res.resourceID);
					continue;
				}

				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "fetch_resources_from_resource_pool: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				const AgentInstance *reserved_agent = myResources[res.tileID][res.resourceID].ReservedForAgent;
				if ((reserved_agent != NULL) && (reserved_agent != this))
				{
					DBG(SUB_AGENT_NOISY, "fetch_resources_from_resource_pool: resource (%d/%d) reserved for agent %p\n", res.tileID, res.resourceID, reserved_agent);
					continue;
				}

				/* Mark as done. */
				ret.add(res);

				if (myRequests[slot].agent_constraints->rateAdditionalResource(myRequests[slot].claim, res) <= 0)
				{
					DBG(SUB_AGENT, "rateAdditionalResource <= 0\n");
					continue;
				}

				this->transferResource(res, this);
			}
		}
	}
	// ACTOR CONSTRAINT
	else if (myRequests[slot].actor_constraint)
	{
		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			// MODIFIED FOR ACTOR CONSTRAINT
			if (!(myRequests[slot].actor_constraint->is_tile_usable(res.tileID)))
			{
				continue;
			}
			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{

				DBG(SUB_AGENT_NOISY, "Resource (%d/%d) is of type (%d)...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);

				if (vipgBlockedPool.contains(res))
				{
					vipgBlockedPool.remove(res);
					myResourcePool.add(res);
					myResources[res.tileID][res.resourceID].ReservedForAgent = this;
					myResources[res.tileID][res.resourceID].ReservationRating = 0;
					DBG(SUB_AGENT, "fetch_resources_from_resource_pool: ViPG unblocking resource T:%d R:%d at INVADE\n", res.tileID, res.resourceID);
				}

				if (!myResourcePool.contains(res))
				{
					DBG(SUB_AGENT_NOISY, "fetch_resources_from_resource_pool: resource (%d/%d) not contained in resource pool\n", res.tileID, res.resourceID);
					continue;
				}

				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "fetch_resources_from_resource_pool: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				const AgentInstance *reserved_agent = myResources[res.tileID][res.resourceID].ReservedForAgent;
				if ((reserved_agent != NULL) && (reserved_agent != this))
				{
					DBG(SUB_AGENT_NOISY, "fetch_resources_from_resource_pool: resource (%d/%d) reserved for agent %p\n", res.tileID, res.resourceID, reserved_agent);
					continue;
				}

				/* Mark as done. */
				ret.add(res);
				// TODO
				// Is it necessary rating additional resources like for the agent constraints ? I don't think so

				DBG(SUB_AGENT, "Transfer Resource\n");
				this->transferResource(res, this);
			}
		}
	}
	return (ret);
}

void os::agent::AgentInstance::fillPoolWithMyFreeResources(os::agent::AgentClaim &pool)
{
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (vipgBlockedPool.contains(res))
			{
				vipgBlockedPool.remove(res);
				myResourcePool.add(res);
				myResources[res.tileID][res.resourceID].ReservedForAgent = this;
				myResources[res.tileID][res.resourceID].ReservationRating = 0;
				DBG(SUB_AGENT, "fillPoolWithMyFreeResources: ViPG unblocking resource T:%d R:%d at INVADE\n", res.tileID, res.resourceID);
			}

			/*
			 * Filtering out resources that match at least one of the following criteria:
			 * - type of resource is 'none'
			 * - resource is reserved for another agent
			 */
			if (os::agent::HardwareMap[res.tileID][res.resourceID].type != none && (myResources[res.tileID][res.resourceID].ReservedForAgent == NULL || myResources[res.tileID][res.resourceID].ReservedForAgent == this))
			{
				/*
				 * Further filtering of resources. Filter out resources, that
				 * - are owned by an agent different from the idle agent and from us
				 * - or owned by us, but already used in another claim
				 */
				os::agent::AgentInstance *owner = os::agent::AgentSystem::getOwner(res);
				if (owner == NULL || owner == os::agent::AgentSystem::idleAgent || (owner == this && getClaim(res) == NULL))
				{
					DBG(SUB_AGENT_NOISY, "fillPoolWithMyFreeResources: add resource %d/%d to pool\n",
							res.tileID, res.resourceID);
					pool.add(res);
				}
			}
		}
	}
}

bool os::agent::AgentInstance::is_tile_usable(const uint8_t slot, const TileID tile_id) const
{
	typedef os::agent::AgentSystem AS;

	bool ret = false;

	if (!myRequests[slot].agent_constraints->isTileAllowed(tile_id))
	{
		return (false);
	}

	if (!myRequests[slot].agent_constraints->isTileShareable())
	{
		ret = AS::can_use_tile_exclusively(tile_id, *this);

		/*
		 * We're either the only agent holding a claim on the given
		 * tile or nobody is.
		 * In the first case, we need to look up whether there is a
		 * claim with tile sharing disabled, as tile sharing is defined
		 * per claim (that is, if we have one claim with tile sharing
		 * disabled on the tile, no matter from what agent, we must not
		 * allow another claim to invade this tile.)
		 */
		if (ret)
		{
			ret = !(this->get_has_active_claim(tile_id));
		}
	}
	else
	{
		const AgentInstance &otherAgent = AS::get_other_agent(tile_id, *this);

		ret = otherAgent.getIsTileShareable(tile_id);
	}

	return (ret);
}

bool os::agent::AgentInstance::get_has_active_claim(const TileID tile_id) const
{
	bool ret = false;

	for (std::size_t slot = 0; slot < os::agent::MAX_REQUESTS_PER_AGENT; ++slot)
	{
		if (!(myRequests[slot].active))
		{
			continue;
		}

		if (myRequests[slot].claim.containsTile(tile_id))
		{
			ret = true;
		}
	}

	return (ret);
}

int os::agent::AgentInstance::getSlot(const os::agent::ResourceID &resource) const
{
	for (std::size_t slot = 0; slot < os::agent::MAX_REQUESTS_PER_AGENT; ++slot)
	{
		if (myRequests[slot].active)
		{
			if (myRequests[slot].claim.contains(resource) || myRequests[slot].blockedResources.contains(resource))
			{
				return slot;
			}
		}
	}

	return -1;
}

const os::agent::AgentClaim *os::agent::AgentInstance::getClaim(const ResourceID &resource) const
{
	int slot = getSlot(resource);

	if (slot != -1)
	{
		if (myRequests[slot].claim.contains(resource))
		{
			return (&(myRequests[slot].claim));
		}
		else if (myRequests[slot].blockedResources.contains(resource))
		{
			return (&(myRequests[slot].blockedResources));
		}
	}

	return NULL;
}

os::agent::AgentClaim *os::agent::AgentInstance::getClaim(const ResourceID &resource)
{
	return (const_cast<os::agent::AgentClaim *>(const_cast<const os::agent::AgentInstance &>(*this).getClaim(resource)));
}

const os::agent::AbstractConstraint *os::agent::AgentInstance::getConstr(const ResourceID &resource) const
{
	int slot = getSlot(resource);
	const AbstractConstraint *ret = NULL;

	if (slot != -1)
	{
		if (myRequests[slot].agent_constraints)
		{
			ret = myRequests[slot].agent_constraints;
		}
		else if (myRequests[slot].actor_constraint)
		{
			ret = myRequests[slot].actor_constraint;
		}
		else
		{
			panic("getConstr: neither agent nor actor constraint\n");
		}
	}

	return (ret);
}

os::agent::AbstractConstraint *os::agent::AgentInstance::getConstr(const ResourceID &resource)
{
	return (const_cast<os::agent::AbstractConstraint *>(const_cast<const os::agent::AgentInstance &>(*this).getConstr(resource)));
}

bool os::agent::AgentInstance::move_resources_to_request(const uint8_t slot, const os::agent::AgentClaim &pseudo_claim, const os::agent::AgentClaim *skiplist, const bool malleability)
{
	typedef os::agent::AgentSystem AS;
	struct elem_t
	{
		os::agent::ResourceID first;
		os::agent::ResourceRating second;
	};

	/*
	 * Don't initialize to false here!
	 * The claim may already be fulfilled, for instance in a reinvade-context.
	 */

	bool ret = false;

	// AGENT CONSTRAINT
	if (myRequests[slot].agent_constraints)
	{
		ret = myRequests[slot].agent_constraints->fulfilledBy(myRequests[slot].claim);

		/* Create new pseudo claim being the difference between pseudo_claim and skiplist, if provided. */
		AgentClaim clean_pseudo_claim = pseudo_claim;
		if (skiplist)
		{
			clean_pseudo_claim.removeClaim(*skiplist);
		}

		std::size_t res_rating_size = clean_pseudo_claim.getResourceCount();

		if (0 == res_rating_size)
		{
			DBG(SUB_AGENT_NOISY, "move_resources_to_request: resource rating container empty. No resources to move?\n");
			return (ret);
		}

		{
			/*
			 * Sanity check time!
			 */
			const std::size_t max_size = 1024 * 100;

			const std::size_t rating_struct_size = reinterpret_cast<std::size_t>(static_cast<elem_t *>(NULL) + 1);

			/* rating_struct_size is now the size of elem_t within an array *including padding*. */
			const std::size_t rating_array_size = rating_struct_size * res_rating_size;

			const std::size_t bool_size = reinterpret_cast<std::size_t>(static_cast<bool *>(NULL) + 1);
			const std::size_t bool_array_size = bool_size * res_rating_size;

			/* Add another 16 bytes of "padding", just to be sure. */
			const std::size_t total_size = rating_array_size + bool_array_size + 16;

			if (total_size > max_size)
			{
				panic("move_resources_to_request: structures to be allocated on stack would exceed 100 KiB. That's probably a bad idea.");
			}
		}

		/*
		 * A general comment on this function: what we do here is potentially
		 * totally unsafe. It relies on assuming that whatever we "allocate"
		 * on the stack does not make it overflow. This is very dangerous.
		 * If possible, this should be ported to dynamically allocated memory
		 * ASAP.
		 */

		/*
		 * Create array that holds a pair of ResourceIDs and ResourceRatings
		 * corresponding to the size of resources in the clean pseudo claim.
		 */
		elem_t res_rating[res_rating_size];
		bool res_rating_valid[res_rating_size];
		DBG(SUB_AGENT_NOISY, "move_resources_to_request: sizeof(elem_t): %" PRIuMAX "; res_rating_size: %" PRIuMAX "; sizeof(res_rating): should be: %" PRIuMAX " - is: %" PRIuMAX "\n", static_cast<uintmax_t>(sizeof(elem_t)), static_cast<uintmax_t>(res_rating_size),
				static_cast<uintmax_t>(res_rating_size * sizeof(elem_t)), static_cast<uintmax_t>(sizeof(res_rating)));
		DBG(SUB_AGENT_NOISY, "move_resources_to_request: alignof(elem_t): %" PRIuMAX "; alignof(res_rating): %" PRIuMAX "\n",
				static_cast<uintmax_t>(__alignof(elem_t)), static_cast<uintmax_t>(__alignof(res_rating)));
		DBG(SUB_AGENT_NOISY, "move_resources_to_request: sizeof(bool): %" PRIuMAX "; sizeof(res_rating_valid): should be: %" PRIuMAX " - is: %" PRIuMAX "\n",
				static_cast<uintmax_t>(sizeof(bool)), static_cast<uintmax_t>(res_rating_size * sizeof(bool)), static_cast<uintmax_t>(sizeof(res_rating_valid)));
		DBG(SUB_AGENT_NOISY, "move_resources_to_request: alignof(bool): %" PRIuMAX "; alignof(res_rating_valid): %" PRIuMAX "\n",
				static_cast<uintmax_t>(__alignof(bool)), static_cast<uintmax_t>(__alignof(res_rating_valid)));

		/* Initialize resource rating array. */
		{
			std::size_t i = 0;
			os::agent::ResourceID res;
			for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
			{
				for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
				{
					if (clean_pseudo_claim.contains(res))
					{
						ResourceRating cur_rating = 0;
						if (malleability)
						{
							AgentInstance *res_owner = AS::getOwner(res);

							if (!res_owner)
							{
								panic("move_resources_to_request: init: resource owner NULL!");
							}

							AgentClaim *tmp_claim = res_owner->getClaim(res);

							if (!tmp_claim)
							{
								panic("move_resources_to_request: init: no claim for specified resource!");
							}

							cur_rating = myRequests[slot].agent_constraints->rateLosingResource(*tmp_claim, res);
							DBG(SUB_AGENT, "rateLosingResource returned %" PRIu64 "\n", cur_rating);
						}
						else
						{
							cur_rating = myRequests[slot].agent_constraints->rateAdditionalResource(
									myRequests[slot].claim, res);
						}
						DBG(SUB_AGENT_NOISY, "move_resources_to_request: replacing slot %" PRIuMAX " with resource (%d/%d), rating %" PRIuMAX "\n", static_cast<uintmax_t>(i), res.tileID, res.resourceID, static_cast<uintmax_t>(cur_rating));
						res_rating[i].first = res;
						res_rating[i].second = cur_rating;
						DBG(SUB_AGENT_NOISY, "move_resources_to_request: setting slot %" PRIuMAX " as valid\n", static_cast<uintmax_t>(i));
						res_rating_valid[i] = true;
						DBG(SUB_AGENT_NOISY, "move_resources_to_request: checkup: r(%d/%d), rr: %" PRIuMAX ", state: %" PRIu8 "\n", res_rating[i].first.tileID, res_rating[i].first.resourceID, static_cast<uintmax_t>(res_rating[i].second), res_rating_valid[i]);
						++i;
					}
				}
			}
		}

		if (malleability)
		{
			/*
			 * Pick until we run out of usable resources or the constraints are satisfied.
			 * We do not want to add more resources than the bare minimum.
			 */
			AgentClaim malleable_claim;
			ResourceRating worst = 0;
			std::size_t res_rating_cur_size = res_rating_size;
			do
			{
				std::size_t worst_elem = 0;
				worst = 0;

				/* Find resource with min rating. */
				DBG(SUB_AGENT_NOISY, "move_resources_to_request: searching for new minimum.\n");
				for (std::size_t i = 0; i < res_rating_size; ++i)
				{
					if (res_rating_valid[i])
					{
						ResourceRating cur_val = res_rating[i].second;
						if ((0 == worst) && (0 != cur_val))
						{
							DBG(SUB_AGENT_NOISY, "move_resources_to_request: first non-zero value in slot %" PRIuMAX ": r(%d/%d), rr: %" PRIuMAX ", valid.\n", static_cast<uintmax_t>(i), res_rating[i].first.tileID, res_rating[i].first.resourceID, static_cast<uintmax_t>(cur_val));
							worst = cur_val;
							worst_elem = i;
						}
						else if ((0 != worst) && (0 != cur_val) && (cur_val <= worst))
						{
							DBG(SUB_AGENT_NOISY, "move_resources_to_request: found new minimum in slot %" PRIuMAX ": r(%d/%d), rr: %" PRIuMAX ", valid.\n", static_cast<uintmax_t>(i), res_rating[i].first.tileID, res_rating[i].first.resourceID, static_cast<uintmax_t>(cur_val));
							worst = cur_val;
							worst_elem = i;
						}
					}
				}

				if (0 != worst)
				{
					DBG(SUB_AGENT_NOISY, "move_resources_to_request: minimum slot: %" PRIuMAX ", rr: %" PRIuMAX "\n", static_cast<uintmax_t>(worst_elem), static_cast<uintmax_t>(worst));
					malleable_claim.add(res_rating[worst_elem].first);
					res_rating_valid[worst_elem] = false;
					--res_rating_cur_size;

					/*
					 * Copy current claim and add all malleable resources.
					 * Otherwise, we always only check the original, non-modified
					 * claim for satisfaction...
					 */
					AgentClaim tmp_claim = myRequests[slot].claim;
					tmp_claim.addClaim(malleable_claim);
					ret = myRequests[slot].agent_constraints->fulfilledBy(tmp_claim);

					/* Recalculate ratings. */
					for (std::size_t i = 0; i < res_rating_size; ++i)
					{
						if (res_rating_valid[i])
						{
							AgentInstance *res_owner = AS::getOwner(res_rating[i].first);

							if (!res_owner)
							{
								panic("move_resources_to_request: recalc: resource owner NULL!");
							}

							AgentClaim *tmp_claim = res_owner->getClaim(res_rating[i].first);

							if (!tmp_claim)
							{
								panic("move_resources_to_request: init: no claim for specified resource!");
							}

							res_rating[i].second = myRequests[slot].agent_constraints->rateLosingResource(*tmp_claim,
																																														res_rating[i].first);
							DBG(SUB_AGENT_NOISY, "move_resources_to_request: recalculated rr for slot %" PRIuMAX ": %" PRIuMAX "\n", static_cast<uintmax_t>(i), static_cast<uintmax_t>(res_rating[i].second));
						}
					}
				}
			} while ((res_rating_cur_size > 0) && (0 != worst) && (!ret));

			/* Constraints should be fulfilled by now. */
			if (ret)
			{
				/* malleable_claim contains all needed resources. Call the malleability handler and do something to release them. */
				remove_malleable_resources(malleable_claim);

				/* Resources have been forcefully "deallocated". Fetch them! */
				transfer_malleable_resources(slot, malleable_claim);

				DBG(SUB_AGENT, "invade_bargain_new: current claim after moving malleable resources:\n");
				myRequests[slot].claim.print();
			}
			else
			{
				panic("move_resources_to_request: constraints still not fulfilled after running out of resources in malleability part. This should not happen!");
			}
		}
		else
		{
			/* Pick until we run out of usable resources. */
			ResourceRating best = 0;
			std::size_t res_rating_cur_size = res_rating_size;
			do
			{
				std::size_t best_elem = 0;
				best = 0;

				/* Find resource with max rating. */
				DBG(SUB_AGENT_NOISY, "move_resources_to_request: searching for new maximum.\n");
				for (std::size_t i = 0; i < res_rating_size; ++i)
				{
					if (res_rating_valid[i])
					{
						ResourceRating cur_val = res_rating[i].second;
						if (cur_val > best)
						{
							DBG(SUB_AGENT_NOISY, "move_resources_to_request: found new maximum in slot %" PRIuMAX ": r(%d/%d), rr: %" PRIuMAX ", valid.\n", static_cast<uintmax_t>(i), res_rating[i].first.tileID, res_rating[i].first.resourceID, static_cast<uintmax_t>(cur_val));
							best = cur_val;
							best_elem = i;
						}
					}
				}
				DBG(SUB_AGENT_NOISY, "move_resources_to_request: maximum slot: %" PRIuMAX ", rr: %" PRIuMAX "\n", static_cast<uintmax_t>(best_elem), static_cast<uintmax_t>(best));

				if (0 != best)
				{
					/* Got a "real", best-rated resource. Let's move it to the request slot. */
					DBG(SUB_AGENT_NOISY, "move_resources_to_request: moving resource (%d/%d) to pending request slot.\n", res_rating[best_elem].first.tileID, res_rating[best_elem].first.resourceID);
					if (invade_bargain_transfer_resource(res_rating[best_elem].first))
					{
						ret = myRequests[slot].agent_constraints->fulfilledBy(myRequests[slot].claim);
						DBG(SUB_AGENT_NOISY, "move_resources_to_request: constraints satisfaction after move: %" PRIu8 "\n", static_cast<uint8_t>(ret));
					}

					/*
					 * Mark best resource as invalid.
					 * If the transfer was successful, we don't need it anymore.
					 * If the transfer failed, it probably doesn't make sense to try again later.
					 */
					res_rating_valid[best_elem] = false;
					--res_rating_cur_size;

					/* Recalculate ratings. */
					for (std::size_t i = 0; i < res_rating_size; ++i)
					{
						if (res_rating_valid[i])
						{
							res_rating[i].second = myRequests[slot].agent_constraints->rateAdditionalResource(
									myRequests[slot].claim, res_rating[i].first);
							DBG(SUB_AGENT_NOISY, "move_resources_to_request: recalculated rr for slot %" PRIuMAX ": %" PRIuMAX "\n", static_cast<uintmax_t>(i), static_cast<uintmax_t>(res_rating[i].second));
						}
					}
				}
			} while ((res_rating_cur_size > 0) && (0 != best));
		}
	}

	// ACTOR CONSTRAINT
	else if (myRequests[slot].actor_constraint)
	{
		DBG(SUB_AGENT, "ACTOR CONSTRAINT\n");

		ret = (os::agent::AgentSystem::solver->solve((*(myRequests[slot].actor_constraint)), myRequests[slot].claim)).first;

		/* Create new pseudo claim being the difference between pseudo_claim and skiplist, if provided. */
		AgentClaim clean_pseudo_claim = pseudo_claim;
		if (skiplist)
		{
			clean_pseudo_claim.removeClaim(*skiplist);
		}

		std::size_t res_rating_size = clean_pseudo_claim.getResourceCount();

		if (0 == res_rating_size)
		{
			DBG(SUB_AGENT_NOISY, "move_resources_to_request: resource rating container empty. No resources to move?\n");
			return (ret);
		}

		// TODO : comment
		/*
		 * Pick until we run out of usable resources or the constraints are satisfied.
		 * We do not want to add more resources than the bare minimum.
		 */
		os::agent::ResourceID res;

		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (clean_pseudo_claim.contains(res))
				{
					DBG(SUB_AGENT, "Resource (%d/%d) in claim\n", res.tileID, res.resourceID);
					if (invade_bargain_transfer_resource(res, false, slot))
						DBG(SUB_AGENT, "Add to request claim\n");
					else
						panic("Resource wasn't trasfered to claim");
				}
			}
		}

		ret = (os::agent::AgentSystem::solver->solve((*(myRequests[slot].actor_constraint)), myRequests[slot].claim)).first;
		myRequests[slot].pending = false; // HERE ????

		/* Constraints should be fulfilled by now. */
		if (!ret)
		{
			panic("move_resources_to_request: constraints still not fulfilled after running out of resources in malleability part. This should not happen!");
		}
	}

	return (ret);
}

bool os::agent::AgentInstance::invade_bargain_transfer_resource(const os::agent::ResourceID &res, const bool isAgentConstraint, const uint8_t slot)
{
	typedef os::agent::AgentSystem AS;

	bool ret = false;

	/*
	 * FIXME: this algorithm looks weird.
	 *
	 * RESERVED means that a resource has been reserved for another agent (in this case, us.)
	 * However, in that case, we explicitly unreserve the resource on both the other agent's
	 * and our part.
	 * This doesn't make a lot of sense.
	 */
	os::agent::AgentInstance *owner = AS::getOwner(res);

	if (isAgentConstraint)
	{
		if (AS::testFlag(res, AS::FLAG_AVAILABLE_FOR_BARGAINING) && owner && owner != this)
		{
			DBG(SUB_AGENT, "invade_bargain_transfer_resource: Res (%" PRIu8 "/%" PRIu8 ") Owner (%p) rates %" PRIu64 ", me (%p) rates %" PRIu64 "\n", res.tileID, res.resourceID, owner, owner->getRating(res), this, this->getRating(res));

			os::agent::ResourceRating myRating = this->getRating(res, owner);
			os::agent::ResourceRating otherRating = owner->getRating(res, this);

			if (otherRating < myRating)
			{
				DBG(SUB_AGENT, "invade_bargain_transfer_resource: Agent %p losing resource (%" PRIu8 "/%" PRIu8 ") because rating %" PRIu64 " < %" PRIu64 "\n", owner, res.tileID, res.resourceID, otherRating, myRating);
				switch (owner->loseResource(res, this, myRating))
				{
				case RESOURCE_AVAILABLE:
				{
					DBG(SUB_AGENT, "invade_bargain_transfer_resource: Agent %p gains resource (%" PRIu8 "/%" PRIu8 ") because rating %" PRIu64 " < %" PRIu64 "\n", this, res.tileID, res.resourceID, otherRating, myRating);
					this->gainResource(res);
					ret = true;
					break;
				}
				case RESERVED:
				{
					ResourceID otherRes = holdReservationFor(owner);

					if (otherRes.isInvalid())
					{
						/* invalid => owner does not reserve anything */
						DBG(SUB_AGENT, "invade_bargain_transfer_resource: Adding to my (%p) reservations: (%d/%d)\n", this, res.tileID, res.resourceID);
						myReservations.add(res);
						ret = false;
					}
					else
					{
						/* valid => owner reserves my otherRes */
						DBG(SUB_AGENT, "invade_bargain_transfer_resource: Other agent should unreserve the resource (%d/%d) ...\n", res.tileID, res.resourceID);
						DBG(SUB_AGENT_NOISY, "invade_bargain_transfer_resource: unreserve((%d/%d), this(%p));\n", res.tileID, res.resourceID, this);
						owner->unreserve(res, this);

						DBG(SUB_AGENT, "invade_bargain_transfer_resource: Other dude should give up otherRes (%d/%d) ...\n", res.tileID, res.resourceID);
						owner->giveUpReservation(otherRes);

						DBG(SUB_AGENT_NOISY, "invade_bargain_transfer_resource: unreserve((%d/%d), owner(%p));\n", otherRes.tileID, otherRes.resourceID, owner);
						this->unreserve(otherRes, owner);
						ret = false;
					}
					break;
				}
				case NOT_RESERVED:
				{
					ret = false;
					break;
				}
				default:
				{
					panic("invade_bargain_transfer_resource: unknown/not handled reservation state.");
					ret = false;
					break;
				}
				}
			}
			else
			{
				DBG(SUB_AGENT, "invade_bargain_transfer_resource: cannot transfer resource (%d/%d): current owner rates %" PRIuMAX ", me rates lower at %" PRIuMAX ".\n", res.tileID, res.resourceID, static_cast<uintmax_t>(otherRating), static_cast<uintmax_t>(myRating));
			}
		}
		else
		{
			DBG(SUB_AGENT, "invade_bargain_transfer_resource: cannot transfer resource (%d/%d): owner (%p) either NULL, me (%p) or not available for bargaining (%d).\n", res.tileID, res.resourceID, owner, this, AS::testFlag(res, AS::FLAG_AVAILABLE_FOR_BARGAINING));
		}
	}
	else if (!isAgentConstraint)
	{
		if (AS::testFlag(res, AS::FLAG_AVAILABLE_FOR_BARGAINING) && owner && owner != this)
		{
			DBG(SUB_AGENT, "invade_bargain_transfer_resource: Res (%" PRIu8 "/%" PRIu8 "), Owner (%p) , me (%p)\n", res.tileID, res.resourceID, owner, this);
			DBG(SUB_AGENT, "invade_bargain_transfer_resource: Agent %p losing resource (%" PRIu8 "/%" PRIu8 ")\n", owner, res.tileID, res.resourceID);

			switch (owner->loseResource(res, this, false))
			{
			case RESOURCE_AVAILABLE:
			{
				DBG(SUB_AGENT, "invade_bargain_transfer_resource: Agent %p gains resource (%" PRIu8 "/%" PRIu8 ")\n", this, res.tileID, res.resourceID);
				this->gainResource(res, false, slot);
				ret = true;
				break;
			}
			case RESERVED:
			{
				ResourceID otherRes = holdReservationFor(owner);

				if (otherRes.isInvalid())
				{
					/* invalid => owner does not reserve anything */
					DBG(SUB_AGENT, "invade_bargain_transfer_resource: Adding to my (%p) reservations: (%d/%d)\n", this, res.tileID, res.resourceID);
					myReservations.add(res);
					ret = false;
				}
				else
				{
					/* valid => owner reserves my otherRes */
					DBG(SUB_AGENT, "invade_bargain_transfer_resource: Other agent should unreserve the resource (%d/%d) ...\n", res.tileID, res.resourceID);
					DBG(SUB_AGENT_NOISY, "invade_bargain_transfer_resource: unreserve((%d/%d), this(%p));\n", res.tileID, res.resourceID, this);
					owner->unreserve(res, this);

					DBG(SUB_AGENT, "invade_bargain_transfer_resource: Other dude should give up otherRes (%d/%d) ...\n", res.tileID, res.resourceID);
					owner->giveUpReservation(otherRes);

					DBG(SUB_AGENT_NOISY, "invade_bargain_transfer_resource: unreserve((%d/%d), owner(%p));\n", otherRes.tileID, otherRes.resourceID, owner);
					this->unreserve(otherRes, owner);
					ret = false;
				}
				break;
			}
			case NOT_RESERVED:
			{
				ret = false;
				break;
			}
			default:
			{
				panic("invade_bargain_transfer_resource: unknown/not handled reservation state.");
				ret = false;
				break;
			}
			}
		}
		else
		{
			DBG(SUB_AGENT, "invade_bargain_transfer_resource: cannot transfer resource (%d/%d): owner (%p) either NULL, me (%p) or not available for bargaining (%d).\n", res.tileID, res.resourceID, owner, this, AS::testFlag(res, AS::FLAG_AVAILABLE_FOR_BARGAINING));
		}
	}
	return (ret);
}

os::agent::AgentClaim os::agent::AgentInstance::invade_bargain_get_malleable_candidates(const uint8_t slot) const
{
	typedef os::agent::AgentSystem AS;

	AgentClaim ret;

	os::agent::ResourceID res;

	// AGENT CONSTRAINT
	if (myRequests[slot].agent_constraints)
	{
		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			if (!(is_tile_usable(slot, res.tileID)))
			{
				DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: tile (%d) unusable because either not permitted by constraints or another agent is using it exclusively already.\n", res.tileID);
				continue;
			}

			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				if (!myRequests[slot].agent_constraints->isResourceAllowed(res))
				{
					continue;
				}

				const AgentInstance *res_owner = AS::getOwner(res);
				if (res_owner)
				{
					if (res_owner == this)
					{
						if (myRequests[slot].claim.contains(res))
						{
							DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: resource (%d/%d) already part of claim in slot %" PRIu8 ". Skipping resource.\n", res.tileID, res.resourceID, slot);
							continue;
						}
					}

					if (res_owner->getConstr(res) && res_owner->getConstr(res)->isAgentConstraint())
					{
						const AgentConstraint *res_constr = static_cast<const os::agent::AgentConstraint *>(res_owner->getConstr(res));
						bool malleable = res_constr->isMalleable();

						if (malleable)
						{
							/*
							 * Recap: the resource
							 *   - is usable according to the slot's constraints,
							 *   - has an owner (which at this point might be us - part of another active slot),
							 *   - has a valid set of constraints,
							 *   - conforms to tile-shareability (implicitly checked for via is_tile_usable()),
							 *   and
							 *   - these constraints specify malleability.
							 *
							 * The last thing to check is whether giving it away is an option or not.
							 */

							/* I will check for tile stickiness here - but is that a good idea or really needed? */
							if (res_owner->stayCauseSticky(res))
							{
								DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: resource (%d/%d) already part of claim in slot %" PRIu8 ". Skipping resource.\n", res.tileID, res.resourceID, slot);
								continue;
							}

							const AgentClaim *res_claim = res_owner->getClaim(res);
							if (res_claim)
							{
								/*
								 * Copy the claim and remove the already gathered resources.
								 * Important because we would otherwise remove all resources,
								 * if the claim specified malleability and the minimum resource
								 * amount constraint does not match the maximum resource constraint.
								 */
								AgentClaim tmp_claim = *res_claim;
								tmp_claim.removeClaim(ret);

								if (res_constr->canLoseResource(tmp_claim, res))
								{
									DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: other agent can lose resource (%d/%d). Adding to malleability set.\n", res.tileID, res.resourceID);
									ret.add(res);
								}
								else
								{
									DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: other agent may not lose resource (%d/%d). Skipping resource.\n", res.tileID, res.resourceID);
									continue;
								}
							}
							else
							{
								DBG(SUB_AGENT, "WARNING: invade_bargain_get_malleable_candidates: failed to get claim for resource (%d/%d). Agent system state potentially broken. Skipping resource.\n", res.tileID, res.resourceID);
								continue;
							}
						}
						else
						{
							DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: constraints for resource (%d/%d) not malleable. Skipping resource.\n", res.tileID, res.resourceID);
							continue;
						}
					}
					else
					{
						DBG(SUB_AGENT, "WARNING: invade_bargain_get_malleable_candidates: failed to get constraints for resource (%d/%d). Agent system state potentially broken. Skipping resource.\n", res.tileID, res.resourceID);
						continue;
					}
				}
				else
				{
					DBG(SUB_AGENT, "WARNING: invade_bargain_get_malleable_candidates: resource (%d/%d) has no owner. Agent system state potentially broken. Skipping resource.\n", res.tileID, res.resourceID);
					continue;
				}
			}
		}
	}

	// ACTOR CONSTRAINT
	if (myRequests[slot].actor_constraint)
	{
		DBG(SUB_AGENT, "ACTOR CONSTRAINT\n");

		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			// MODIFIED FOR ACTOR CONSTRAINT
			if (!(myRequests[slot].actor_constraint->is_tile_usable(res.tileID)))
			{
				continue;
			}

			for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (none == os::agent::HardwareMap[res.tileID][res.resourceID].type)
				{
					DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: resource (%d/%d) unusable because its type is none (%d), skipping...\n", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					continue;
				}

				// MODIFIED FOR ACTOR CONSTRAINT
				if (!(myRequests[slot].actor_constraint->is_resource_usable(res)))
				{
					continue;
				}

				const AgentInstance *res_owner = AS::getOwner(res);

				if (res_owner)
				{
					if (res_owner == this)
					{
						if (myRequests[slot].claim.contains(res))
						{
							DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: resource (%d/%d) already part of claim in slot %" PRIu8 ". Skipping resource.\n", res.tileID, res.resourceID, slot);
							continue;
						}
					}

					if (!res_owner->getConstr(res) || !res_owner->getConstr(res)->isAgentConstraint())
					{
						continue;
					}
					const AgentConstraint *res_constr = static_cast<const os::agent::AgentConstraint *>(res_owner->getConstr(res));
					bool malleable = res_constr->isMalleable();

					if (malleable)
					{
						/*
						 * Recap: the resource
						 *   - is usable according to the slot's constraints,
						 *   - has an owner (which at this point might be us - part of another active slot),
						 *   - has a valid set of constraints,
						 *   - conforms to tile-shareability (implicitly checked for via is_tile_usable()),
						 *   and
						 *   - these constraints specify malleability.
						 *
						 * The last thing to check is whether giving it away is an option or not.
						 */

						/* I will check for tile stickiness here - but is that a good idea or really needed? */
						if (res_owner->stayCauseSticky(res))
						{
							DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: resource (%d/%d) already part of claim in slot %" PRIu8 ". Skipping resource.\n", res.tileID, res.resourceID, slot);
							continue;
						}

						const AgentClaim *res_claim = res_owner->getClaim(res);

						if (res_claim)
						{
							/*
							 * Copy the claim and remove the already gathered resources.
							 * Important because we would otherwise remove all resources,
							 * if the claim specified malleability and the minimum resource
							 * amount constraint does not match the maximum resource constraint.
							 */
							AgentClaim tmp_claim = *res_claim;
							tmp_claim.removeClaim(ret); // Is it right here : ret or res???;

							if (res_constr->canLoseResource(tmp_claim, res))
							{
								DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: other agent can lose resource (%d/%d). Adding to malleability set.\n", res.tileID, res.resourceID);
								ret.add(res);
							}
							else
							{
								DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: other agent may not lose resource (%d/%d). Skipping resource.\n", res.tileID, res.resourceID);
								continue;
							}
						}
						else
						{
							DBG(SUB_AGENT, "WARNING: invade_bargain_get_malleable_candidates: failed to get claim for resource (%d/%d). Agent system state potentially broken. Skipping resource.\n", res.tileID, res.resourceID);
							continue;
						}
					}
					else
					{
						DBG(SUB_AGENT_NOISY, "invade_bargain_get_malleable_candidates: constraints for resource (%d/%d) not malleable. Skipping resource.\n", res.tileID, res.resourceID);
						continue;
					}
				}
				else
				{
					DBG(SUB_AGENT, "WARNING: invade_bargain_get_malleable_candidates: resource (%d/%d) has no owner. Agent system state potentially broken. Skipping resource.\n", res.tileID, res.resourceID);
					continue;
				}
			}
		}
	}

	return (ret);
}

std::ptrdiff_t os::agent::AgentInstance::get_agents_element_index(const AgentInstance *const *arr, const AgentInstance *elem, const std::size_t arr_size) const
{
	std::ptrdiff_t ret = -1;

	for (std::size_t i = 0; i < arr_size; ++i)
	{
		if (arr[i] == elem)
		{
			ret = i;
		}
	}

	return (ret);
}

std::size_t os::agent::AgentInstance::get_real_claims_count_for_agent(const os::agent::AgentInstance &agent, const os::agent::AgentClaim &claim) const
{
	typedef os::agent::AgentSystem AS;

	size_t ret = 0;
	const AgentClaim *claims[MAX_REQUESTS_PER_AGENT];

	for (std::size_t i = 0; i < MAX_REQUESTS_PER_AGENT; ++i)
	{
		claims[i] = NULL;
	}

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (claim.contains(res))
			{
				const AgentInstance *res_owner = AS::getOwner(res);

				if (!res_owner)
				{
					panic("get_real_claims_count_for_agent: owner of resource NULL!");
				}

				if (res_owner == &agent)
				{
					const AgentClaim *cur_claim = res_owner->getClaim(res);

					if (!cur_claim)
					{
						panic("get_real_claims_count_for_agent: claim is NULL!");
					}

					bool included = false;
					for (std::size_t y = 0; y < ret; ++y)
					{
						if (cur_claim == claims[y])
						{
							included = true;
						}
					}

					/* If claim is unknown, add it. */
					if (!included)
					{
						if (ret >= MAX_REQUESTS_PER_AGENT)
						{
							panic("get_real_claims_count_for_agent: more claims than possible slots - out of bounds!");
						}

						claims[ret] = cur_claim;
						++ret;
					}
				}
			}
		}
	}

	/* claims_count now contains the current agent's claims count. */
	return (ret);
}

void os::agent::AgentInstance::get_real_claims_for_agents(const os::agent::AgentInstance *const *agents, const std::size_t agents_count, const os::agent::AgentClaim &claim, const std::size_t max_claims_count, os::agent::AgentClaim **claims, std::size_t *claims_count)
{
	typedef os::agent::AgentSystem AS;

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (claim.contains(res))
			{
				AgentInstance *res_owner = AS::getOwner(res);

				if (!res_owner)
				{
					panic("get_real_claims_for_agents: owner of resource NULL!");
				}

				std::ptrdiff_t agents_i = get_agents_element_index(agents, res_owner, agents_count);

				if (agents_i < 0)
				{
					panic("get_real_claims_for_agents: agents element index invalid!");
				}

				AgentClaim *cur_claim = res_owner->getClaim(res);

				if (!cur_claim)
				{
					panic("get_real_claims_for_agents: claim is NULL!");
				}

				bool included = false;
				for (std::size_t y = 0; y < claims_count[agents_i]; ++y)
				{
					if (cur_claim == claims[(agents_i * max_claims_count) + y])
					{
						included = true;
					}
				}

				/* If claim is unknown, add it. */
				if (!included)
				{
					if (claims_count[agents_i] >= max_claims_count)
					{
						panic("get_real_claims_for_agents: more claims than possible slots - out of bounds!");
					}

					claims[(agents_i * max_claims_count) + claims_count[agents_i]] = cur_claim;
					++claims_count[agents_i];
				}
			}
		}
	}
}

void os::agent::AgentInstance::remove_malleable_resources(const os::agent::AgentClaim &claim)
{
	typedef os::agent::AgentSystem AS;
	typedef hw::hal::Tile tile_t;

	if (claim.isEmpty())
	{
		panic("remove_malleable_resources: called with empty claim. This is an error.");
	}

	const std::size_t tile_count = tile_t::getTileCount();
	const std::size_t res_per_tile = os::agent::MAX_RES_PER_TILE;

	AgentInstance *agents[claim.getResourceCount()];

	/* We may not initialize variable-sized arrays directly, so do it manually. */
	for (std::size_t i = 0; i < claim.getResourceCount(); ++i)
	{
		agents[i] = NULL;
	}

	/*
	 * Generate array of unique agent pointers to remove resources from.
	 * agents_count contains the number of unique agent pointers afterwards.
	 */
	os::agent::ResourceID res;
	std::size_t agents_count = 0;
	for (res.tileID = 0; res.tileID < tile_count; ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < res_per_tile; ++res.resourceID)
		{
			if (claim.contains(res))
			{
				AgentInstance *res_owner = AS::getOwner(res);

				if (!res_owner)
				{
					panic("remove_malleable_resources: agent init: owner of resource NULL!");
				}

				/* Check, if current resource's owner is already included in the agents list. */
				bool included = false;
				for (std::size_t i = 0; i < agents_count; ++i)
				{
					if (res_owner == agents[i])
					{
						included = true;
					}
				}

				/* If agent is unknown, add it. */
				if (!included)
				{
					if (agents_count >= claim.getResourceCount())
					{
						panic("remove_malleable_resources: agent init: more agents than resources - out of bounds!");
					}

					agents[agents_count] = res_owner;
					++agents_count;
				}
			}
		}
	}

	/* Go through agents array and find the maximum number of claims per agent. */
	std::size_t max_claims_count = 0;
	for (std::size_t i = 0; i < agents_count; ++i)
	{
		std::size_t claims_count = get_real_claims_count_for_agent(*(agents[i]), claim);

		if (claims_count >= max_claims_count)
		{
			max_claims_count = claims_count;
		}
	}

	/* Finally initialize and build the actual claims array per agent. */
	std::size_t claims_count[agents_count];
	AgentClaim *claims[agents_count][max_claims_count];

	for (std::size_t i = 0; i < agents_count; ++i)
	{
		for (std::size_t y = 0; y < max_claims_count; ++y)
		{
			claims[i][y] = NULL;
		}

		claims_count[i] = 0;
	}

	get_real_claims_for_agents(agents, agents_count, claim, max_claims_count, reinterpret_cast<AgentClaim **>(claims), claims_count);

	/* Go through agents and claims array and call resize handlers. */
	for (std::size_t i = 0; i < agents_count; ++i)
	{
		for (std::size_t y = 0; y < claims_count[i]; ++y)
		{
			AgentConstraint *tmp_constr = NULL;
			AgentClaim *cur_claim = claims[i][y];
			AgentRequest_s *remote_slot = agents[i]->getSlot(agents[i]->get_slot_no_of_claim_no(cur_claim->getUcid()));

			AgentClaim loss_claim = claim;
			loss_claim.intersectClaim(*cur_claim);

			for (res.tileID = 0; res.tileID < tile_count; ++res.tileID)
			{
				for (res.resourceID = 0; res.resourceID < res_per_tile; ++res.resourceID)
				{
					if (loss_claim.contains(res))
					{
						if (!tmp_constr && agents[i]->getConstr(res)->isAgentConstraint())
						{
							tmp_constr = const_cast<AgentConstraint *>(static_cast<const os::agent::AgentConstraint *>(agents[i]->getConstr(res)));
							if (tmp_constr)
							{
								break;
							}
						}
					}
				}

				/* Don't waste cycles if we already fetched tmp_constr successfully. */
				if (tmp_constr)
				{
					break;
				}
			}

			if (!tmp_constr)
			{
				//				panic("remove_malleable_resources: resize handlers: constraints NULL!");
				continue;
			}

			/* Call the resize handler via RPC. */
			DBG(SUB_AGENT, "remove_malleable_resources: calling resize handlers: current tile ID: %d, current claim tag: %d\n", remote_slot->dispatch_claim.getTID(), remote_slot->dispatch_claim.getTag().value);
			(void)os::agent::Agent::run_resize_handler(remote_slot->dispatch_claim, tmp_constr->getResizeEnvPointer(), loss_claim, tmp_constr->getResizeHandler(), tile_count, res_per_tile);
		}
	}

	/*
	 * By now the applications should have had enough time to move data off the affected resources/tiles.
	 * We have to actually remove the resources from their original claims.
	 */
	for (std::size_t i = 0; i < agents_count; ++i)
	{
		for (std::size_t y = 0; y < claims_count[i]; ++y)
		{
			AgentClaim remove_claim = claim;
			AgentClaim *cur_claim = claims[i][y];

			if (!cur_claim)
			{
				panic("remove_malleable_resources: cur_claim is NULL! this should not happen!");
			}

			remove_claim.intersectClaim(*cur_claim);

			/* Build the "difference set" as a new claim. */
			cur_claim->removeClaim(remove_claim);

			AgentOctoClaim *cur_octo_claim = cur_claim->getAgentOctoClaim();

			if (!cur_octo_claim)
			{
				DBG(SUB_AGENT, "remove_malleable_resources: current claim: %p, UCID: %" PRIu32 "\n", cur_claim, cur_claim->getUcid());
				panic("remove_malleable_resources: cur_octo_claim is NULL! this should not happen!");
			}

			/* Now we actually have to "adapt" the corresponding AgentOctoClaim to the modified claim. */
			AgentRequest_s *remote_slot = agents[i]->getSlot(agents[i]->get_slot_no_of_claim_no(cur_claim->getUcid()));
			DBG(SUB_AGENT, "remove_malleable_resources: adaptation: current tile ID: %d, current claim tag: %d\n", remote_slot->dispatch_claim.getTID(), remote_slot->dispatch_claim.getTag().value);
			(void)os::agent::Agent::update_claim_structures(remote_slot->dispatch_claim, cur_octo_claim, *cur_claim);

			/* Unset the owner within the Agent System. */
			for (res.tileID = 0; res.tileID < tile_count; ++res.tileID)
			{
				for (res.resourceID = 0; res.resourceID < res_per_tile; ++res.resourceID)
				{
					if (remove_claim.contains(res))
					{
						AS::unsetOwner(res, agents[i]);
					}
				}
			}
		}
	}
}

void os::agent::AgentInstance::transfer_malleable_resources(const uint8_t slot, const AgentClaim &claim)
{
	typedef os::agent::AgentSystem AS;

	if (claim.isEmpty())
	{
		panic("transfer_malleable_resources: called with empty claim. This is an error.");
	}

	if (MAX_REQUESTS_PER_AGENT == slot)
	{
		panic("transfer_malleable_resources: called with invalid slot.");
	}

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (claim.contains(res))
			{
				AS::setOwner(res, this);

				/* Reset reservation status. Should not be necessary, but play it safe. */
				myResources[res.tileID][res.resourceID].ReservationRating = 0;
				myResources[res.tileID][res.resourceID].ReservedForAgent = NULL;

				/* Actually move to slot, yay. */
				myRequests[slot].claim.add(res);

				/* Never gonna give you up. If sticky constraints are set, anyway. */
				sticky_tiles.set(res.tileID);
			}
		}
	}
}

bool os::agent::AgentInstance::is_part_of_active_request(const ResourceID &resource) const
{
	typedef os::agent::AgentSystem AS;

	bool ret = false;

	const AgentInstance *res_owner = AS::getOwner(resource);

	if (res_owner != this)
	{
		DBG(SUB_AGENT, "is_part_of_active_request: called for resource (%d/%d) not owned by me (%p) - returning false!\n", resource.tileID, resource.resourceID, this);
	}
	else
	{
		const auto *res_claim = getClaim(resource);

		if (res_claim)
		{
			ret = true;
		}
	}

	return (ret);
}

void os::agent::AgentInstance::register_AgentOctoClaim(uint32_t claim_id, os::agent::AgentOctoClaim &octo_claim, os::res::DispatchClaim dispatch_claim)
{
	auto slot = get_slot_no_of_claim_no(claim_id);
	myRequests[slot].claim.setAgentOctoClaim(octo_claim);
	DBG(SUB_AGENT, "register_AgentOctoClaim: setting DC: tile ID: %d, claim tag: %d\n", dispatch_claim.getTID(), dispatch_claim.getTag().value);
	myRequests[slot].dispatch_claim = dispatch_claim;
}

// This function is outdated. The new version is part of the AgentConstraint class.
// Let's keep this in the code for the moment as a reference
os::agent::ResourceRating os::agent::downey_rate(uint16_t n, uint16_t A, uint16_t s)
{
	/*
	 * Use these wide variants when calculating stuff to not run into
	 * overflowing issues.
	 */
	uint64_t n_wide = static_cast<uint64_t>(n);
	uint64_t A_wide = static_cast<uint64_t>(A);
	uint64_t s_wide = static_cast<uint64_t>(s);

	if (s < 100)
	{
		if (n <= 1)
		{
			return 100 * n_wide;
		}
		else if (n <= A)
		{
			return 100 * (A_wide * n_wide) / (A_wide + (n_wide - 1) * s_wide / (100 * 2));
		}
		else if (n < A * 2)
		{
			return 100 * 100 * A_wide * n_wide / (s_wide * A_wide + 100 * n_wide - s_wide / 2 - n_wide * s_wide / 2);
		}
		else
		{
			kassert(n_wide >= A_wide * 2);
			return 100 * A_wide;
		}
	}
	else
	{
		kassert(s >= 100);
		uint64_t cutoff = A_wide + A_wide * s_wide / 100 - s_wide / 100;
		if (n_wide < cutoff)
		{
			return 100 * n_wide * A_wide * (s_wide + 100) / (s_wide * (n_wide + A_wide - 1) / 100 + A_wide) / 100;
		}
		else
		{
			return 100 * A_wide;
		}
	}
}
