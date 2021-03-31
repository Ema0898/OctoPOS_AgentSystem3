/**
 * Project Untitled
 */

#include "AgentClusterManager.h"
#include "Agent2.h"
#include "lib/debug.h"
#include "os/rpc/RPCStub.h"
#include "AgentMemory.h"

#ifdef cf_agent2_metrics_custom
#include "AgentMetrics.h"
#endif

#include <stdio.h>

using namespace os::agent2;
AgentClusterManager *LocalAgentClusterManager = nullptr;

AgentClusterManager::AgentClusterManager(void) : m_TileID(hw::hal::Tile::getTileID()), m_is_active(is_running_on_a_cluster_Tile())
{
	m_TileID = hw::hal::Tile::getTileID();
	m_is_active = is_running_on_a_cluster_Tile();
	DBG_RAW(SUB_AGENT, "AgentClusterManager was started on Tile %d. it is marked  as %s\n", m_TileID, m_is_active ? "[ACTIVE]" : "[NOT ACTIVE]");

	#ifdef cf_agent2_metrics_custom
	// AgentMetrics::new_cluster(hw::hal::Tile::getTileID());
	if (m_is_active)
	{
		AgentMetrics::new_cluster(hw::hal::Tile::getTileID());
	}
	#endif
}

AgentClusterManager::~AgentClusterManager()
{
	/// @todo: free list of registered Agents
	/// @todo: free list of landlord Agents
}

void *AgentClusterManager::operator new(size_t size)
{
	return AgentMemory::agent_mem_allocate(size);
}

void AgentClusterManager::operator delete(void *p)
{
	AgentMemory::agent_mem_free(p);
}

bool AgentClusterManager::register_local_Agent(AgentID id, TID tileid)
{
	bool ret;
	CI<Agent> *ci_ptr = new CI<Agent>;
	if (!ci_ptr)
		panic("os::agent2::AgentClusterManager::register_local_Agent:: allocation of CI failed - out of memory\n");
	/*
	 * Note:
	 * this is the reason we are using CI instead of TID for the registration process.
	 * We already have an allocator for CI but no allocator for TID.
	 */
	ci_ptr->set_instance_identifier(id);
	ci_ptr->set_TileID(tileid);
	ret = m_cluster_local_agents.insert(id, ci_ptr);
	if (ret)
		delete (ci_ptr);
	DBG(SUB_AGENT, "Registering Agent with AgentID: (%u,%u) at AgentClusterManager[%d]: %s\n", id.m_TileID, id.m_TileUniqueAgentID, m_TileID, ret ? "[FAILED]" : "[SUCCEEDED]");
	return (ret);
}

bool AgentClusterManager::unregister_local_Agent(AgentID id)
{
	bool ret = m_cluster_local_agents.erase(id);
	DBG(SUB_AGENT, "Unregistering Agent with AgentID: (%u,%u) at AgentClusterManager[%d]: %s\n", id.m_TileID, id.m_TileUniqueAgentID, m_TileID, ret ? "[FAILED]" : "[SUCCEEDED]");
	return (ret);
}

TID AgentClusterManager::find_cluster_local_Agent(AgentID id)
{
	DBG(SUB_AGENT, "Trying to find Agent with id (%u/%u) in AgentClusterManager[%u]\n", id.m_TileID, id.m_TileUniqueAgentID, m_TileID);
	auto it = m_cluster_local_agents.find(id);
	if (it == m_cluster_local_agents.get_list_end_itr())
		return (UNKNOWN_TID);
	return ((it->value_ptr)->get_TileID());
}

bool AgentClusterManager::register_landlord_Agent(AgentID id, TID tileid)
{
	bool ret;
	CI<Agent> *ci_ptr = new CI<Agent>;
	if (!ci_ptr)
		panic("os::agent2::AgentClusterManager::register_landlord_Agent::allocation of CI failed - out of memory\n");
	ci_ptr->set_instance_identifier(id);
	ci_ptr->set_TileID(tileid);
	ret = m_cluster_landlord_agents.Insert(id, ci_ptr);
	DBG(SUB_AGENT, "Registering LandlordAgent with AgentID: (%u,%u) currently locatey on tile %u at AgentClusterManager[%d]: %s\n", id.m_TileID, id.m_TileUniqueAgentID, tileid, m_TileID, ret ? "[FAILED]" : "[SUCCEEDED]");
	return (ret);
}

bool AgentClusterManager::unregister_landlord_Agent(AgentID id)
{
	bool ret = m_cluster_landlord_agents.Delete(id);
	DBG(SUB_AGENT, "Unregistering LandlordAgent with AgentID: (%u,%u) at AgentClusterManager[%d]: %s\n", id.m_TileID, id.m_TileUniqueAgentID, m_TileID, ret ? "[FAILED]" : "[SUCCEEDED]");
	return (ret);
}

unsigned int AgentClusterManager::get_landlord_Agents(unsigned int max_nr_of_agents, void *target_address)
{
	unsigned int counter = 0;
	CI<Agent> *target = (CI<Agent> *)target_address;

	for (auto it = m_cluster_landlord_agents.begin(); it != m_cluster_landlord_agents.end() && counter < max_nr_of_agents; ++it, counter++)
	{
		target[counter] = *it->value_ptr;
	}
	return counter;
}

bool AgentClusterManager::is_running_on_a_cluster_Tile(void)
{
	return (true == ClusterTiles[m_TileID]);
}

bool AgentClusterManager::is_AgentClusterManager(TID TileID)
{
	/*TODO: check input against maximum tileid as soon as such a thing is existing*/
	TID itr = TileID;
	if (itr++ == m_TileID)
		return true;

	for (;; ++itr)
	{
		if (ClusterTiles[itr])
			return (false);

		if (itr == m_TileID)
			break;
	}
	return (true);
}
