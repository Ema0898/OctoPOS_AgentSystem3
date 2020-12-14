#ifndef OS_AGENT_AGENT_H
#define OS_AGENT_AGENT_H

#include <cstddef>
#include <utility>

#include "hw/hal/Tile.h"

#include "lib/kassert.h"
#include "lib/adt/BitmapObjectAllocator.h"
#include "os/rpc/RPCStub.h"

#include "lib/adt/AtomicID.h"
#include "lib/adt/Bitset.h"
#include "lib/debug.h"

#include "os/res/ProxyClaim.h"
#include "os/res/DispatchClaim.h"

#include "octo_types.h"

#include "os/agent3/AgentConstraint.h"

#include "os/agent3/AgentSystem.h"
#include "os/agent3/AgentClaim.h"

#include "os/ipc/SimpleSignal.h"

#include "os/agent3/AgentRPCHeader.h"

#include "os/agent3/Platform.h"

#include "os/dev/IOTile.h"

namespace os
{
	namespace agent
	{

		class AbstractConstraint;
		class AgentConstraint;
		class ActorConstraint;
		class OperatingPoint;

		enum invadestage_t
		{
			idle,
			regioncheck,
			fetchmonitors,
			bargain,
			octoinvade,
			reinvading
		};

		typedef DMA_ANSWER_ENVELOPE(AgentClaim) RPCAnswer_t;

		typedef struct
		{
			bool active;
			bool improvable;
			bool pending;
			invadestage_t stage;
			AgentConstraint *agent_constraints;
			ActorConstraint *actor_constraint;
			ConstraintSolver *solver; // agent constraint solver
			AgentClaim claim;
			//holds resources that are not used in the claim, but blockend anyway
			AgentClaim blockedResources;
			RPCAnswer_t RPCAnswerDesc;
			os::res::DispatchClaim dispatch_claim;
		} AgentRequest_s;

		ResourceRating downey_rate(uint16_t n, uint16_t A, uint16_t s);

		class AgentInstance
		{

			friend class ConstraintSolver;

		public:
			AgentInstance(bool isIdleAgent = false)
					: myAgentID(AgentSystem::registerAgent(this))
			{
				myResourcePool.reset(false);
				myReservations.reset(false);
				this->isIdleAgent = isIdleAgent;
				sticky_tiles.clearAll();

				os::agent::ResourceID resource;
				for (resource.tileID = 0; resource.tileID < hw::hal::Tile::getTileCount(); resource.tileID++)
				{
					for (resource.resourceID = 0; resource.resourceID < MAX_RES_PER_TILE; resource.resourceID++)
					{
						myResources[resource.tileID][resource.resourceID].ReservedForAgent = NULL;
						myResources[resource.tileID][resource.resourceID].ReservationRating = 0;
					}
				}

				for (int i = 0; i < os::agent::MAX_REQUESTS_PER_AGENT; i++)
				{
					myRequests[i].active = false;
				}

				strcpy(name, "");
			}

			AgentInstance(const char *agent_name, bool isIdleAgent = false) : AgentInstance(isIdleAgent)
			{
				strncpy(name, agent_name, maxNameLength - 1);
				name[maxNameLength - 1] = '\0';
			}

			~AgentInstance()
			{
				if (checkAlive())
				{
					panic("Killing Active Agent!");
				}
				if (!myResourcePool.isEmpty())
				{
					os::agent::ResourceID resource;
					for (resource.tileID = 0; resource.tileID < hw::hal::Tile::getTileCount(); resource.tileID++)
					{
						for (resource.resourceID = 0; resource.resourceID < MAX_RES_PER_TILE; resource.resourceID++)
						{
							if (myResourcePool.contains(resource))
							{
								this->transferResource(resource, os::agent::AgentSystem::idleAgent);
							}
						}
					}
				}
				if (!vipgBlockedPool.isEmpty())
				{
					os::agent::ResourceID resource;
					for (resource.tileID = 0; resource.tileID < hw::hal::Tile::getTileCount(); resource.tileID++)
					{
						for (resource.resourceID = 0; resource.resourceID < MAX_RES_PER_TILE; resource.resourceID++)
						{
							if (vipgBlockedPool.contains(resource))
							{
								this->transferResource(resource, os::agent::AgentSystem::idleAgent);
							}
						}
					}
				}
				if (!myReservations.isEmpty())
				{
					os::agent::ResourceID resource;
					for (resource.tileID = 0; resource.tileID < hw::hal::Tile::getTileCount(); resource.tileID++)
					{
						for (resource.resourceID = 0; resource.resourceID < MAX_RES_PER_TILE; resource.resourceID++)
						{
							if (myReservations.contains(resource))
							{
								/* A resource owner can be NULL following an unsetOwner() call. */
								os::agent::AgentInstance *owner = os::agent::AgentSystem::getOwner(resource);
								if (owner)
								{
									DBG(SUB_AGENT, "in AgentInstance destructor: unreserve((%d/%d), this(%p));\n", resource.tileID, resource.resourceID, this);
									owner->unreserve(resource, this);
								}
								else
								{
									DBG(SUB_AGENT, "WARNING: reservation list of agent %p contains resource (%d/%d), but resource has no owner. Agent system state potentially broken.\n", this, resource.tileID, resource.resourceID);
								}
							}
						}
					}
				}
				for (int i = 0; i < os::agent::MAX_REQUESTS_PER_AGENT; i++)
				{
					if (myRequests[i].active)
					{
						delete myRequests[i].solver;
						myRequests[i].solver = NULL;
						delete myRequests[i].agent_constraints;
						myRequests[i].agent_constraints = NULL;
						myRequests[i].active = false;
					}
				}

				AgentSystem::unregisterAgent(this, myAgentID);
			}

			void *operator new(size_t s)
			{
				return AgentMemory::agent_mem_allocate(s);
			}

			void operator delete(void *c, size_t s)
			{
				AgentMemory::agent_mem_free(c);
			}

			const char *get_name()
			{
				return this->name;
			}
			void set_name(const char *agent_name)
			{
				strncpy(name, agent_name, maxNameLength - 1);
				name[maxNameLength - 1] = '\0';
			}

			AgentClaim invade(AgentConstraint *constraints);

			// MultiStage Invade
			uint8_t invade_regioncheck(AbstractConstraint *constraints); // returns the Slot for the invade
			uint8_t invade_fetchmonitors(uint8_t slot, uint8_t oldSlot);

			/*
	 * Checks if the given constraints could theoretically be fulfillable
	 * with the current system resources.
	 *
	 * Please make sure that the system state is locked before calling this function!
	 * Otherwise this function's result is potentially meaningless.
	 *
	 * The return value is an std::pair of bool and AgentClaim. The bool will be set
	 * to true if the request is fulfillable, i.e., the constraints are
	 * satisfiable. Otherwise, it is set to false.
	 * The AgentClaim value will contain all usable resources according to the
	 * slot's constraints if the bool value is true. Otherwise, its content is
	 * undefined and must not be used.
	 *
	 * Note that in a success case, the AgentClaim might include *more* resources
	 * than necessary to satisfy the constraints and especially more resources then
	 * specified as the maximum PE count in the constraints. Whatever is processing
	 * the data must take care to select the resources appropriately.
	 */
			std::pair<bool, AgentClaim> invade_fulfillable(const uint8_t slot) const;

			uint8_t invade_bargain(uint8_t slot, uint8_t oldSlot);
			uint8_t invade_bargain_dcop(uint8_t slot, uint8_t oldSlot);
			uint8_t invade_bargain_new(uint8_t slot);
			uint8_t invade_bargain_old(uint8_t slot);

			/*
	 * Registers the given AgentOctoClaim with the AgentOctoClaim as specified by its claim ID.
	 *
	 * This is needed for malleability.
	 */
			void register_AgentOctoClaim(uint32_t claim_id, AgentOctoClaim &octo_claim, os::res::DispatchClaim dispatch_claim);

			void signalOctoPOSConfigDone(uint32_t claim_no)
			{
				// find the slot that belongs to this claim_no..
				uint8_t slot = get_slot_no_of_claim_no(claim_no);

				if (MAX_REQUESTS_PER_AGENT == slot)
				{
					return;
				}

				if (!myRequests[slot].active)
				{
					panic("Signal OctoPOS Config Done on invalid (inactive) slot!");
				}

				if (octoinvade != myRequests[slot].stage)
				{
					panic("Signal OctoPOS Config Done on slot in wrong stage!");
				}

				// okay, we received a valid OctoPOS Config signal.. now we can reveal to be the owner of the claim.. :)
				DBG(SUB_AGENT, "Making claim available for bargaining...\n");
				os::agent::AgentSystem::unlockBargaining(&myRequests[slot].claim);

				/*
		 * Needed for reinvades: if not all resources have been used,
		 * some are still in the resource pool, but in a locked state.
		 * Unlock them here.
		 */
				os::agent::AgentSystem::unlockBargaining(&myResourcePool);
			}

			AgentClaim getClaim(int request_no);

			bool checkAlive() const;

			/*
	 * Used in a reinvade-context to move unused, yet also unreserved (to either the current or other agents)
	 * to the resource pool.
	 * The main difference between this and pure_retreat() is, that it does not try to move resources that
	 * are reserved for other agents to the respective agent.
	 * Keeps the complete old claim to be able to restore it, if the reinvade fails.
	 */
			uint8_t reinvade_retreat(uint32_t claim_no, uint8_t newSlot);

			/*
	 * The "normal" retreat function.
	 * Handles resources reserved for other agents by either means of transfer (if useful for the other agent)
	 * or deletion of still active, but currently useless reservations.
	 * Eventually moves all resources of a claim into a resource pool, which also implicitly invalidates an
	 * entry in the private reservations list, as the resource pool is not considered when checking
	 * this reservations list.
	 */
			bool pure_retreat(uint32_t claim_no);

			/*
	 * This is essentially a wrapper around pure_retreat() - retreats all active claims.
	 * Only really useful if you want to be able to forcibly delete an agent.
	 * Mainly used by the DeleteAgent RPC.
	 */
			void fullRetreat();

			/* delete reservation of given resource */
			void unreserve(const ResourceID &resource, AgentInstance *whom)
			{
				if (myResources[resource.tileID][resource.resourceID].ReservedForAgent == whom)
				{
					DBG(SUB_AGENT_NOISY, "unreserve: current reservation %p matches to-be-unreserved\n", whom);
					myResources[resource.tileID][resource.resourceID].ReservedForAgent = NULL;
				}
				else
				{
					DBG(SUB_AGENT_NOISY, "unreserve: current reservation: %p, to be unreserved: %p\n", myResources[resource.tileID][resource.resourceID].ReservedForAgent, whom);
					panic("unreserve: AgentSystem Inconsistent: Should remove wrong reservation...");
				}
			}

			/* remove resource from my reservation list */
			void giveUpReservation(const ResourceID &resource)
			{
				if (myReservations.contains(resource))
					myReservations.remove(resource);
				else
					panic("giveUpReservation: AgentSystem Inconsistent: Should give up nonexisting reservation...");
			}

			/* returns a resource reserved for otherAgent
	 * or invalid resource if no such reservation is found.
	 */
			ResourceID holdReservationFor(AgentInstance *otherAgent)
			{
				ResourceID res;

				for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
				{
					for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; res.resourceID++)
					{
						if (myResources[res.tileID][res.resourceID].ReservedForAgent == otherAgent)
						{
							return res;
						}
					}
				}
				res.tileID = 0xFF; // marks it as "invalid"
				return res;
			}

			/*
	 * Returns true iff for a specific slot and tile:
	 *   - tile is usable according to the constraints
	 *
	 * AND
	 *
	 *   - tile sharing is turned on in the constraints and there is no other non-sharable claim
	 *     on the given tile (no matter which agent owns the non-sharable claim)
	 *   OR
	 *  - tile sharing is turned off and no other claim (no matter what agent) holds
	 *    a resource on the given tile.
	 *
	 * False otherwise.
	 */
			bool is_tile_usable(const uint8_t slot, const TileID tile_id) const;

			/*
	 * Return true iff for a specific tile there is an active claim/request.
	 * False otherwise.
	 */
			bool get_has_active_claim(TileID tile_id) const;

			bool getIsTileShareable(TileID tileID) const
			{
				for (std::size_t slot = 0; slot < os::agent::MAX_REQUESTS_PER_AGENT; ++slot)
				{
					if (!myRequests[slot].active)
						continue;

					/*
			 * Only looking at the first active request is enough.
			 * There can either be one active claim with tile sharing turned off,
			 * (x)or at least one active claim with tile sharing turned on.
			 */
					if (myRequests[slot].claim.containsTile(tileID))
					{
						return myRequests[slot].agent_constraints->isTileShareable();
					}
				}

				/* No active slots - the first application to grab the tile will define shareability. */
				return (true);
			}

			/* Return a rating which determines how valuable a resource is for this agent. */
			ResourceRating getRating(const ResourceID &resource, AgentInstance *otherAgent = NULL) const
			{
				// basically what we do is: See how many resources we have and look up the speedup curve.
				// TODO otherAgent is not used. Remove or use?

				/*
		 * Rate zero if we are the idle agent or the resource is contained in
		 * the resource pool to get rid of it.
		 */
				if ((this->isIdleAgent) || (myResourcePool.contains(resource)))
				{
					DBG(SUB_AGENT_NOISY, "getRating: we (%p) are the idle agent or resource (%d/%d) is contained in pool - rate 0 to get rid of it.\n", this, resource.tileID, resource.resourceID);
					return 0;
				}

				/* first count resources in my pool */
				int myResCount = myResourcePool.getResourceCount();

				/* add resources requested and used by my application */
				bool hasRes = false;
				uint8_t slot;
				bool agent_has_request = false;
				for (slot = 0; slot < os::agent::MAX_REQUESTS_PER_AGENT; ++slot)
				{
					if (!myRequests[slot].active)
					{
						continue;
					}

					agent_has_request = true;

					if (myRequests[slot].claim.contains(resource))
					{
						hasRes = true;
					}

					myResCount += myRequests[slot].claim.getResourceCount();
				}

				// <qznc> Ein Agent ohne request ist sowas wie ein Zombie Prozess unter Linux denke ich
				// <qznc> Eigentlich weg, aber noch nicht ganz
				// <qznc> Sollte eigentlich nicht auftreten, aber 0 zurückgeben klingt ok.
				if (!agent_has_request)
				{
					return 0;
				}

				/* add resources currently owned by other agents but reserved for me */
				myResCount += myReservations.getResourceCount();

				/* subtract resources currently owned by me but reserved for other agents */
				os::agent::ResourceID res;
				for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
				{
					for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
					{
						if (myResources[res.tileID][res.resourceID].ReservedForAgent == NULL)
						{
							continue;
						}

						if (myResources[res.tileID][res.resourceID].ReservedForAgent == this)
						{
							continue;
						}

						--myResCount;
					}
				}

				if (!hasRes)
				{
					/* for the rating assume we would get the resource */
					myResCount += 1;
				}

				/* check scalability curve for rating */
				for (slot = 0; slot < os::agent::MAX_REQUESTS_PER_AGENT; ++slot)
				{
					if (myRequests[slot].active)
					{
						break;
					}
				}

				int downeyA = myRequests[slot].agent_constraints->getDowneyA();
				int downeySigma = myRequests[slot].agent_constraints->getDowneySigma();
				ResourceRating x = myRequests[slot].agent_constraints->downey_rate(myResCount, downeyA, downeySigma);
				ResourceRating y = myRequests[slot].agent_constraints->downey_rate(myResCount - 1, downeyA, downeySigma);
				ResourceRating rating = x - y;

				return rating;
			}

			int getSlot(const ResourceID &resource) const;
			AgentClaim *getClaim(const ResourceID &resource);
			const AgentClaim *getClaim(const ResourceID &resource) const;
			AbstractConstraint *getConstr(const ResourceID &resource);
			const AbstractConstraint *getConstr(const ResourceID &resource) const;

			void gainReservedResource(ResourceID &resource)
			{
				DBG(SUB_AGENT, "Agent %p gaining reserved Resource %d/%d\n", this, resource.tileID, resource.resourceID);
				AgentSystem::setOwner(resource, this);
				myReservations.remove(resource);
				myResources[resource.tileID][resource.resourceID].ReservationRating = 0;
				myResources[resource.tileID][resource.resourceID].ReservedForAgent = NULL;
				myResourcePool.add(resource); // store it in our resource pool for future use or bargaining
			}

			void gainResource(const ResourceID &resource, const bool isAgentConstraint = true, const uint8_t slot = 0)
			{
				AgentSystem::setOwner(resource, this);

				if (isAgentConstraint)
				{
					/* for all my pending requests, select the one that makes the best use of that resource */
					uint8_t best = MAX_REQUESTS_PER_AGENT;
					ResourceRating bestrating = 0;
					for (uint8_t req = 0; req < MAX_REQUESTS_PER_AGENT; ++req)
					{
						if (!myRequests[req].active)
						{
							continue;
						}

						if (idle == myRequests[req].stage)
						{
							continue;
						}

						if (!myRequests[req].pending)
						{
							continue;
						}

						if (myRequests[req].agent_constraints)
						{
							DBG(SUB_AGENT, "agent %p gainResource (%d/%d) try slot %d\n", this, resource.tileID, resource.resourceID, req);
							ResourceRating rating = myRequests[req].agent_constraints->rateAdditionalResource(myRequests[req].claim, resource);
							if (rating > bestrating)
							{
								bestrating = rating;
								best = req;
							}
						}
					}

					if (MAX_REQUESTS_PER_AGENT == best)
					{
						/* No request really requires the resource.
				 * Look for a (improvable) request which could at least use it. */
						for (uint8_t req = 0; req < MAX_REQUESTS_PER_AGENT; ++req)
						{
							if (!myRequests[req].active)
							{
								continue;
							}

							if (idle == myRequests[req].stage)
							{
								continue;
							}

							if (!myRequests[req].improvable)
							{
								continue;
							}

							if (myRequests[req].agent_constraints)
							{
								DBG(SUB_AGENT, "agent %p gainResource (%d/%d) try to improve slot %d\n", this, resource.tileID, resource.resourceID, req);
								ResourceRating rating = myRequests[req].agent_constraints->rateAdditionalResource(myRequests[req].claim, resource);
								if (rating > bestrating)
								{
									bestrating = rating;
									best = req;
								}
							}
						}
					}

					DBG(SUB_AGENT_NOISY, "gainResource: setting ReservedForAgent to NULL\n");
					myResources[resource.tileID][resource.resourceID].ReservationRating = 0;
					myResources[resource.tileID][resource.resourceID].ReservedForAgent = NULL;

					if (MAX_REQUESTS_PER_AGENT == best)
					{
						/* currently we can not use the resource at all. */
						DBG(SUB_AGENT, "Currently agent %p cannot use (%d/%d); pool!\n", this, resource.tileID, resource.resourceID);
						myResourcePool.add(resource);
					}
					else
					{
						/* give resource to best claim */
						DBG(SUB_AGENT, "Agent %p got (%d/%d) in slot %d\n", this, resource.tileID, resource.resourceID, best);
						myRequests[best].claim.add(resource);
						sticky_tiles.set(resource.tileID);
						if (myRequests[best].agent_constraints->fulfilledBy(myRequests[best].claim, &(myRequests[best].improvable)))
						{
							myRequests[best].pending = false;
							// TODO: Signal application about finished invade request
						}
					}
				}
				else
				{
					if ((!myRequests[slot].active) || (idle == myRequests[slot].stage) || (!myRequests[slot].pending))
					{
						DBG(SUB_AGENT_NOISY, "gainResource: setting ReservedForAgent to NULL\n");
						myResources[resource.tileID][resource.resourceID].ReservationRating = 0;
						myResources[resource.tileID][resource.resourceID].ReservedForAgent = NULL;
						/* currently we can not use the resource at all. */
						DBG(SUB_AGENT, "Currently agent %p cannot use (%d/%d); pool!\n", this, resource.tileID, resource.resourceID);
						myResourcePool.add(resource);
					}

					else
					{
						/* give resource to best claim */
						DBG(SUB_AGENT, "Agent %p got (%d/%d) in slot %d\n", this, resource.tileID, resource.resourceID, slot);
						myRequests[slot].claim.add(resource);
						sticky_tiles.set(resource.tileID);
						//myRequests[slot].pending = false; // TO MOVE
						// TODO: Signal application about finished invade request
					}
				}
			}

			/* take resource from this agent and give to other agent */
			void transferResource(ResourceID &resource, AgentInstance *other)
			{
				DBG(SUB_AGENT, "Give %d/%d from %p to %p\n", resource.tileID, resource.resourceID, this, other);
				this->loseResource(resource, NULL, 0);
				other->gainResource(resource);
			}

			void yieldResources(int slot, AgentClaim &claim)
			{
				AgentRequest_s *request = &myRequests[slot];

				ResourceID res;
				for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
				{
					if (!claim.containsTile(res.tileID))
					{
						continue;
					}

					for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; res.resourceID++)
					{
						if (!claim.contains(res))
						{
							continue;
						}

						if (request->claim.contains(res))
						{
							request->claim.remove(res);
							AgentSystem::unsetOwner(res, this);
						}
						else if (request->blockedResources.contains(res))
						{
							request->blockedResources.remove(res);
							AgentSystem::unsetOwner(res, this);
						}
						else
						{
							DBG(SUB_AGENT, "I was told to yield resource %d/%d in slot %d, but its not there..\n",
									res.tileID, res.resourceID, slot);
						}
					}
				}
			}

			enum loseState_t
			{
				RESOURCE_AVAILABLE,
				NOT_RESERVED,
				RESERVED
			};

			loseState_t loseResource(const ResourceID &resource, AgentInstance *otherAgent, ResourceRating bestrating, const bool isAgentConstraint = true)
			{
				kassert(!stayCauseSticky(resource));
				if (myResourcePool.contains(resource))
				{
					myResourcePool.remove(resource);
					AgentSystem::unsetOwner(resource, this);
					return (RESOURCE_AVAILABLE);
				}

				if (bestrating && otherAgent)
				{
					// if we already have reserved something _FROM_ otherAgent, it would not make too much sense to reserve something for him..
					// but how to express that without using expensive data structures...
					AgentInstance *reserved_owner = myResources[resource.tileID][resource.resourceID].ReservedForAgent;
					ResourceRating reserved_rating = myResources[resource.tileID][resource.resourceID].ReservationRating;
					if (reserved_owner != NULL)
					{
						if (isAgentConstraint && (reserved_owner == this || reserved_rating < reserved_owner->getRating(resource, this)))
						{
							return (NOT_RESERVED);
						}
						else if (!isAgentConstraint && (reserved_owner == this))
						{
							return (NOT_RESERVED);
						}
						else
						{
							DBG(SUB_AGENT, "IT IS HERE! I AM %p, reserved for %p (Rating %" PRIu64 ")\n", this, reserved_owner, reserved_rating);
							reserved_owner->giveUpReservation(resource);
						}
					}
					myResources[resource.tileID][resource.resourceID].ReservationRating = bestrating;
					myResources[resource.tileID][resource.resourceID].ReservedForAgent = otherAgent;
					return (RESERVED);
				}

				return (NOT_RESERVED);
			}

			AgentRequest_s *getSlot(int slot)
			{
				return &myRequests[slot];
			}

			char getId() const
			{
				return 'm';
			}

			bool stayCauseSticky(const os::agent::ResourceID &res, const os::agent::AgentClaim *pseudo_claim = NULL) const;
			static constexpr size_t maxNameLength = 80;

			bool claimOwnership(ResourceID &res, int slot)
			{
				AgentInstance *oldOwner = AgentSystem::getOwner(res);
				if (oldOwner == this)
				{
					if (myRequests[slot].claim.contains(res) || myRequests[slot].blockedResources.contains(res) || myResourcePool.contains(res))
					{
						return true;
					}
				}
				if (oldOwner && oldOwner->loseResource(res, NULL, 0) != RESOURCE_AVAILABLE)
				{
					DBG(SUB_AGENT, "not getting resource_available\n");
					return false;
				}
				/*
		 * We become the owner when there either is no owner (which should not happen)
		 * or when the call to loseResource returned successfully
		 */
				AgentSystem::setOwner(res, this); //if it fails, it crashes
				return true;
			}

			bool blockResource(ResourceID &res, const int slot)
			{
				if (claimOwnership(res, slot))
				{
					myRequests[slot].blockedResources.add(res);
					return true;
				}
				return false;
			}

			/**
     * \brief Creates all MalleabilityClaims for the agent.
     *
     * Iterates over all active slots and saves the respective AgentMalleabilityClaim into
     * the given number, if malleability is set for the slot.
     * If this is called on the callingAgent, the given slot will be ignored.
     *
     * \param end marks the end of the given buffer. If this is reached, the function stops
     * generating new claims.
     *
     * Returns the number of MalleabilityClaims that where added to the buffer.
     */
			uint8_t getMalleabilityClaims(AgentMalleabilityClaim **buffer, AgentMalleabilityClaim **end,
																		AgentInstance *callingAgent, int slot);

			ResourceRating costOfLosingResource(const int slot, const ResourceID &res)
			{
				if (!myRequests[slot].claim.contains(res))
				{
					return 0;
				}
				else
				{
					myRequests[slot].claim.remove(res);
					ResourceRating newRating = myRequests[slot].solver->getRating() - myRequests[slot].agent_constraints->rateClaim(myRequests[slot].claim);
					return newRating;
				}
			}

			bool callResizeHandlers(int slot);
			bool claimResources(int slot);

			static void debugPrintResourceDistribution()
			{
				DBG_RAW(SUB_AGENT, "Resources distribution (tile-resource:agent) idleAgent: %p, initialAgent: %p\n", AgentSystem::idleAgent, AgentSystem::initialAgent);
				for (uint8_t tileIdx = 0; tileIdx < hw::hal::Tile::MAX_TILE_COUNT; ++tileIdx)
				{
					DBG_RAW(SUB_AGENT, "Resources of tile %" PRIu8 ":\t", tileIdx);
					for (uint8_t resIdx = 0; resIdx < os::agent::MAX_RES_PER_TILE; ++resIdx)
					{
						DBG_RAW(SUB_AGENT, "(%d-%d:%p) ", tileIdx, resIdx, (void *)AgentSystem::systemResources[tileIdx][resIdx].responsibleAgent);
					}
					DBG_RAW(SUB_AGENT, "\n");
				}

				AgentInstance **instances = AgentSystem::getAgentArray();
				DBG_RAW(SUB_AGENT, "resource pools of agents: \n");
				for (uint8_t agentIdx = 0; agentIdx < MAX_AGENTS; ++agentIdx)
				{
					AgentInstance *agent = instances[agentIdx];
					if (!agent || agent == AgentSystem::idleAgent || agent == AgentSystem::initialAgent)
					{
						continue;
					}
					DBG_RAW(SUB_AGENT, "agent: %p\n", (void *)agent);
					agent->myResourcePool.print();
				}
			}

		private:
			bool isIdleAgent;
			char name[maxNameLength];

			const uint8_t myAgentID;

			AgentRequest_s myRequests[MAX_REQUESTS_PER_AGENT];

			/* Since we cannot retreat a tile completely during reinvade,
	 * we must remember which tiles we invaded.
	 * Note that the meaning is very different from the AgentConstraint.tiles Bitset,
	 * which set 'possible' tiles. This bitset sets 'necessary' tiles. */
			lib::adt::Bitset<16> sticky_tiles;

			uint8_t coreCount;

			struct
			{
				AgentInstance *ReservedForAgent;
				ResourceRating ReservationRating;
			} myResources[hw::hal::Tile::MAX_TILE_COUNT][os::agent::MAX_RES_PER_TILE];

			AgentClaim myResourcePool;
			AgentClaim myReservations;
			AgentClaim vipgBlockedPool; // a pool of resources that must not be released to other agents. Used to enhance the Energy-Delay of the system.

			bool check_sticky_invariant(uint8_t slot) const;

			uint8_t get_slot_no_of_claim_no(uint32_t claim_no) const;

			/*
	 * Moves usable resources from the resource pool to the given request slot.
	 *
	 * Returns an AgentClaim containing all transfered resources.
	 */
			os::agent::AgentClaim fetch_resources_from_resource_pool(const uint8_t slot);

			void fillPoolWithMyFreeResources(AgentClaim &pool);

			/*
	 * Moves resources included in pseudo_claim to the given request slot.
	 *
	 * The optional skiplist parameter can be used to provide a pointer to an AgentClaim
	 * denoting resources that shall be skipped, even if included in pseudo_claim.
	 * This is mostly interesting if a subset of resources have already been moved
	 * into the request slot previously.
	 *
	 * If the optional malleability parameter is set to true, the algorithm is changed
	 * into "malleable mode".
	 *
	 * Returns true if the agent_constraints in the request slot are fulfilled afterwards,
	 * false otherwise.
	 */
			bool move_resources_to_request(const uint8_t slot, const AgentClaim &pseudo_claim, const AgentClaim *skiplist = NULL, const bool malleability = false);

			/*
	 * Tries to transfer the given resource to the current agent.
	 * The resource will automatically be added to the first active and pending
	 * or improvable slot.
	 * Note that during invade_bargain, at least one slot is pending.
	 *
	 * Returns true iff the resource was successfully moved to the current agent,
	 * false otherwise.
	 *
	 * Reasons for failure can be: the owner's rating for this resource is better
	 * than the current agent's rating, the owner is currently using this resource
	 * or agent system inconsistencies (e.g., resource marked as reserved, but no
	 * such reservation actually exists.)
	 */
			bool invade_bargain_transfer_resource(const ResourceID &res, const bool isAgentConstraint = true, const uint8_t slot = 0);

			/*
	 * Examines the current agent system (global state) to find malleable resources.
	 *
	 * The return value should include all malleable resource candidates.
	 */
			AgentClaim invade_bargain_get_malleable_candidates(const uint8_t slot) const;

			/*
	 * For a given array of AgentInstance pointers of specific size, returns the index
	 * of the provided element or -1 if the element is not included.
	 */
			std::ptrdiff_t get_agents_element_index(const AgentInstance *const *arr, const AgentInstance *elem, const std::size_t arr_size) const;

			/*
	 * For a given claim, fetches the claims amount of all resources belonging
	 * to the provided agent.
	 */
			std::size_t get_real_claims_count_for_agent(const AgentInstance &agent, const AgentClaim &claim) const;

			/*
	 * For a given claim, writes "real" claim pointers as used in the agents to the
	 * provided claims array for every agent specified in the agents array of
	 * given size. The claims array must be bounded by the max_claims_count
	 * parameter.
	 *
	 * Additionally, the claims_count array is also populated.
	 *
	 * WARNING: the claims parameter is expected to be and will be converted to
	 * a two-dimensional array of AgentClaim pointers.
	 *
	 * This function should probably belong to os::agent::AgentSystem.
	 */
			void get_real_claims_for_agents(const AgentInstance *const *agents, const std::size_t agents_count, const AgentClaim &claim, const std::size_t max_claims_count, AgentClaim **claims, std::size_t *claims_count);

			/*
	 * Takes a claim reference containing all resources to be forcefully removed from
	 * malleable claims.
	 *
	 * The resize handlers of all affected claims are called before actually removing
	 * the resources.
	 *
	 * The given claim must not be empty.
	 */
			void remove_malleable_resources(const AgentClaim &claim);

			/*
	 * Takes a claim reference containing all resources to be forcefully transfered
	 * to the current agent.
	 *
	 * The claim must have been "prepared" with remove_malleable_resources() prior
	 * to using this function!
	 *
	 * The given claim must not be empty.
	 */
			void transfer_malleable_resources(const uint8_t slot, const AgentClaim &claim);

			/*
	 * For a specific resource, checks if it's part of an active request slot.
	 *
	 * Returns true if the specified resource is part of one of this agent's active request slots,
	 * false otherwise.
	 */
			bool is_part_of_active_request(const ResourceID &resource) const;

			/**
     * \brief Creates a MalleabilityClaim for the given slot.
     *
     * If malleability is not set in the slot's constraints, NULL is returned.
     * Otherwise, this method returns an AgentMalleabilityClaim based on the slot's AgentClaim:
     * Resources, that due to some Constraint inside this hierarchy must not be removed from
     * the claim are excluded from the AgentMalleabilityClaim
     */
			AgentMalleabilityClaim *buildMalleabilityClaim(int slot);
		};

	} // Namespace agent
} // Namespace os

inline bool os::agent::AgentInstance::checkAlive() const
{
	uint8_t slot = 0;
	while (slot < MAX_REQUESTS_PER_AGENT)
	{
		if (myRequests[slot].active)
		{
			return true;
		}
		slot++;
	}
	return false;
}

BLOCK_RPC_FUNC(AgentSystemGetDispatchClaim, os::res::DispatchClaim)
{
	return os::agent::AgentSystem::getDispatchClaim();
}

inline os::res::DispatchClaim os::agent::AgentSystem::getDispatchClaim()
{
	if (AGENT_DISPATCHCLAIM.getTID() == 255)
	{
		if (hw::hal::Tile::existsIOTile())
		{
			AGENT_DISPATCHCLAIM = os::dev::IOTile::Inst().getDispatchClaim();
		}
		else if (hw::hal::Tile::getTileID() != os::agent::AgentSystem::AGENT_TILE)
		{
			AgentSystemGetDispatchClaimRPC::FType f;
			AgentSystemGetDispatchClaimRPC::stub(AGENT_TILE, &f);
			f.force();
			AGENT_DISPATCHCLAIM = f.getData();
		}
		else
		{
			panic("os::agent::AgentSystem::getDispatchClaim:: This should never happen\n");
		}
	}

	return AGENT_DISPATCHCLAIM;
}

// =========== Agent System ===========

extern "C" uintptr_t os_agent_agentsystem_no_init;

inline uint8_t os::agent::AgentSystem::getNextAgentID()
{
	uint8_t retVal = nextAgent;
	if (instances[retVal] != NULL)
	{ //this should not happen
		DBG(SUB_AGENT, "getNextAgentID: what shouldn't happen, happened\n");
		do
		{
			retVal = (retVal + 1) % MAX_AGENTS;
			if (instances[retVal] == NULL)
			{
				break;
			}
		} while (retVal != nextAgent); //loop only once

		if (instances[retVal] != NULL)
		{ //no empty spot available
			return (uint8_t)-1;
		}
		else
		{
			nextAgent = retVal;
		}
	}

	//find next free spot, but only loop once
	do
	{
		nextAgent = (nextAgent + 1) % MAX_AGENTS;
	} while (instances[nextAgent] != NULL && nextAgent != retVal);

	return retVal;
}

inline uint8_t os::agent::AgentSystem::registerAgent(os::agent::AgentInstance *agent)
{
	if (os::agent::AgentSystem::agentCount == os::agent::MAX_AGENTS)
	{
		DBG(SUB_AGENT, "MAXIMUM AGENTS REACHED! Can't register agent %p\n", agent);
		panic("Agent limit exceeded\n");
		return (uint8_t)-1;
	}

	uint8_t id = os::agent::AgentSystem::getNextAgentID();
	os::agent::AgentSystem::agentCount++;
	os::agent::AgentSystem::instances[id] = agent;
	DBG(SUB_AGENT, "New agent registered with id %d\n", id);
	return id;
}

inline void os::agent::AgentSystem::unregisterAgent(os::agent::AgentInstance *agent, const uint8_t id)
{
	if (os::agent::AgentSystem::instances[id] != agent)
	{
		return;
	}

	os::agent::AgentSystem::instances[id] = NULL;
	os::agent::AgentSystem::agentCount--;
	os::agent::AgentSystem::nextAgent = id;
}

inline void os::agent::AgentSystem::unsetOwner(const os::agent::ResourceID &resource, const os::agent::AgentInstance *ag)
{
	// TODO: GET LOCK
	if (systemResources[resource.tileID][resource.resourceID].responsibleAgent != ag)
	{
		panic("Agent System Inconsistent unsetOwner\n");
	}
	systemResources[resource.tileID][resource.resourceID].responsibleAgent = NULL;
	// TODO: RELEASE LOCK
}

inline void os::agent::AgentSystem::setOwner(const os::agent::ResourceID &resource, os::agent::AgentInstance *ag)
{
	// TODO: GET LOCK
	if (systemResources[resource.tileID][resource.resourceID].responsibleAgent != NULL)
	{
		DBG(SUB_AGENT, "Owner is %p should %p\n", systemResources[resource.tileID][resource.resourceID].responsibleAgent, ag);
		panic("Agent System Inconsistent setOwner\n");
	}
	systemResources[resource.tileID][resource.resourceID].responsibleAgent = ag;
	// TODO: RELEASE LOCK
}

inline os::agent::AgentInstance *os::agent::AgentSystem::getOwner(const os::agent::ResourceID &resource)
{
	AgentInstance *owner = NULL;
	// TODO: GET LOCK
	owner = systemResources[resource.tileID][resource.resourceID].responsibleAgent;
	// TODO: RELEASE LOCK
	return owner;
}

#endif
