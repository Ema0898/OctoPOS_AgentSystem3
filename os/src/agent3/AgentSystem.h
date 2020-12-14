#ifndef AGENTSYSTEM_H
#define AGENTSYSTEM_H

#include "hw/hal/CPU.h"
#include "hw/hal/Tile.h"

#include "os/rpc/RPCStub.h"

#include "lib/adt/AtomicID.h"
#include "lib/debug.h"

#include "os/res/ProxyClaim.h"
#include "os/res/DispatchClaim.h"
#include "os/proc/iLet.h"

#include "octo_types.h"

#include "lib/adt/SimpleSpinlock.h"
#include "os/irq/Guard.h"

#include "os/agent3/AgentConstraint.h"
#include "os/agent3/AgentRPCHeader.h"

#include "os/agent3/Platform.h"

namespace os
{
	namespace agent
	{

		/*
 * I loaded this thing with some state, i.e., an array that keeps
 * track of all AgentInstances. So it would probably be nicer to
 * make a Singleton out of this..
 * F Pepinghege
 */
		class AgentSystem
		{
			static lib::adt::SimpleSpinlock agentLock;

		public:
			static uint32_t AGENT_TILE;
			static os::res::DispatchClaim AGENT_DISPATCHCLAIM;

			static const uint8_t FLAG_AVAILABLE_FOR_BARGAINING = 0;

			static const uint8_t IDLE_AGENT = 0;
			static const uint8_t INIT_AGENT = 1;

			static void init();

			static os::res::DispatchClaim getDispatchClaim();

			static void enterAgent(TID src)
			{
				DBG(SUB_AGENT, "EnterAgent %d (0x%p)\n", src, os::proc::ContextManager::getCurrentContext());
				if (hw::hal::Tile::getTileID() == AGENT_TILE)
				{
					os::irq::Guard::enter();
				}
				agentLock.lock();
				DBG(SUB_AGENT, "EnterAgent complete %d (0x%p)\n", src, os::proc::ContextManager::getCurrentContext());
			}
			static void leaveAgent(TID src)
			{
				agentLock.unlock();
				if (hw::hal::Tile::getTileID() == AGENT_TILE)
				{
					os::irq::Guard::leave();
				}
				DBG(SUB_AGENT, "LeaveAgent %d (0x%p)\n", src, os::proc::ContextManager::getCurrentContext());
			}

			static AgentInstance *idleAgent;
			static AgentInstance *initialAgent;
			static ActorConstraintSolver *solver;

			static uint8_t registerAgent(AgentInstance *agent);
			static void unregisterAgent(AgentInstance *agent, const uint8_t id);

			/**
     * \brief Collect MalleabilityClaims all other requests
     *
     * The MalleabilityClaims are written into a buffer that will be allocated in this function.
     * Freeing the allocated memory is the responsibility of the caller.
     *
     * \return the number of collected MalleabilityClaims.
     */
			static uint8_t collectMalleabilityClaims(AgentMalleabilityClaim ***ptrToBuffer,
																							 AgentInstance *callingAgent, int slot);
#define MAX_MALLEABILITY_CLAIMS 32

			static ResType getHWType(ResourceID resource)
			{
				return HardwareMap[resource.tileID][resource.resourceID].type;
			}
			static uint8_t getHWFlags(ResourceID resource)
			{
				return HardwareMap[resource.tileID][resource.resourceID].flags;
			}

			static bool testFlag(const ResourceID &resource, uint8_t flag)
			{
				return TEST_BIT(systemResources[resource.tileID][resource.resourceID].flags, flag);
			}

			static void setFlag(ResourceID &resource, uint8_t flag)
			{
				SET_BIT(systemResources[resource.tileID][resource.resourceID].flags, flag);
			}

			static void clearFlag(ResourceID &resource, uint8_t flag)
			{
				CLEAR_BIT(systemResources[resource.tileID][resource.resourceID].flags, flag);
			}

			static void setOwner(const ResourceID &resource, AgentInstance *ag);
			static void unsetOwner(const ResourceID &resource, const AgentInstance *ag);
			static AgentInstance *getOwner(const ResourceID &resource);

			/*
	 * Resource-Transfer between two Agents must always happen in that order:
	 * - Current owner: AgentSystem::unsetOwner(res, this);
	 * - New owner: AgentSystem::setOwner(res, this);
	*/

			struct sysRes_s
			{
				uint8_t flags;
				AgentInstance *responsibleAgent;
			};

			static sysRes_s systemResources[hw::hal::Tile::MAX_TILE_COUNT][os::agent::MAX_RES_PER_TILE];

			static void lockBargaining(AgentClaim *claim);
			static void unlockBargaining(AgentClaim *claim);

			/*
	 * Returns a reference-to-const for the first AgentID different from agent_id if existent,
	 * otherwise a reference-to-const of agent_id.
	 */
			static const AgentInstance &get_other_agent(const TileID tile_id, const AgentInstance &agent_id);

			static uint8_t getAgentCount()
			{
				return agentCount;
			}

			static AgentInstance *getAgentByID(uint8_t id)
			{
				return instances[id];
			}

			static AgentInstance **getAgentArray()
			{
				return instances;
			}

			static void dumpAgents();

			static bool can_use_tile_exclusively(const TileID tile_id, const AgentInstance &agent_id);

		private:
			static uint8_t agentCount;
			static uint8_t nextAgent;
			static AgentInstance *instances[MAX_AGENTS];
			static uint8_t getNextAgentID();

			//Constraints used for the initial Agent
			static TileSharingConstraint *tsc;
			static AndConstraintList *acl;
			static TileConstraint *tc;
			static PEQuantityConstraint *peqc;
		};

	} // namespace agent
} // namespace os

#endif // AGENTSYSTEM_H
