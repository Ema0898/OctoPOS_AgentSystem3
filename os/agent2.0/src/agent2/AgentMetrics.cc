#include "AgentMetrics.h"
#include "AgentSerialization.h"
#include "AgentID.h"
#include "hw/hal/Tile.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define TID_BITS (8 * sizeof(TID))
#define UID_BITS (8 * sizeof(uint32_t) - TID_BITS)
#define UID_MASK ((1 << UID_BITS) - 1)

using namespace os::agent2;

SerializationBuffer metric_buffer;
uint32_t buffer_size;

void AgentMetrics::new_agent(AgentID *agent_id)
{
  uint32_t agentID = (agent_id->m_TileID << UID_BITS) | (agent_id->m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 20, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);

  buffer_size += 2;
}

void AgentMetrics::delete_agent(AgentID *agent_id)
{
  uint32_t agentID = (agent_id->m_TileID << UID_BITS) | (agent_id->m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 21, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);

  buffer_size += 2;
}

void AgentMetrics::invade_agent(AgentID *agent_id, ClaimID *claim_id)
{
  uint32_t agentID = (agent_id->m_TileID << UID_BITS) | (agent_id->m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 22, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, *claim_id, 1);

  buffer_size += 3;
}

void AgentMetrics::retreat_agent(AgentID *agent_id, ClaimID *claim_id)
{
  uint32_t agentID = (agent_id->m_TileID << UID_BITS) | (agent_id->m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, 23, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, 1);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, *claim_id, 1);

  buffer_size += 3;
}

void AgentMetrics::print_metrics()
{
  int buffer_value = 0;
  int counter = 0;
  printf("$$$$$ Start printing metrics $$$$$\n");
  while (counter < buffer_size)
  {
    buffer_value = metric_buffer.data[counter];

    if (buffer_value == 20)
    {
      printf("---<metric>--- An Agent with id = %d was created\n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == 21)
    {
      printf("---<metric>--- An Agent with id = %d was deleted\n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == 22)
    {
      printf("---<metric>--- An Agent with id = %d made an invasion in claim = %d\n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
    else if (buffer_value == 23)
    {
      printf("---<metric>--- An Agent with id = %d made a retreat in claim = %d\n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
  }

  printf("$$$$$ Finished printing metrics $$$$$\n");
}