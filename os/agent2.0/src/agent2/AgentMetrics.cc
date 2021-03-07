#include "AgentMetrics.h"
#include "AgentSerialization.h"
#include "config.h"

#include <stdio.h>
#include <stdint.h>

/* Macros to calculate the agent id */
constexpr int TID_BITS = 8 * sizeof(TID);
constexpr int UID_BITS = 8 * sizeof(uint32_t) - TID_BITS;
constexpr int UID_MASK = (1 << UID_BITS) - 1;

using namespace os::agent2;

SerializationBuffer metric_buffer; /* Buffer to store the metrics */
uint32_t buffer_size;              /* Keep track of the buffer size */
int clusters;

/* New agent is created. Stores the agent id in the buffer */
void AgentMetrics::new_agent(const AgentID &agent_id)
{
  /* Calculates the agent id */
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  /* Put data inside the buffer */
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 20, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);

  /* Increase buffer size */
  buffer_size += 2;
  //printf("$$$ New Agent Created on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

/* Agent is deleted. Stores the agent id in the buffer */
void AgentMetrics::delete_agent(const AgentID &agent_id)
{
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 21, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);

  buffer_size += 2;
  //printf("$$$ Agent Deleted on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

/* Agent made an invasion. Stores the agent id and the claim in the buffer */
void AgentMetrics::invade_agent(const AgentID &agent_id, const ClaimID &claim_id)
{
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 22, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, claim_id, 1);

  buffer_size += 3;
  //printf("$$$ Agent made Invasion on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

/* Agent made a retreat. Stores the agent id and the claim in the buffer */
void AgentMetrics::retreat_agent(const AgentID &agent_id, const ClaimID &claim_id)
{
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 23, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, claim_id, 1);

  buffer_size += 3;
  //printf("$$$ Agent made a Retreat on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

void AgentMetrics::new_cluster()
{
  int *cluster_address = hw::hal::Tile::onRemoteTile(&clusters, 0);
  *cluster_address += 1;
  // hw::hal::Atomic::addFetch(cluster_address, 1);

  // printf("???? Calling new cluster metric from tile %d with clusters = %d ????\n", hw::hal::Tile::getTileID(), clusters);
  // printf("???? Cluster Address = %p in tile %d ????\n", cluster_address, hw::hal::Tile::getTileID());
}

/* Prints the data which is inside the buffer. */
void AgentMetrics::print_metrics()
{
  int buffer_value = 0;
  int counter = 0;
  printf("$$$$$ Start printing metrics $$$$$\n");

  int *dir = nullptr;

  /* Prints the actives agent clusters */
  for (int i = 0; i < hw::hal::Tile::getTileCount(); ++i)
  {
    if (ClusterTiles[i])
    {
      // dir = hw::hal::Tile::onRemoteTile(&clusters, (TID)i);
      printf("---<metric>--- An agent cluster was started in the tile %d\n", i);
      // printf("---<metric>--- Cluster variable dir is %p on tile %d\n", dir, i);
      // printf("---<metric>--- Cluster variable is %d on tile %d\n", *dir, i);
    }
  }

  printf("Cluster is %d on tile %d\n", clusters, hw::hal::Tile::getTileID());

  while (counter < buffer_size)
  {
    buffer_value = metric_buffer.data[counter];

    /* Classifies the data based in the metric id */
    if (buffer_value == 20)
    {
      printf("---<metric>--- An Agent with id %d was created\n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == 21)
    {
      printf("---<metric>--- An Agent with id %d was deleted\n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == 22)
    {
      printf("---<metric>--- An Agent with id %d made an invasion in claim %d\n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
    else if (buffer_value == 23)
    {
      printf("---<metric>--- An Agent with id %d made a retreat in claim %d\n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
  }

  printf("$$$$$ Finished printing metrics $$$$$\n");
}