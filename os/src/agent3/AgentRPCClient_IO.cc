#include "os/agent3/AgentRPCHeader.h"
#include "os/agent3/AgentRPCClient.h"
#include "os/agent3/Agent.h"
#include "os/agent3/ActorConstraint.h"

os::agent::AgentInstance *os::agent::Agent::createAgent()
{
	panic("Agent on I/O Tile");
	return NULL;
}

void os::agent::Agent::deleteAgent(os::agent::AgentInstance *ag, bool force)
{
	panic("Agent on I/O Tile");
}

os::agent::AgentClaim os::agent::Agent::invade_agent_constraints(os::agent::AgentInstance *ag, os::agent::AgentConstraint *agent_constraints)
{
	panic("invade_agent_constraints(...): Agent on I/O Tile");
	return AgentClaim();
}

os::agent::AgentClaim os::agent::Agent::reinvade_agent_constraints(os::agent::AgentInstance *ag, os::agent::AgentConstraint *agent_constraints, uint32_t old_claim_id)
{
	panic("reinvade_agent_constraints(...): Agent on I/O Tile");
	return AgentClaim();
}

os::agent::AgentClaim os::agent::Agent::invade_actor_constraints(os::agent::AgentInstance *ag, os::agent::ActorConstraint *actor_constraints)
{
	panic("invade_actor_constraints(...): Agent on I/O Tile");
	return AgentClaim();
}

void os::agent::Agent::register_AgentOctoClaim(os::agent::AgentInstance *ag, uint32_t claim_id, os::agent::AgentOctoClaim *octo_claim_ptr, os::res::DispatchClaim dispatch_claim)
{
	panic("Agent on I/O Tile");
	return;
}

void *os::agent::Agent::run_resize_handler(os::res::DispatchClaim dispatch_claim, resize_env_t resize_env_pointer, os::agent::AgentClaim &loss_claim, resize_handler_t resize_handler, size_t tile_count, size_t res_per_tile)
{
	panic("Agent on I/O Tile");

	/* Dummy return value, not actually used. */
	return (NULL);
}

void *os::agent::Agent::update_claim_structures(os::res::DispatchClaim dispatch_claim, os::agent::AgentOctoClaim *octo_claim_ptr, os::agent::AgentClaim &claim)
{
	panic("Agent on I/O Tile");

	/* Dummy return value, not actually used. */
	return (NULL);
}

os::agent::AgentClaim os::agent::Agent::getInitialAgentClaim(void *)
{
	panic("Agent on I/O Tile");
	return AgentClaim();
}

void os::agent::Agent::setLEDs(uint32_t bits)
{
}

void os::agent::Agent::signalOctoPOSConfigDone(os::agent::AgentInstance *ag, uint32_t claimNr)
{
	panic("Agent on I/O Tile");
}

bool os::agent::Agent::pure_retreat(os::agent::AgentInstance *ag, uint32_t claim_no)
{
	panic("Agent on I/O Tile");
	return false;
}

bool os::agent::Agent::checkAlive(os::agent::AgentInstance *ag)
{
	panic("Agent on I/O Tile");
	return false;
}

int os::agent::Agent::getAgentName(os::agent::AgentInstance *ag, char buffer[], size_t size)
{
	panic("Agent on I/O Tile");
	return -1;
}

void os::agent::Agent::setAgentName(os::agent::AgentInstance *ag, const char *agent_name)
{
	panic("Agent on I/O Tile");
}

// ProxyAgentOctoClaim related RPCs. See AgentRPCClient.h for code documentation.

uintptr_t os::agent::Agent::proxyAOC_get_AOC_address(int objects_tile, uint32_t octo_ucid)
{
	panic("Agent on I/O Tile");
	return (uintptr_t)0;
}

uint8_t os::agent::Agent::proxyAOC_get_ResourceCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	panic("Agent on I/O Tile");
	return (uint8_t)0;
}

void *os::agent::Agent::proxyAOC_print(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	panic("Agent on I/O Tile");
	return NULL;
}

bool os::agent::Agent::proxyAOC_isEmpty(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	panic("Agent on I/O Tile");
	return false;
}

uint8_t os::agent::Agent::proxyAOC_getQuantity(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, os::agent::TileID tileID, os::agent::HWType type)
{
	panic("Agent on I/O Tile");
	return (uint8_t)0;
}

uint8_t os::agent::Agent::proxyAOC_getTileCount(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	panic("Agent on I/O Tile");
	return (uint8_t)0;
}

uint8_t os::agent::Agent::proxyAOC_getTileID(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address, uint8_t iterator)
{
	panic("Agent on I/O Tile");
	return (uint8_t)0;
}

int os::agent::Agent::proxyAOC_reinvadeSameConstraints(os::res::DispatchClaim dispatch_claim, uintptr_t agentoctoclaim_address)
{
	panic("Agent on I/O Tile");
	return (int)-1;
}
