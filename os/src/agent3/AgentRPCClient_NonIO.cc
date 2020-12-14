/*
REMARK:
The includes are somehow strange. AgentRPCClient_Hetero.cc has different includes
*/
#include "os/agent3/AgentRPCHeader.h"
#include "os/agent3/AgentRPCClient.h"
#include "os/rpc/RPCStub.h"

#include "os/agent3/AgentSystem.h"
#include "os/agent3/Agent.h" // sorry... vorerst zu dumm zum völlig auseinanderziehen.

#include "os/agent3/ActorConstraint.h"

/*
REMARK:
[VW] This code will be executed on all tiles non io tiles on the hw platforms
[VW] This will currently not work on hw designs without io tile
*/

namespace os
{
  namespace agent
  {

    DEFINE_RPC(NO_ANSWER_RPC_STUB, AGENT_SETLEDS_RPC);
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_INVADE_RPC);
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_REINVADE_RPC);
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, ACTOR_INVADE_RPC);
    DEFINE_RPC(NO_ANSWER_RPC_STUB, AGENT_REGISTER_AGENTOCTOCLAIM_RPC);
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, CLAIM_RUN_RESIZE_HANDLER_RPC);
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, CLAIM_UPDATE_CLAIM_STRUCTURES_RPC);
    DEFINE_RPC(BLOCK_RPC_STUB, AGENT_ALLOCATE_RPC);
    DEFINE_RPC(BLOCK_RPC_STUB, AGENT_DELETE_RPC);
    DEFINE_RPC(BLOCK_RPC_STUB, AGENT_GET_NAME);
    DEFINE_RPC(NO_ANSWER_RPC_STUB, AGENT_SET_NAME);
    DEFINE_RPC(NO_ANSWER_RPC_STUB, AGENT_SIGNAL_CONFIG_DONE_RPC);
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_GETINITIALCLAIM_RPC);
    DEFINE_RPC(BLOCK_RPC_STUB, AGENT_CHECKALIVE_RPC);
    DEFINE_RPC(BLOCK_RPC_STUB, AGENT_PURE_RETREAT_RPC);
    // ProxyAgentOctoClaim RPCs
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_GETADDRESS_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_GETRESOURCECOUNT_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_PRINT_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_ISEMPTY_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_GETQUANTITY_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_GETTILECOUNT_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_GETTILEID_RPC)
    DEFINE_RPC(MAN_ANSWER_BLOCK_RPC_DMA_STUB, AGENT_PROXY_AOC_REINVADESAMECONSTR_RPC)

  } // namespace agent
} // namespace os

os::agent::AgentInstance *os::agent::Agent::createAgent()
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  //REMARK: this code will not work on designs without io tile?
  DBG(SUB_AGENT, "Sending RPC for \"createAgent\"\r\n");
  AllocateAgentRPC::FType future;
  AllocateAgentRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future);
  DBG(SUB_AGENT, "forcing RPC for \"createAgent\"\r\n");
  future.force();
  DBG(SUB_AGENT, "Allocated Agent at %p\r\n", future.getData());
/* --- JK: Telemetry Begin---*/
#ifdef cf_gui_enabled
  AgentInstance *newlyCreatedAgent = (AgentInstance *)future.getData();
  if (newlyCreatedAgent)
  {
    MetricNewAgent m((int)newlyCreatedAgent);
    MetricSender::measureMetric(m);
  }
#endif
  /* --- JK Telemetry End ---*/

  return future.getData();
}

void os::agent::Agent::deleteAgent(os::agent::AgentInstance *ag, bool force)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  DBG(SUB_AGENT, "Sending RPC for \"deleteAgent\"\r\n");
// JK: Remember the ID of the Agent here so that we can monitor it.
// After all, the Agent will be deleted within this method call graph..
#ifdef cf_gui_enabled
  int idOfAgentToDelete = (int)ag;
#endif

  DeleteAgentRPC::FType future;
  DeleteAgentRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag, force);
  DBG(SUB_AGENT, "forcing RPC for \"deleteAgent\"\r\n");
  future.force();

/* --- JK: Telemetry DELETE AGENT---*/
#ifdef cf_gui_enabled
  MetricDeletedAgent m(idOfAgentToDelete);
  MetricSender::measureMetric(m);
#endif
  /* --- JK Telemetry End ---*/
}

os::agent::AgentClaim os::agent::Agent::invade_agent_constraints(os::agent::AgentInstance *ag, os::agent::AgentConstraint *agent_constraints)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  DBG(SUB_AGENT, "NonIO: Invade Agent %p with Min: %d and Max %d cores (Size %" PRIuPTR ")\r\n", ag, agent_constraints->getMinOfType((os::agent::HWType)0), agent_constraints->getMaxOfType((os::agent::HWType)0), sizeof(*agent_constraints));
  os::agent::FlatConstraints flatC;
  if (!flatC.flatten(agent_constraints))
  {
    DBG(SUB_AGENT, "Constraint-flattening didn't work in NonIO-invade. Random things will happen!");
  }

  InvadeAgentMANRPC_DMA::FType future;
  InvadeAgentMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, ag, flatC);
  future.force();

/* --- JK: Telemetry for INVADE ---*/
#ifdef cf_gui_enabled
  AgentClaim claim = future.getData();
  MetricAgentInvade m(&claim);
  MetricSender::measureMetric(m);
#endif
  /* --- JK Telemetry End ---*/

  return future.getData();
}

os::agent::AgentClaim os::agent::Agent::reinvade_agent_constraints(os::agent::AgentInstance *ag, os::agent::AgentConstraint *agent_constraints, uint32_t old_claim_id)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  DBG(SUB_AGENT, "NonIO: Reinvade Agent %p with Min: %d and Max %d cores (Size %" PRIuPTR ")\r\n", ag, agent_constraints->getMinOfType((os::agent::HWType)0), agent_constraints->getMaxOfType((os::agent::HWType)0), sizeof(*agent_constraints));
  os::agent::FlatConstraints flatC;
  if (!flatC.flatten(agent_constraints))
  {
    DBG(SUB_AGENT, "Constraint-flattening didn't work in NonIO-Reinvade. Random things will happen!");
  }

  ReinvadeAgentMANRPC_DMA::FType future;
  ReinvadeAgentMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, ag, flatC, old_claim_id);
  future.force();

/* --- JK: Telemetry for INVADE ---*/
#ifdef cf_gui_enabled
  AgentClaim claim = future.getData();
  MetricAgentInvade m(&claim);
  MetricSender::measureMetric(m);
#endif
  /* --- JK Telemetry End ---*/

  return future.getData();
}

os::agent::AgentClaim os::agent::Agent::invade_actor_constraints(os::agent::AgentInstance *ag, os::agent::ActorConstraint *actor_constraints)
{

  /*
	 * Memory serialization of all the pointers contained in the actor constraint
	 */
  FlatCluster flat_clusters;
  if (!flat_clusters.flatten(actor_constraints->get_cluster_list(), AC_MAX_NUMBER_OF_CLUSTERS))
    DBG(SUB_AGENT, "Clusters-flattening didn't work in Hetero-invade. Random things will happen!\n");
  else
    DBG(SUB_AGENT, "Clusters-flattening work in Hetero-invade!\n");

  FlatClusterGuarantee flat_cluster_guarantees;
  if (!flat_cluster_guarantees.flatten(actor_constraints->get_cluster_guarantee_list(), AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES))
    DBG(SUB_AGENT, "ClusterGuarantees-flattening didn't work in Hetero-invade. Random things will happen!\n");
  else
    DBG(SUB_AGENT, "ClusterGuarantees-flattening work in Hetero-invade!\n");

  FlatOperatingPoint flat_operating_points;
  if (!flat_operating_points.flatten(actor_constraints->get_operating_point_list(), AC_MAX_NUMBER_OF_OPERATING_POINTS))
    DBG(SUB_AGENT, "Operating Points-flattening didn't work in Hetero-invade. Random things will happen!\n");
  else
    DBG(SUB_AGENT, "Operating Points-flattening work in Hetero-invade!\n");

  ActorInvadeAgentMANRPC_DMA::FType future;
  ActorInvadeAgentMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, ag, *actor_constraints, flat_operating_points, flat_cluster_guarantees, flat_clusters);
  future.force();

/* --- JK: Telemetry for INVADE ---*/
#ifdef cf_gui_enabled
  AgentClaim claim = future.getData();
  MetricAgentInvade m(&claim);
  MetricSender::measureMetric(m);
#endif
  /* --- JK Telemetry End ---*/
  return future.getData();
}

void os::agent::Agent::register_AgentOctoClaim(os::agent::AgentInstance *ag, uint32_t claim_id, os::agent::AgentOctoClaim *octo_claim_ptr, os::res::DispatchClaim dispatch_claim)
{
  RegisterAgentOctoClaimAgentNARPC::stub(os::agent::AgentSystem::AGENT_TILE, ag, claim_id, octo_claim_ptr, dispatch_claim);
}

void *os::agent::Agent::run_resize_handler(os::res::DispatchClaim dispatch_claim, resize_env_t resize_env_pointer, os::agent::AgentClaim &loss_claim, resize_handler_t resize_handler, size_t tile_count, size_t res_per_tile)
{
  /* We don't actually need any futures/don't use a return value, but there is no NO ANSWER DMA RPC. */
  RunResizeHandlerClaimMANRPC_DMA::FType future;
  RunResizeHandlerClaimMANRPC_DMA::stub(dispatch_claim, &future, resize_env_pointer, loss_claim, resize_handler, tile_count, res_per_tile);
  future.force();

  return (future.getData());
}

void *os::agent::Agent::update_claim_structures(os::res::DispatchClaim dispatch_claim, os::agent::AgentOctoClaim *octo_claim_ptr, os::agent::AgentClaim &claim)
{
  UpdateClaimStructuresClaimMANRPC_DMA::FType future;
  UpdateClaimStructuresClaimMANRPC_DMA::stub(dispatch_claim, &future, octo_claim_ptr, claim);
  future.force();

  return (future.getData());
}

os::agent::AgentClaim os::agent::Agent::getInitialAgentClaim(void *)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  GetInitialAgentClaimMANRPC_DMA::FType future;
  GetInitialAgentClaimMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, NULL);
  future.force();

/* --- JK: Telemetry for INVADE ---*/
#ifdef cf_gui_enabled
  os::agent::AgentClaim result = future.getData();
  MetricAgentInvade m(&result);
  m.isInitialClaim = 1;
  MetricSender::measureMetric(m);
#endif
  /* Telemetry End */

  return future.getData();
}

void os::agent::Agent::setLEDs(uint32_t bits)
{
}

void os::agent::Agent::signalOctoPOSConfigDone(os::agent::AgentInstance *ag, uint32_t claimNr)
{
  SignalOctoPOSConfigDoneNARPC::stub(os::agent::AgentSystem::AGENT_TILE, ag, claimNr);
}

bool os::agent::Agent::pure_retreat(os::agent::AgentInstance *ag, uint32_t claim_no)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  DBG(SUB_AGENT, "os::agent::Agent::pure_retreat(%p, %" PRIu32 ")\n", ag, claim_no);
  PureRetreatAgentRPC::FType future;
  PureRetreatAgentRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag, claim_no);
  future.force();

/* --- JK: Telemetry for RETREAT---*/
#ifdef cf_gui_enabled
  MetricAgentRetreat m(claim_no, agentId);
  MetricSender::measureMetric(m);
#endif
  /* --- JK Telemetry End ---*/

  return (future.getData());
}

bool os::agent::Agent::checkAlive(os::agent::AgentInstance *ag)
{
  CheckAgentAliveRPC::FType future;
  CheckAgentAliveRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag);
  future.force();
  return future.getData(); // returns the same as checkAlive to avoid unneccessary RPC's
}

int os::agent::Agent::getAgentName(os::agent::AgentInstance *ag, char buffer[], size_t size)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  if (!ag)
    panic("AgentRPCClient::getAgentName:: AgentInstance *ag == NULL");

  AgentGetNameRPC::FType future;

  AgentGetNameRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag, buffer, size);
  future.force();

  return future.getData();
}

void os::agent::Agent::setAgentName(os::agent::AgentInstance *ag,
                                    const char *agent_name)
{

  //REMARK: this code is simply duplicated from AgentRPCClient_Hetero.cc
  if (!ag)
    panic("AgentRPCClient::setAgentName:: AgentInstance *ag == NULL");
/* -- Telemetry for Name Change --
     Send out Message before RPC Call to make sure
     Name change reaches GUI before program ends.
  */
#ifdef cf_gui_enabled
  MetricAgentRename m((int)ag, agent_name);
  MetricSender::measureMetric(m);
#endif
  /* -- Telemetry end -- */

  AgentSetNameNARPC::stub(os::agent::AgentSystem::AGENT_TILE, ag, agent_name);
}

// ProxyAgentOctoClaim RPCs. See AgentRPCClient.h for code documentation.
// code duplicated from AgentRPCClient_Hetero.cc; added warnings that code hasn't been tested on these Platforms

uintptr_t os::agent::Agent::proxyAOC_get_AOC_address(int objects_tile, uint32_t octo_ucid)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCGetAddressMANRPC_DMA::FType future;
  ProxyAOCGetAddressMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, objects_tile, octo_ucid); // crashes when AOC has been retreated
  future.force();
  return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_get_ResourceCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCGetResourceCountMANRPC_DMA::FType future;
  ProxyAOCGetResourceCountMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
  future.force();
  return future.getData();
}

void *os::agent::Agent::proxyAOC_print(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCPrintMANRPC_DMA::FType future;
  ProxyAOCPrintMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
  future.force();
  return future.getData();
}

bool os::agent::Agent::proxyAOC_isEmpty(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCIsEmptyMANRPC_DMA::FType future;
  ProxyAOCIsEmptyMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
  future.force();
  return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_getQuantity(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, os::agent::TileID tileID, os::agent::HWType type)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCGetQuantityMANRPC_DMA::FType future;
  ProxyAOCGetQuantityMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address, tileID, type);
  future.force();
  return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_getTileCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCGetTilecountMANRPC_DMA::FType future;
  ProxyAOCGetTilecountMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
  future.force();
  return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_getTileID(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, uint8_t iterator)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCGetTileIDMANRPC_DMA::FType future;
  ProxyAOCGetTileIDMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address, iterator);
  future.force();
  return future.getData();
}

int os::agent::Agent::proxyAOC_reinvadeSameConstraints(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
  DBG(SUB_AGENT, "Warning, ProxyAgentOctoClaim RPCs have not been tested on these NonIO tiles. Only AgentRPCClient_Hetero tested. You may remove this warning if it works on this platform.\n");
  ProxyAOCReinvadeSameConstrMANRPC_DMA::FType future;
  ProxyAOCReinvadeSameConstrMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
  future.force();
  return future.getData();
}
