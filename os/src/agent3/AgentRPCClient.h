#ifndef AGENTRPCCLIENT_H
#define AGENTRPCCLIENT_H

#include "os/agent3/AgentRPCHeader.h"
#include "os/agent3/AgentClaim.h"
#include "os/agent3/AgentConstraint.h"

namespace os
{
	namespace agent
	{

		class Agent
		{
		public:
			static AgentInstance *createAgent();
			static void deleteAgent(os::agent::AgentInstance *ag, bool force = false);

			static AgentClaim invade_agent_constraints(AgentInstance *ag, os::agent::AgentConstraint *agent_constraints);
			static AgentClaim reinvade_agent_constraints(AgentInstance *ag, os::agent::AgentConstraint *agent_constraints, uint32_t old_claim_id = os::agent::AgentClaim::INVALID_CLAIM);
			static AgentClaim invade_actor_constraints(AgentInstance *ag, os::agent::ActorConstraint *actor_constraints);
			static void register_AgentOctoClaim(AgentInstance *ag, uint32_t claim_id, AgentOctoClaim *octo_claim_ptr, os::res::DispatchClaim dispatch_claim);
			static void *run_resize_handler(os::res::DispatchClaim dispatch_claim, resize_env_t resize_env_pointer, os::agent::AgentClaim &loss_claim, resize_handler_t resize_handler, size_t tile_count, size_t res_per_tile);
			static void *update_claim_structures(os::res::DispatchClaim dispatch_claim, os::agent::AgentOctoClaim *octo_claim_ptr, os::agent::AgentClaim &claim);
			static AgentClaim getInitialAgentClaim(void * = NULL);
			static bool pure_retreat(AgentInstance *ag, uint32_t claim_no);
			static bool checkAlive(AgentInstance *ag);

			static void setLEDs(uint32_t bits);

			static void signalOctoPOSConfigDone(AgentInstance *ag, uint32_t claimNr);

			/**
	 * @brief      Gets the agent's name and writes it into buffer
	 *				The buffer must be allocated on the global memory (MEM_TLM_GLOBAL or MEM_SHM), as it gets filled in
	 *				a RPC-Call, if buffer on local memory it will stay empty.
	 *				If Agent Name is longer than size only (size) number of characters will be copied.
	 * @param      ag      Instance of AgentInstance
	 * @param      buffer  The buffer, must be allocated on global memory (MEM_TLM_GLOBAL or MEM_SHM)
	 * @param[in]  size    The size of buffer.
	 *
	 * @return     The agent name.
	 */
			static int getAgentName(os::agent::AgentInstance *ag, char buffer[], size_t size);

			/**
	 * @brief      Sets the agent name.
	 *				If name is longer than the maximum number of characters for a name
	 *				it will be cut off.
	 * @param      ag          Instance of AgentInstance
	 * @param[in]  agent_name  The agent name
	 */
			static void setAgentName(os::agent::AgentInstance *ag, const char *agent_name);

			// ProxyAgentOctoClaim related RPCs:

			/**
	 * \brief Returns the address of the AgentOctoClaim via DMA RPC to AgentSystem::AGENT_TILE.
	 *
	 * Returns the address of the AgentOctoClaim via DMA RPC to AgentSystem::AGENT_TILE. Using AgentSystem::getOwner funciton, goes through all resources on tile objects_tile: First, tries to find a non-NULL AgentInstance* which has this resource; in case of such non-NULL pointer, then tries to find a non-NULL AgentClaim* of this AgentInstance*; in case of such non-NULL pointer, then tries to find a non-NULL AgentOctoClaim* of this AgentClaim*; in case of such non-NULL AgentOctoClaim*, returns the address of this AgentOctoClaim*. Returns NULL if AgentOctoClaim could not be found.
	 *
	 * \param objects_tile The tile id where the AgentOctoClaim is located
	 * \param octo_ucid The unique id of the AgentOctoClaim
	 *
	 * \return The address of the AgentOctoClaim that we look for. NULL if AgentOctoClaim could not be found.
	 */
			static uintptr_t proxyAOC_get_AOC_address(int objects_tile, uint32_t octo_ucid);

			/**
	 * \brief Returns the AgentOctoClaim's resource count via DMA RPC.
	 *
	 * Returns the AgentOctoClaim's resource count via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 *
	 * \return The AgentOctoClaim's resource count.
	 */
			static uint8_t proxyAOC_get_ResourceCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address);

			/**
	 * \brief Prints AgentOctoClaim's AgentClaim on its tile via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * Prints AgentOctoClaim's AgentClaim on its tile via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 */
			static void *proxyAOC_print(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address);

			/**
	 * \brief Returns whether the AgentOctoClaim is empty via DMA RPC.
	 *
	 * Returns whether the AgentOctoClaim is empty via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 *
	 * \return Whether the AgentOctoClaim is empty.
	 */
			static bool proxyAOC_isEmpty(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address);

			/**
	 * \brief Returns the AgentOctoClaim's number of resources of specific tile and type is empty via DMA RPC.
	 *
	 * Returns the AgentOctoClaim's number of resources of specific tile and type via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 *
	 * \return The AgentOctoClaim's number of resources of specific tile and type.
	 */
			static uint8_t proxyAOC_getQuantity(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, os::agent::TileID tileID, os::agent::HWType type);

			/**
	 * \brief Returns the AgentOctoClaim's number of claimed tiles via DMA RPC.
	 *
	 * Returns the AgentOctoClaim's number of claimed tiles via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 *
	 * \return The AgentOctoClaim's number of claimed tiles.
	 */
			static uint8_t proxyAOC_getTileCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address);

			/**
	 * \brief Returns the AgentOctoClaim's tile id at the specified iterator position via DMA RPC.
	 *
	 * Returns the AgentOctoClaim's tile id at the specified iterator position via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \note The getTileID function is described in class AgentOctoClaim as "this function doesn't make too much sense as it is..".
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 *
	 * \return The AgentOctoClaim's tile id at the specified iterator position.
	 */
			static uint8_t proxyAOC_getTileID(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, uint8_t iterator);

			/**
	 * \brief Reinvades the AgentOctoClaim with same Constraints via DMA RPC.
	 *
	 * Reinvades the AgentOctoClaim with same Constraints via DMA RPC using dispatch_claim and AgentOctoClaim's address agentoctoclaim_address.
	 *
	 * \param dispatch_claim DispatchClaim to ProxyAgentOctoClaim's objects_tile
	 * \param agentoctoclaim_address The address of the AgentOctoClaim
	 *
	 * \return Absolute sum of resource changes after reinvading the AgentOctoClaim with same Constraints. (i.e. losing 2, gaining 1 resource would return: 3)
	 */
			static int proxyAOC_reinvadeSameConstraints(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address);
		};

	} // namespace agent
} // namespace os

#endif // AGENTRPCCLIENT_H
