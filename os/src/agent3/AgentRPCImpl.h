#include "os/agent3/Agent.h"
#include "os/agent3/AgentConstraint.h"
#include "os/agent3/ActorConstraint.h"
#include "os/agent3/AgentSystem.h"
#include "os/agent3/AgentRPCHeader.h"
#include "os/rpc/RPCStub.h"
#include "octo_agent3.h"

namespace os
{
	namespace agent
	{

		DEFINE_RPC(NO_ANSWER_RPC_FUNC, AGENT_SETLEDS_RPC)
		{
			DBG(SUB_AGENT, "SetLEDsRPC %" PRIx32 "\n", bits);
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_INVADE_RPC)
		{

			DBG(SUB_AGENT, "MAN_ANSWER_BLOCK_RPC_FUNC %d\n", getReturnEnvelope().getParent().getTID());
			DBG(SUB_AGENT, "RPCImpl: called InvadeAgent.\n");

			os::agent::AgentConstraint *constrHierarchie;
			flatConstraints.unflatten(&constrHierarchie); //don't unflatten inplace

			TID src = getParent().getTID();

			AgentSystem::enterAgent(src);
			uint8_t slot = Agent->invade_regioncheck(constrHierarchie);
			AgentSystem::leaveAgent(src);
			if (slot == os::agent::MAX_REQUESTS_PER_AGENT)
			{
				// the agent is 'fully booked'. We need to abort this invade..
				os::agent::AgentClaim retVal(false); // sets ucid to INVALID_CLAIM
				retVal.setOwningAgent(Agent);
				getReturnEnvelope().sendReply(retVal);
			}
			else
			{
				Agent->getSlot(slot)->RPCAnswerDesc = getReturnEnvelope();

				Agent->invade_fetchmonitors(slot, INVALID_REQUEST_SLOT);
			}

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_REINVADE_RPC)
		{

			DBG(SUB_AGENT, "MAN_ANSWER_BLOCK_RPC_FUNC %d\n", getReturnEnvelope().getParent().getTID());
			DBG(SUB_AGENT, "RPCImpl: called ReinvadeAgent.\n");

			os::agent::AgentConstraint *constrHierarchie;
			flatConstraints.unflatten(&constrHierarchie); //don't unflatten inplace

			TID src = getParent().getTID();

			AgentSystem::enterAgent(src);

			uint8_t slot = Agent->invade_regioncheck(constrHierarchie);

			if (slot == os::agent::MAX_REQUESTS_PER_AGENT)
			{
				AgentSystem::leaveAgent(src);
				// the agent is 'fully booked'. We need to abort this invade..
				os::agent::AgentClaim retVal(false); // sets ucid to INVALID_CLAIM
				retVal.setOwningAgent(Agent);
				getReturnEnvelope().sendReply(retVal);
				return;
			}

			DBG(SUB_AGENT, "RPCImpl: called ReinvadeRetreatAgent.\n");
			uint8_t oldSlot = Agent->reinvade_retreat(old_claim_id, slot);
			AgentSystem::leaveAgent(src);

			Agent->getSlot(slot)->RPCAnswerDesc = getReturnEnvelope();

			Agent->invade_fetchmonitors(slot, oldSlot);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, ACTOR_INVADE_RPC)
		{

			DBG(SUB_AGENT, "MAN_ANSWER_BLOCK_RPC_FUNC %d\n", getReturnEnvelope().getParent().getTID());
			os::agent::OperatingPoint *operating_points_hierarchy = new OperatingPoint[AC_MAX_NUMBER_OF_OPERATING_POINTS];
			flat_operating_points.unflatten(&operating_points_hierarchy, AC_MAX_NUMBER_OF_OPERATING_POINTS);
			rpc_actor_constraints.set_operating_point_list(operating_points_hierarchy);

			os::agent::ClusterGuarantee *cluster_guarantees_hierarchy = new ClusterGuarantee[AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES];
			flat_cluster_guarantees.unflatten(&cluster_guarantees_hierarchy, AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES);
			rpc_actor_constraints.set_cluster_guarantee_list(cluster_guarantees_hierarchy);

			os::agent::Cluster *clusters_hierarchy = new Cluster[AC_MAX_NUMBER_OF_CLUSTERS];
			flat_clusters.unflatten(&clusters_hierarchy, AC_MAX_NUMBER_OF_CLUSTERS);
			rpc_actor_constraints.set_cluster_list(clusters_hierarchy);

			TID src = getParent().getTID();

			AgentSystem::enterAgent(src);
			uint8_t slot = Agent->invade_regioncheck(&rpc_actor_constraints);
			AgentSystem::leaveAgent(src);

			if (slot == os::agent::MAX_REQUESTS_PER_AGENT)
			{
				// the agent is 'fully booked'. We need to abort this invade..
				os::agent::AgentClaim retVal(false); // sets ucid to INVALID_CLAIM
				retVal.setOwningAgent(Agent);
				getReturnEnvelope().sendReply(retVal);
			}
			else
			{
				Agent->getSlot(slot)->RPCAnswerDesc = getReturnEnvelope();
				Agent->invade_fetchmonitors(slot, INVALID_REQUEST_SLOT);
			}

			return;
		}

		DEFINE_RPC(NO_ANSWER_RPC_FUNC, AGENT_REGISTER_AGENTOCTOCLAIM_RPC)
		{
			DBG(SUB_AGENT, "RegisterAgentOctoClaimRPC %" PRIu32 ", %p, %p\n", claim_id, octo_claim_ptr, &dispatch_claim);

			TID src = getParentTID();
			AgentSystem::enterAgent(src);
			Agent->register_AgentOctoClaim(claim_id, *octo_claim_ptr, dispatch_claim);
			AgentSystem::leaveAgent(src);
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, CLAIM_RUN_RESIZE_HANDLER_RPC)
		{
			DBG(SUB_AGENT, "RunResizeHandlerRPC %p, %p, %" PRIuMAX ", %" PRIuMAX ", %p\n", &loss_claim, resize_handler, static_cast<uintmax_t>(tile_count), static_cast<uintmax_t>(res_per_tile), resize_env_pointer);

			uint8_t gain[tile_count][res_per_tile];
			uint8_t loss[tile_count][res_per_tile];

			/* We may not initialize variable-sized arrays directly, so do it manually. */
			for (std::size_t i = 0; i < tile_count; ++i)
			{
				for (std::size_t y = 0; y < res_per_tile; ++y)
				{
					gain[i][y] = loss[i][y] = 0;
				}
			}

			os::agent::ResourceID res;
			for (res.tileID = 0; res.tileID < tile_count; ++res.tileID)
			{
				for (res.resourceID = 0; res.resourceID < res_per_tile; ++res.resourceID)
				{
					if (loss_claim.contains(res))
					{
						loss[res.tileID][res.resourceID] = 1;
					}
				}
			}

			/* No locking - agent system MUST already be locked. */
			/*
	[VW] possible reason for bug #30 (resize handler claim invalid)
	need to return an AbstractAgentOctoClaim rather than an AgentClaim
	*/
			resize_handler(&loss_claim, tile_count, res_per_tile, gain, loss, resize_env_pointer);
			//resize_handler(loss_claim.getAgentOctoClaim(), tile_count, res_per_tile, gain, loss, resize_env_pointer);

			/* Dummy return value, not actually used. */
			getReturnEnvelope().sendReply(NULL);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, CLAIM_UPDATE_CLAIM_STRUCTURES_RPC)
		{
			DBG(SUB_AGENT, "RunResizeHandlerRPC %p, %p\n", octo_claim_ptr, &claim);

			/* No locking - agent system MUST already be locked. */

			/* Copy modified AgentClaim - old data will be invalid after the function terminates. */
			AgentClaim *new_claim = new AgentClaim();
			(*new_claim) = claim;

			/* Adapt AgentOctoClaim to new claim. The _finish function will delete its original AgentClaim copy! */
			auto ret = octo_claim_ptr->adaptToAgentClaim_prepare(*new_claim);
			octo_claim_ptr->adaptToAgentClaim_finish(*new_claim, ret);

			/* Dummy return value, not actually used. */
			getReturnEnvelope().sendReply(NULL);

			return;
		}

		DEFINE_RPC(BLOCK_RPC_FUNC, AGENT_ALLOCATE_RPC)
		{
			DBG(SUB_AGENT, "RPCImpl: creating new agent.\r\n");
			TID src = getParent().getTID();
			AgentSystem::enterAgent(src);
			AgentInstance *retVal = new AgentInstance();
			AgentSystem::leaveAgent(src);
			return retVal;
		}

		DEFINE_RPC(BLOCK_RPC_FUNC, AGENT_GET_NAME)
		{
			TID src = getParent().getTID();
			int success;
			const char *name;
			AgentSystem::enterAgent(src);
			name = ag->get_name();
			if (strlen(name) >= size)
				success = -1;
			else
				success = 0;
			for (uint32_t i = 0; i <= strlen(name) && i < size; ++i)
			{
				buffer[i] = name[i];
			}
			AgentSystem::leaveAgent(src);
			if (success != 0)
				buffer[size - 1] = '\0';
			return success;
		}

		DEFINE_RPC(NO_ANSWER_RPC_FUNC, AGENT_SET_NAME)
		{
			TID src = getParentTID(); //getParent().getTID();
			AgentSystem::enterAgent(src);
			ag->set_name(agent_name);
			AgentSystem::leaveAgent(src);
		}

		DEFINE_RPC(BLOCK_RPC_FUNC, AGENT_DELETE_RPC)
		{
			DBG(SUB_AGENT, "RPCImpl: deleting agent.\n");
			TID src = getParent().getTID();
			AgentSystem::enterAgent(src);
			if (force)
			{
				Agent->fullRetreat();
			}
			delete Agent;
			AgentSystem::leaveAgent(src);
			return true;
		}

		DEFINE_RPC(NO_ANSWER_RPC_FUNC, AGENT_SIGNAL_CONFIG_DONE_RPC)
		{
			DBG(SUB_AGENT, "RPCImpl: called SignalOctoPOSConfigDone.\n");
			TID src = getParentTID();
			AgentSystem::enterAgent(src);
			Agent->signalOctoPOSConfigDone(claimNr);
			AgentSystem::leaveAgent(src);
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_GETINITIALCLAIM_RPC)
		{
			DBG(SUB_AGENT, "RPCImpl: called GetInitialAgentClaim.\n");
			DBG(SUB_AGENT, "MAN_ANSWER_BLOCK_RPC_FUNC %d\n", getReturnEnvelope().getParent().getTID());
			TID src = getParent().getTID();
			AgentSystem::enterAgent(src);
			AgentClaim retVal = reinterpret_cast<AgentInstance *>(os::agent::AgentSystem::initialAgent)->getClaim(0);
			getReturnEnvelope().sendReply(retVal);
			AgentSystem::leaveAgent(src);
			return;
		}

		DEFINE_RPC(BLOCK_RPC_FUNC, AGENT_CHECKALIVE_RPC)
		{
			DBG(SUB_AGENT, "RPCImpl: called CheckAgentAlive.\n");
			TID src = getParent().getTID();
			AgentSystem::enterAgent(src);
			bool retVal = Agent->checkAlive();
			AgentSystem::leaveAgent(src);
			return retVal;
		}

		DEFINE_RPC(BLOCK_RPC_FUNC, AGENT_PURE_RETREAT_RPC)
		{
			DBG(SUB_AGENT, "RPCImpl: called PureRetreatAgent\n");
			TID src = getParent().getTID();

			AgentSystem::enterAgent(src);
			bool ret = Agent->pure_retreat(claim_no);
			AgentSystem::leaveAgent(src);

			return (ret);
		}

		// ProxyAgentOctoClaim related RPCs. See AgentRPCClient.h for code documentation.

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_GETADDRESS_RPC)
		{
			int objects_tile_parm = objects_tile; // parameter 1
			uint32_t octo_ucid_parm = octo_ucid;	// parameter 2
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCGetAddress working on tile %u, objects_tile is %i, called from tile %u, with ucid %" PRIu32 "\n", hw::hal::Tile::getTileID(), objects_tile_parm, src, octo_ucid_parm);

			uintptr_t octo_address = 0; // return code
			os::agent::ResourceID res;
			res.tileID = src; // we have to find the AgentOctoClaim which physically is on tile objects_tile_parm, but has a resource tileID src;
			bool foundAddress = false;

			for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
			{
				// find the correct AgentInstance first, in order to find correct AgentOctoClaim
				AgentInstance *agent_instance = AgentSystem::getOwner(res);
				if (agent_instance == NULL)
				{
					DBG(SUB_AGENT, "ProxyAOCGetAddress found agent_instance == NULL at tile %u resid %u, ucid = %" PRIu32 "\n", res.tileID, res.resourceID, octo_ucid_parm);
					continue;
				}

				// finds its AgentClaim
				AgentClaim *agent_claim = agent_instance->getClaim(res);
				if (agent_claim == NULL)
				{
					DBG(SUB_AGENT, "ProxyAOCGetAddress found agent_claim == NULL at tile %u resid %u, ucid = %" PRIu32 "\n", res.tileID, res.resourceID, octo_ucid_parm);
					continue;
				}

				// finally find AgentOctoClaim
				if (agent_claim->getUcid() == octo_ucid_parm)
				{
					// we found our AgentOctoClaim, return its address so that in future RPCs on this tile won't need to do this lookup again
					octo_address = (uintptr_t)(agent_claim->getAgentOctoClaim()); // saves the address of the AgentOctoClaim object into octo_address
					foundAddress = true;
					DBG(SUB_AGENT, "ProxyAOCGetAddress found the wanted AgentOctoClaim at tile %u resid %u and address: %" PRIuPTR ", ucid = %" PRIu32 "\n", res.tileID, res.resourceID, octo_address, octo_ucid_parm);
					break;
				}
				else
				{
					DBG(SUB_AGENT, "ProxyAOCGetAddress found another AgentOctoClaim at tile %u resid %u. Its ucid (which is different from our wanted ucid %" PRIu32 ") is = %" PRIu32 "\n", res.tileID, res.resourceID, octo_ucid_parm, agent_claim->getUcid());
				}
			}

			if (!foundAddress)
			{
				DBG(SUB_AGENT, "Error! ProxyAOCGetAddress has not found AgentOctoClaim address! Possible reasons: You tried to create PAOC within iLet that has created the AOC (currently not supported); AOC has been changed in the meanwhile, e.g. reinvaded.\n");
			}

			getReturnEnvelope().sendReply(octo_address);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_GETRESOURCECOUNT_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCGetResourceCount working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR "\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);
			/*TODO maybe better do it like this?
	volatile uintptr_t iptr = 0xdeadbeef;
	unsigned int *ptr = (unsigned int *)iptr;
	see https://www.securecoding.cert.org/confluence/display/c/INT36-C.+Converting+a+pointer+to+integer+or+integer+to+pointer */
			uint8_t resource_count = octo_claim->getResourceCount();
			DBG(SUB_AGENT, "ProxyAOCGetResourceCount finds resource_count: %" PRIu8 "\n", resource_count);

			getReturnEnvelope().sendReply(resource_count);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_PRINT_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCPrint working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR "\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);

			octo_claim->print();
			DBG(SUB_AGENT, "ProxyAOCPrint is finished printing on tile %u\n", hw::hal::Tile::getTileID());

			getReturnEnvelope().sendReply(NULL); // Dummy return value, not actually used

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_ISEMPTY_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCIsEmpty working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR "\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);

			bool isEmpty = octo_claim->isEmpty();
			DBG(SUB_AGENT, "ProxyAOCIsEmpty AgentOctoClaim isEmpty? %s\n", isEmpty ? "True" : "False");

			getReturnEnvelope().sendReply(isEmpty);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_GETQUANTITY_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			os::agent::TileID tileID_parm = tileID;													// parameter 2
			os::agent::HWType type_parm = type;															// parameter 3
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCGetQuantity working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR ", tileID = %" PRIu8 " and type %d\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm, tileID_parm, type_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);

			uint8_t quantity = octo_claim->getQuantity(tileID_parm, type_parm);
			DBG(SUB_AGENT, "ProxyAOCGetQuantity AgentOctoClaim quantity = %" PRIu8 " \n", quantity);

			getReturnEnvelope().sendReply(quantity);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_GETTILECOUNT_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCGetTilecount working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR "\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);

			uint8_t tilecount = octo_claim->getTileCount();
			DBG(SUB_AGENT, "ProxyAOCGetTilecount AgentOctoClaim tilecount = %" PRIu8 " \n", tilecount);

			getReturnEnvelope().sendReply(tilecount);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_GETTILEID_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			uint8_t iterator_parm = iterator;																// parameter 2
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCGetTileID working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR ", iterator = %" PRIu8 "\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm, iterator_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);

			uint8_t tileid = octo_claim->getTileID(iterator_parm);
			DBG(SUB_AGENT, "ProxyAOCGetTileID AgentOctoClaim tileid = %" PRIu8 " \n", tileid);

			getReturnEnvelope().sendReply(tileid);

			return;
		}

		DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_FUNC, AGENT_PROXY_AOC_REINVADESAMECONSTR_RPC)
		{
			uintptr_t agentoctoclaim_address_parm = agentoctoclaim_address; // parameter 1
			TID src = getParent().getTID();
			DBG(SUB_AGENT, "ProxyAOCReinvadeSameConstr working on tile %u, called from tile %u, with agentoctoclaim_address %" PRIuPTR "\n", hw::hal::Tile::getTileID(), src, agentoctoclaim_address_parm);

			AgentOctoClaim *octo_claim = reinterpret_cast<AgentOctoClaim *>(agentoctoclaim_address_parm);

			int abs_sum_of_changes = agent_claim_reinvade((AbstractAgentOctoClaim *)octo_claim);

			DBG(SUB_AGENT, "ProxyAOCReinvadeSameConstr Successfully reinvaded with same Constraints. Absolute sum of changes = %i \n", abs_sum_of_changes);

			getReturnEnvelope().sendReply(abs_sum_of_changes);

			return;
		}

	} // namespace agent
} // namespace os
