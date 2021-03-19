/** @file AgentClusterManager.h
 *  @brief Main Interface description for the AgentClusterManager
 *
 *  @author Thorsten Quadt (uni@thorstenquadt.de)
 *  @bug No known bugs.
 */

#ifndef OS_AGENT2_AGENTCLUSTERMANAGER_H
#define OS_AGENT2_AGENTCLUSTERMANAGER_H
#include <stddef.h>
#include "config.h"
#include "CI.h"
#include "AgentID.h"
#include "common.h"



namespace os {
namespace agent2 {
/*Forward declarations*/
class Agent;
/**
 * @class AgentClusterManager
 * @brief Manages all Agents located in its Cluster
 * @details The AgentClusterManager manages all Agents that are either located in
 * 			its Cluster or own at least one resource inside the cluster.
 * 			A Cluster consists of one or more tiles.
 * 			All AgentTileManagers that are located inside a cluster report new Agents to the AgentClusterManager.
 * 			Agents that own a resource inside the cluster register themselves as so-called Landlord Agents
 * 			AgentTileManagers (inside as well as outside the cluster) can use the AgentClusterManager to find Agents by their unique AgentID
 * 			AgentTileManagers (inside as well as outside the cluster) can use the AgentClusterManager to find Agents that are holding Resources on a tile inside the Cluster.
 * 			Every AgentClusterManager can be ask if it is the AgentClusterManager for a dedicated Tile.
 * 			At the moment this feature is unused, it can be used later in order to allow non uniform clusters.
 */
class AgentClusterManager {

public: 
	/**
	 * @brief The default constructor of the AgentClusterManager
	 * @details The default constructor checks queries the TileID it is currently running on.
	 * 			It then determines whether or not an AgentClusterManager should be running on the current Tile (using the is_running_on_a_cluster_Tile function).
	 * 			It sets the member variable m_is_active accordingly.
	 * 			In cases where the AgentClusterManager is created on a Tile marked as a Cluster Tile, the AgentClusterManager works as specified.
	 * 			In cases where the AgentClusterManager is created on a Tile not marked as an Cluster Tile, the AgentClusterManager does response to every
	 * 			every attempt of communication with a NULL answer.
	 * 			The only exception is the is_AgentClusterManager(TID TileID) this function will always return FALSE.
	 */
	AgentClusterManager();
	~AgentClusterManager();

	void* operator new(size_t size);
	void operator delete(void *p);

	/**
	 * @brief Used to register new cluster-local Agents to the AgentClusterManager.
	 * @details This function is called by the AgentTileManager it should never be called directly by the Agents.
	 * @param[in] id: Unique AgentID of the Agent that shall be registered
	 * @param[in] tileid: TileID of the tile the Agent can be found on
	 * @return returns ERROR(true) in case of an error and SUCCESS(false) in case the registration was successful
	 *
	 */
	bool register_local_Agent(AgentID id, TID tileid);

	/**
	 * @brief Used to unregister a cluster-local Agents from the AgentClusterManager.
	 * @details This function is called by the AgentTileManager it should never be called directly by the Agents.
	 * @param[in] id: unique AgentID of the Agent that shall be registered
	 * @return returns ERROR(true) in case of an error and SUCCESS(false) in case the registration was successful
	 */
	bool unregister_local_Agent(AgentID id);
	/**
	 * @brief Used to register an Agent to the AgentClusterManager that owns resources inside the cluster of the AgentClusterManager.
	 * @details This function is called by the AgentTileManager it should never be called directly by the Agents.
	 * @param[in] id: Unique AgentID of the Agent that shall be registered
	 * @param[in] tileid: TileID of the tile the Agent can be found on
	 * @return returns ERROR(true) in case of an error and SUCCESS(false) in case the registration was successful
	 */
	bool register_landlord_Agent(AgentID id, TID tileid);
	
	/**
	 * @brief Used to unregister an Agent to the AgentClusterManager that owns resources inside the cluster of the AgentClusterManager.
	 * @details This function is called by the AgentTileManager it should never be called directly by the Agents.
	 * @param[in] id: unique AgentID of the Agent that shall be registered
	 * @return returns ERROR(true) in case of an error and SUCCESS(false) in case the registration was successful
	 */
	bool unregister_landlord_Agent(AgentID id);
	/**
	 * @brief Returns whether or not this AgentClusterManager instance is responsible for the a given TileID
	 * @param[in] TileID: TileID
	 * @return Returns a bool value whether or not this AgentClusterManager instance is responsible for the a given TileID
	 * @return returns ERROR(true) in case of an error and SUCCESS(false) in case the registration was successful
	 */
	bool is_AgentClusterManager(TID TileID);
	
	/**
	 * @brief Search for an Agent with AgentID id in the local cluster
	 * @details Note: this information might be outdated.
	 * @param[in] id: AgentID of the Agent
	 * @return returns the TileID on which the Agent is located or NOTILE if the AgentID can not be found inside this cluster.
	 */
	TID find_cluster_local_Agent(AgentID id);

	/**
	 * @brief find up to max_nr_of_agents that are holding resources on tile tileid
	 * @param[in] max_nr_of_agents: Maximum Number of Agents that shall be returned
	 * @param[out] target_address: Target address where the Data should be copied to
	 * @return Number of actually copied Agent Informations
	 *
	 */
	unsigned int get_landlord_Agents(unsigned int max_nr_of_agents, void *target_address);

private: 

	TID m_TileID; ///< The TileID of the Tile this AgentClusterManager instance is running on
	bool m_is_active; ///< saves the state whether or not the current instance of the AgentClusterManager is running on a cluster tile
	bool is_running_on_a_cluster_Tile(void); ///< is this instance of the AgentClusterManager is running on a cluster tile
	os::libkit::LfHashMap<os::agent2::AgentID,os::agent2::CI<Agent>, 16> m_cluster_local_agents;
	os::libkit::LfList<os::agent2::CI<Agent>,os::agent2::AgentID> m_cluster_landlord_agents;
};

} //namespace agent2
} //namespace os
#endif // OS_AGENT2_AGENTCLUSTERMANAGER_H
