#ifndef AGENTRPCHEADER_H
#define AGENTRPCHEADER_H

#include "cfAttribs.h"
#include "hw/hal/Tile.h"
#include "octo_types.h"
#include "os/agent3/FlatCluster.h"
#include "os/agent3/FlatClusterGuarantee.h"
#include "os/agent3/FlatOperatingPoint.h"
#include "os/agent3/AgentMemory.h"

#define AGENT_SETLEDS_RPC SetLEDs, uint32_t bits

#define AGENT_INVADE_RPC InvadeAgent, os::agent::AgentClaim, AgentInstance *Agent, os::agent::FlatConstraints flatConstraints
#define AGENT_REINVADE_RPC ReinvadeAgent, os::agent::AgentClaim, AgentInstance *Agent, os::agent::FlatConstraints flatConstraints, uint32_t old_claim_id
#define ACTOR_INVADE_RPC ActorInvadeAgent, os::agent::AgentClaim, AgentInstance *Agent, os::agent::ActorConstraint rpc_actor_constraints, os::agent::FlatOperatingPoint flat_operating_points, os::agent::FlatClusterGuarantee flat_cluster_guarantees, os::agent::FlatCluster flat_clusters

#define AGENT_REGISTER_AGENTOCTOCLAIM_RPC RegisterAgentOctoClaimAgent, AgentInstance *Agent, uint32_t claim_id, os::agent::AgentOctoClaim *octo_claim_ptr, os::res::DispatchClaim dispatch_claim

/*
 * We need a workaround here: normally, I'd like to have the resize_env_pointer as the
 * last parameter, but DMA RPC expects the first parameter to be convertible to void*.
 * An AgentClaim object obviously is not an option, but resize_env_t already *is*
 * type void*, so we just move it to the front.
 */
#define CLAIM_RUN_RESIZE_HANDLER_RPC RunResizeHandlerClaim, void *, resize_env_t resize_env_pointer, os::agent::AgentClaim loss_claim, resize_handler_t resize_handler, size_t tile_count, size_t res_per_tile

#define CLAIM_UPDATE_CLAIM_STRUCTURES_RPC UpdateClaimStructuresClaim, void *, os::agent::AgentOctoClaim *octo_claim_ptr, os::agent::AgentClaim claim
#define AGENT_PURE_RETREAT_RPC PureRetreatAgent, bool, AgentInstance *Agent, uint32_t claim_no
#define AGENT_CHECKALIVE_RPC CheckAgentAlive, bool, AgentInstance *Agent

#define AGENT_ALLOCATE_RPC AllocateAgent, AgentInstance *
#define AGENT_DELETE_RPC DeleteAgent, bool, AgentInstance *Agent, bool force

#define AGENT_GET_NAME AgentGetName, int, AgentInstance *ag, char buffer[], size_t size
#define AGENT_SET_NAME AgentSetName, AgentInstance *ag, const char *agent_name

#define AGENT_SIGNAL_CONFIG_DONE_RPC SignalOctoPOSConfigDone, AgentInstance *Agent, uint32_t claimNr
#define AGENT_GETINITIALCLAIM_RPC GetInitialAgentClaim, os::agent::AgentClaim, void *

// ProxyAgentOctoClaim RPCs. See AgentRPCClient.h for code documentation.
#define AGENT_PROXY_AOC_GETADDRESS_RPC ProxyAOCGetAddress, uintptr_t, int objects_tile, uint32_t octo_ucid
#define AGENT_PROXY_AOC_GETRESOURCECOUNT_RPC ProxyAOCGetResourceCount, uint8_t, uintptr_t agentoctoclaim_address
#define AGENT_PROXY_AOC_PRINT_RPC ProxyAOCPrint, void *, uintptr_t agentoctoclaim_address
#define AGENT_PROXY_AOC_ISEMPTY_RPC ProxyAOCIsEmpty, bool, uintptr_t agentoctoclaim_address
#define AGENT_PROXY_AOC_GETQUANTITY_RPC ProxyAOCGetQuantity, uint8_t, uintptr_t agentoctoclaim_address, os::agent::TileID tileID, os::agent::HWType type
#define AGENT_PROXY_AOC_GETTILECOUNT_RPC ProxyAOCGetTilecount, uint8_t, uintptr_t agentoctoclaim_address
#define AGENT_PROXY_AOC_GETTILEID_RPC ProxyAOCGetTileID, uint8_t, uintptr_t agentoctoclaim_address, uint8_t iterator
#define AGENT_PROXY_AOC_REINVADESAMECONSTR_RPC ProxyAOCReinvadeSameConstr, int, uintptr_t agentoctoclaim_address

/* a=target variable, b=bit number to act upon 0-n */
#define SET_BIT(a, b) ((a) |= (1UL << (b)))
#define CLEAR_BIT(a, b) ((a) &= ~(1UL << (b)))
#define FLIP_BIT(a, b) ((a) ^= (1UL << (b)))
#define TEST_BIT(a, b) ((a) & (1UL << (b)))

namespace os
{
	namespace agent
	{

		class AgentMemory;
		class Agent;
		class AgentSystem;
		class AgentInstance;
		class AgentClaim;
		class AgentMalleabilityClaim;
		class AgentOctoClaim;
		class AgentConstraint;
		class ActorConstraint;
		class ActorConstraintSolver;
		class FlatConstraints;
		class ProxyAgentOctoClaim;
		class SolverClaim;

		typedef uint64_t ResourceRating;
		typedef uint8_t TileID;
		typedef ResType HWType;

		class __ResID_s
		{
		public:
			TileID tileID;
			uint8_t resourceID;
			uint32_t toAgentX10Handle()
			{
				uint32_t retval = resourceID | (tileID << 8);
				return retval;
			}

			void *operator new(size_t s)
			{
				return AgentMemory::agent_mem_allocate(s);
			}

			void *operator new[](size_t s)
			{
				return AgentMemory::agent_mem_allocate(s);
			}

			void operator delete(void *c, size_t s)
			{
				AgentMemory::agent_mem_free(c);
			}

			void operator delete[](void *c, size_t s)
			{
				AgentMemory::agent_mem_free(c);
			}

			void fromAgentX10Handle(uint32_t handle)
			{
				resourceID = handle & 0xFF;
				tileID = (handle >> 8) & 0xFF;
			}
			bool operator==(__ResID_s const &lhs) const
			{
				return lhs.resourceID == this->resourceID && lhs.tileID == this->tileID;
			}

			bool operator!=(__ResID_s const &lhs) const
			{
				return lhs.resourceID != this->resourceID || lhs.tileID != this->tileID;
			}

			bool isInvalid() const
			{
				return tileID >= hw::hal::Tile::MAX_TILE_COUNT;
			}
		};

		typedef __ResID_s ResourceID; // TODO: Move to HW::...

		static const uint8_t HWTypes = 3; // TODO: Move to HW::...
		static const uint32_t MAX_RES_PER_TILE = cf_hw_sys_max_cores_per_tile;
		static const TileID NOTILE = 0xFF; // TODO: Move to HW::...

		static const uint32_t MAX_AGENTS = 32;
		static const uint8_t MAX_REQUESTS_PER_AGENT = 32;
		static const uint8_t INVALID_REQUEST_SLOT = 255;

	} // namespace agent
} // namespace os

#endif // AGENTRPCHEADER_H
