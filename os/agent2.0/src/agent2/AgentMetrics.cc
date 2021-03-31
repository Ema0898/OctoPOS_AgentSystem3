#include "AgentMetrics.h"
#include "lib/adt/SimpleSpinlock.h"
#include "MetricsEnum.h"
#include "AgentMetricsPrinter.h"
#include "hw/dev/TSCDeadlineTimer.h"

/* Macros to calculate the agent id */
constexpr int TID_BITS = 8 * sizeof(TID);
constexpr int UID_BITS = 8 * sizeof(uint32_t) - TID_BITS;
constexpr int UID_MASK = (1 << UID_BITS) - 1;
constexpr int SIZE_BYTE = 1;

using namespace os::agent2;

SerializationBuffer AgentMetrics::metric_buffer; /* Buffer to store the metrics */
uint32_t AgentMetrics::buffer_size;              /* Keep track of the buffer size */
int AgentMetrics::clusters;
uint64_t AgentMetrics::start_timer;
uint64_t AgentMetrics::stop_timer;

lib::adt::SimpleSpinlock locker;

int cluster_arr[4];

/* New agent is created. Stores the agent id in the buffer */
void AgentMetrics::new_agent(const AgentID &agent_id)
{
  /* Calculates the agent id */
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  /* Put data inside the buffer */
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, static_cast<int>(METRICS::NEW), SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, SIZE_BYTE);

  /* Increase buffer size */
  buffer_size += 2;
  //printf("$$$ New Agent Created on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

/* Agent is deleted. Stores the agent id in the buffer */
void AgentMetrics::delete_agent(const AgentID &agent_id)
{
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, static_cast<int>(METRICS::DELETE), SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, SIZE_BYTE);

  buffer_size += 2;
  //printf("$$$ Agent Deleted on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

/* Agent made an invasion. Stores the agent id and the claim in the buffer */
void AgentMetrics::invade_agent(const AgentID &agent_id, const ClaimID &claim_id)
{
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, static_cast<int>(METRICS::INVADE), SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, claim_id, SIZE_BYTE);

  buffer_size += 3;
  //printf("$$$ Agent made Invasion on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

/* Agent made a retreat. Stores the agent id and the claim in the buffer */
void AgentMetrics::retreat_agent(const AgentID &agent_id, const ClaimID &claim_id)
{
  uint32_t agentID = (agent_id.m_TileID << UID_BITS) | (agent_id.m_TileUniqueAgentID & UID_MASK);

  SerializableAgent::serialize_element<uint32_t>(metric_buffer, static_cast<int>(METRICS::RETREAT), SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, agentID, SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(metric_buffer, claim_id, SIZE_BYTE);

  buffer_size += 3;
  //printf("$$$ Agent made a Retreat on Tile = %d $$$\n", hw::hal::Tile::getTileID());
}

void AgentMetrics::new_cluster(int tile)
{
  locker.lock();
  int *cluster_address = hw::hal::Tile::onRemoteTile(&clusters, 0);
  int *clusters = hw::hal::Tile::onRemoteTile(cluster_arr, 0);
  hw::hal::Atomic::addFetch(cluster_address, 1);
  clusters[tile] = 1;
  locker.unlock();
}

void AgentMetrics::enable_metrics()
{
  AgentMetricsPrinter::enable_metrics();
}

void AgentMetrics::metrics_timer_init()
{
  hw::dev::TSCDeadlineTimer::init();
}

uint64_t AgentMetrics::metrics_timer_start()
{
  //hw::dev::TSCDeadlineTimer::init();
  start_timer = uint64_t(hw::dev::TSCDeadlineTimer::getCyclesStart());
  return start_timer;
}

uint64_t AgentMetrics::metrics_timer_stop()
{
  stop_timer = uint64_t(hw::dev::TSCDeadlineTimer::getCyclesStop());
  return stop_timer;
}

/* Prints the data which is inside the buffer. */
void AgentMetrics::print_metrics(uint8_t &options)
{
  uint64_t time;

  if (start_timer <= 0 || stop_timer <= 0)
  {
    time = -1;
  }

  time = stop_timer - start_timer;

  // for (int i = 0; i < 4; ++i)
  // {
  //   printf("CLUSTER ARRAY [%d] = %d\n", i, cluster_arr[i]);
  // }
  AgentMetricsPrinter::basic_print(metric_buffer, buffer_size, options, time, cluster_arr);
}