#include "os/agent3/AgentRPCHeader.h"
#include "os/agent3/AgentRPCClient.h"
#include "os/agent3/Agent.h"
#include "os/rpc/RPCStub.h"
#include "os/agent3/AgentRPCImpl.h" //TODO: remove?

#include <stdio.h>

/*
REMARK: [VW] This code will be executed on all tiles (io and non io) on the guest layer
*/

// JK: Added for Telemetry Functionality
#ifdef cf_gui_enabled
#include "os/agent3/MetricSender.h"
#include "os/agent3/Metric.h"
#include "os/agent3/MetricNewAgent.h"
#include "os/agent3/MetricDeletedAgent.h"
#include "os/agent3/MetricAgentInvade.h"
#include "os/agent3/MetricAgentRetreat.h"
#include "os/agent3/MetricAgentRename.h"
#endif

os::agent::AgentInstance *os::agent::Agent::createAgent()
{

	DBG(SUB_AGENT, "Sending RPC to %" PRIu32 " for \"createAgent\"\r\n", os::agent::AgentSystem::AGENT_TILE);
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		AllocateAgentRPC instance(os::res::DispatchClaim::getOwnDispatchClaim());
		AllocateAgentRPC::FType future;
		future.init();
		future.signal(instance.impl());
/* --- JK: Telemetry Begin---*/
#ifdef cf_gui_enabled
		AgentInstance *newlyCreatedAgent = (AgentInstance *)future.getData();
		const uint8_t id = newlyCreatedAgent->get_id();
		MetricNewAgent m(id);
		MetricSender::measureMetric(m);
#endif
		/* --- JK Telemetry End ---*/
		return future.getData();
	}
	else
	{
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
			const uint8_t id = newlyCreatedAgent->get_id();
			MetricNewAgent m(id);
			MetricSender::measureMetric(m);
		}
#endif
		/* --- JK Telemetry End ---*/
		return future.getData();
	}

	return NULL;
}

void os::agent::Agent::deleteAgent(os::agent::AgentInstance *ag, bool force)
{
	DBG(SUB_AGENT, "Sending RPC for \"deleteAgent\"\r\n");
// JK: Remember the ID of the Agent here so that we can monitor it.
// After all, the Agent will be deleted within this method call graph..
#ifdef cf_gui_enabled
	const uint8_t id = ag->get_id();
#endif
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		DeleteAgentRPC::FType future;
		DeleteAgentRPC instance(os::res::DispatchClaim::getOwnDispatchClaim());
		future.init();
		future.signal(instance.impl(ag, force));
	}
	else
	{
		DeleteAgentRPC::FType future;
		DeleteAgentRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag, force);
		DBG(SUB_AGENT, "forcing RPC for \"deleteAgent\"\r\n");
		future.force();
	}
/* --- JK: Telemetry DELETE AGENT---*/
#ifdef cf_gui_enabled
	MetricDeletedAgent m(id);
	MetricSender::measureMetric(m);
#endif
	/* --- JK Telemetry End ---*/
}

os::agent::AgentClaim os::agent::Agent::invade_agent_constraints(os::agent::AgentInstance *ag, os::agent::AgentConstraint *agent_constraints)
{
	DBG(SUB_AGENT, "Hetero: Invade Agent %p with Min: %d and Max %d cores (Size %" PRIuPTR ")\r\n", ag, agent_constraints->getMinOfType((os::agent::HWType)0), agent_constraints->getMaxOfType((os::agent::HWType)0), sizeof(*agent_constraints));
	os::agent::FlatConstraints flatC;
	if (!flatC.flatten(agent_constraints))
	{
		DBG(SUB_AGENT, "Constraint-flattening didn't work in Hetero-invade. Random things will happen!");
	}

	InvadeAgentMANRPC_DMA::FType future;
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		// Always use flatConstraints, even without RPC, as this will copy the Constraint objects so applications don't have to save them.
		// TODO: Invade ohne RPC ist... fricklig.
		InvadeAgentMANRPC_DMA instance(os::res::DispatchClaim::getOwnDispatchClaim(), &future, 0);
		future.init();
		instance.impl(ag, flatC);
		future.force();
	}
	else
	{
		InvadeAgentMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, ag, flatC);
		future.force();
	}
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
	DBG(SUB_AGENT, "Hetero: Reinvade Agent %p with Min: %d and Max %d cores (Size %" PRIuPTR ")\r\n", ag, agent_constraints->getMinOfType((os::agent::HWType)0), agent_constraints->getMaxOfType((os::agent::HWType)0), sizeof(*agent_constraints));
	os::agent::FlatConstraints flatC;
	if (!flatC.flatten(agent_constraints))
	{
		DBG(SUB_AGENT, "Constraint-flattening didn't work in Hetero-reinvade. Random things will happen!");
	}

	ReinvadeAgentMANRPC_DMA::FType future;
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		// Always use flatConstraints, even without RPC, as this will copy the Constraint objects so applications don't have to save them.
		// TODO: Renvade ohne RPC ist... fricklig.
		ReinvadeAgentMANRPC_DMA instance(os::res::DispatchClaim::getOwnDispatchClaim(), &future, 0);
		future.init();
		instance.impl(ag, flatC, old_claim_id);
		future.force();
	}
	else
	{
		ReinvadeAgentMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, ag, flatC, old_claim_id);
		future.force();
	}
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
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		// TODO: Invade ohne RPC ist... fricklig.
		ActorInvadeAgentMANRPC_DMA instance(os::res::DispatchClaim::getOwnDispatchClaim(), &future, 0);
		future.init();
		instance.impl(ag, *actor_constraints, flat_operating_points, flat_cluster_guarantees, flat_clusters);
		future.force();
	}
	else
	{
		ActorInvadeAgentMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, ag, *actor_constraints, flat_operating_points, flat_cluster_guarantees, flat_clusters);
		future.force();
	}
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
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		RegisterAgentOctoClaimAgentNARPC instance(hw::hal::Tile::getTileID());
		instance.impl(ag, claim_id, octo_claim_ptr, dispatch_claim);
	}
	else
	{
		RegisterAgentOctoClaimAgentNARPC::stub(os::agent::AgentSystem::AGENT_TILE, ag, claim_id, octo_claim_ptr, dispatch_claim);
	}
}

void *os::agent::Agent::run_resize_handler(os::res::DispatchClaim dispatch_claim, resize_env_t resize_env_pointer, os::agent::AgentClaim &loss_claim, resize_handler_t resize_handler, size_t tile_count, size_t res_per_tile)
{
	RunResizeHandlerClaimMANRPC_DMA::FType future;
	if (hw::hal::Tile::getTileID() == dispatch_claim.getTID())
	{
		RunResizeHandlerClaimMANRPC_DMA instance(dispatch_claim, &future, 0);
		future.init();
		instance.impl(resize_env_pointer, loss_claim, resize_handler, tile_count, res_per_tile);
		future.force();
	}
	else
	{
		RunResizeHandlerClaimMANRPC_DMA::stub(dispatch_claim, &future, resize_env_pointer, loss_claim, resize_handler, tile_count, res_per_tile);
		future.force();
	}

	return (future.getData());
}

void *os::agent::Agent::update_claim_structures(os::res::DispatchClaim dispatch_claim, os::agent::AgentOctoClaim *octo_claim_ptr, os::agent::AgentClaim &claim)
{
	UpdateClaimStructuresClaimMANRPC_DMA::FType future;
	if (hw::hal::Tile::getTileID() == dispatch_claim.getTID())
	{
		UpdateClaimStructuresClaimMANRPC_DMA instance(dispatch_claim, &future, 0);
		future.init();
		instance.impl(octo_claim_ptr, claim);
		future.force();
	}
	else
	{
		UpdateClaimStructuresClaimMANRPC_DMA::stub(dispatch_claim, &future, octo_claim_ptr, claim);
		future.force();
	}

	return (future.getData());
}

os::agent::AgentClaim os::agent::Agent::getInitialAgentClaim(void *)
{
	GetInitialAgentClaimMANRPC_DMA::FType future;
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		GetInitialAgentClaimMANRPC_DMA instance(os::res::DispatchClaim::getOwnDispatchClaim(), &future, 0);
		future.init();
		instance.impl(NULL);
		future.force();
	}
	else
	{
		GetInitialAgentClaimMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, NULL);
		future.force();
	}
	os::agent::AgentClaim result = future.getData();
/* --- JK: Telemetry for INVADE ---*/
#ifdef cf_gui_enabled
	MetricAgentInvade m(&result);
	m.isInitialClaim = 1;
	MetricSender::measureMetric(m);
#endif
	/* Telemetry End */
	return result;
}

void os::agent::Agent::setLEDs(uint32_t bits)
{

	if (hw::hal::Tile::getTileID() == 0)
	{
		SetLEDsNARPC instance(hw::hal::Tile::getTileID());
		instance.impl(bits);
	}
	else
	{
		SetLEDsNARPC::stub(0, bits);
	}
}

void os::agent::Agent::signalOctoPOSConfigDone(os::agent::AgentInstance *ag, uint32_t claimNr)
{
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		SignalOctoPOSConfigDoneNARPC instance(hw::hal::Tile::getTileID());
		instance.impl(ag, claimNr);
	}
	else
	{
		SignalOctoPOSConfigDoneNARPC::stub(os::agent::AgentSystem::AGENT_TILE, ag, claimNr);
	}
}

bool os::agent::Agent::pure_retreat(os::agent::AgentInstance *ag, uint32_t claim_no)
{
	DBG(SUB_AGENT, "os::agent::Agent::pure_retreat(%p)\n", ag);
// JK: Remember the ID of the Agent here so that we can monitor it.
#ifdef cf_gui_enabled
	int agent_id = ag->get_id();
#endif
	PureRetreatAgentRPC::FType future;
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		PureRetreatAgentRPC instance(os::res::DispatchClaim::getOwnDispatchClaim());
		future.init();
		future.signal(instance.impl(ag, claim_no));
	}
	else
	{
		PureRetreatAgentRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag, claim_no);
		future.force();
	}
	/* --- JK: Telemetry for RETREAT---*/
#ifdef cf_gui_enabled
	MetricAgentRetreat m(claim_no, agent_id);
	MetricSender::measureMetric(m);
#endif
	/* --- JK Telemetry End ---*/
	return (future.getData());
}

bool os::agent::Agent::checkAlive(os::agent::AgentInstance *ag)
{
	CheckAgentAliveRPC::FType future;
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		CheckAgentAliveRPC instance(os::res::DispatchClaim::getOwnDispatchClaim());
		future.init();
		future.signal(instance.impl(ag));
	}
	else
	{
		CheckAgentAliveRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag);
		future.force();
	}
	return future.getData(); // returns the same as checkAlive to avoid unneccessary RPC's
} //AgentGetName

int os::agent::Agent::getAgentName(os::agent::AgentInstance *ag, char buffer[], size_t size)
{
	if (!ag)
		panic("AgentRPCClient::getAgentName:: AgentInstance *ag == NULL");

	AgentGetNameRPC::FType future;
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		AgentGetNameRPC instance(os::res::DispatchClaim::getOwnDispatchClaim());
		future.init();
		future.signal(instance.impl(ag, buffer, size));
	}
	else
	{
		AgentGetNameRPC::stub(os::agent::AgentSystem::AGENT_TILE, &future, ag, buffer, size);
		future.force();
	}
	return future.getData();
}

void os::agent::Agent::setAgentName(os::agent::AgentInstance *ag, const char *agent_name)
{
	if (!ag)
		panic("AgentRPCClient::setAgentName:: AgentInstance *ag == NULL");
/* -- Telemetry for Name Change --
		Send out Message before RPC Call to make sure
		Name change reaches GUI before program ends.
	*/
#ifdef cf_gui_enabled
	uint8_t id = ag->get_id();
	MetricAgentRename m(id, agent_name);
	MetricSender::measureMetric(m);
#endif
	/* -- Telemetry end -- */
	if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE)
	{
		AgentSetNameNARPC instance(hw::hal::Tile::getTileID());
		instance.impl(ag, agent_name);
	}
	else
	{
		AgentSetNameNARPC::stub(os::agent::AgentSystem::AGENT_TILE, ag, agent_name);
	}
}

// ProxyAgentOctoClaim related RPCs. See AgentRPCClient.h for code documentation.
// All of them use DMA RPCs to avoid RPC parameter size problems.

uintptr_t os::agent::Agent::proxyAOC_get_AOC_address(int objects_tile, uint32_t octo_ucid)
{
	ProxyAOCGetAddressMANRPC_DMA::FType future;
	ProxyAOCGetAddressMANRPC_DMA::stub(os::agent::AgentSystem::getDispatchClaim(), &future, objects_tile, octo_ucid); // crashes when AOC has been retreated
	future.force();
	return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_get_ResourceCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	ProxyAOCGetResourceCountMANRPC_DMA::FType future;
	ProxyAOCGetResourceCountMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
	future.force();
	return future.getData();
}

void *os::agent::Agent::proxyAOC_print(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	ProxyAOCPrintMANRPC_DMA::FType future;
	ProxyAOCPrintMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
	future.force();
	return future.getData();
}

bool os::agent::Agent::proxyAOC_isEmpty(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	ProxyAOCIsEmptyMANRPC_DMA::FType future;
	ProxyAOCIsEmptyMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
	future.force();
	return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_getQuantity(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, os::agent::TileID tileID, os::agent::HWType type)
{
	ProxyAOCGetQuantityMANRPC_DMA::FType future;
	ProxyAOCGetQuantityMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address, tileID, type);
	future.force();
	return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_getTileCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	ProxyAOCGetTilecountMANRPC_DMA::FType future;
	ProxyAOCGetTilecountMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
	future.force();
	return future.getData();
}

uint8_t os::agent::Agent::proxyAOC_getTileID(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, uint8_t iterator)
{
	ProxyAOCGetTileIDMANRPC_DMA::FType future;
	ProxyAOCGetTileIDMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address, iterator);
	future.force();
	return future.getData();
}

int os::agent::Agent::proxyAOC_reinvadeSameConstraints(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	ProxyAOCReinvadeSameConstrMANRPC_DMA::FType future;
	ProxyAOCReinvadeSameConstrMANRPC_DMA::stub(dispatch_claim, &future, agentoctoclaim_address);
	future.force();
	return future.getData();
}
