#include "AgentMetrics.h"
#include "lib/adt/SimpleSpinlock.h"
#include "MetricsEnum.h"
#include "AgentMetricsPrinter.h"
#include "hw/dev/TSCDeadlineTimer.h"
#include <string.h>

/* Macros to calculate the agent id */
constexpr int TID_BITS = 8 * sizeof(TID);
constexpr int UID_BITS = 8 * sizeof(uint32_t) - TID_BITS;
constexpr int UID_MASK = (1 << UID_BITS) - 1;
constexpr int SIZE_BYTE = 1;

using namespace os::agent2;

SerializationBuffer AgentMetrics::metric_buffer; /* Buffer to store the metrics */
uint32_t AgentMetrics::buffer_size;              /* Keep track of the buffer size */
uint64_t AgentMetrics::start_timer;
uint64_t AgentMetrics::stop_timer;

SerializationBuffer AgentMetrics::cluster_buffer;
uint32_t AgentMetrics::cluster_buffer_size;

lib::adt::SimpleSpinlock locker;

bool AgentMetrics::metrics_enabled = false;

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

void AgentMetrics::new_cluster(const int &tile, const int &enabled)
{
  locker.lock();

  SerializationBuffer *buffer = hw::hal::Tile::onRemoteTile(&cluster_buffer, 0);
  uint32_t *clusters_size = hw::hal::Tile::onRemoteTile(&cluster_buffer_size, 0);

  SerializableAgent::serialize_element<uint32_t>(*buffer, tile, SIZE_BYTE);
  SerializableAgent::serialize_element<uint32_t>(*buffer, enabled, SIZE_BYTE);

  //hw::hal::Atomic::addFetch(clusters_size, 2);
  *clusters_size += 2;

  locker.unlock();
}

void AgentMetrics::enable_metrics()
{
  metrics_enabled = true;
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

void AgentMetrics::general_metrics(uint8_t &options)
{
  if (!metrics_enabled)
    return;

  AgentMetricsPrinter::print_general_metrics(metric_buffer, buffer_size, options);
}

void AgentMetrics::cluster_metrics()
{
  if (!metrics_enabled)
    return;

  int counter = 0;
  int clusters[cluster_buffer_size];

  while (counter < cluster_buffer_size)
  {
    int buffer_value_tile = cluster_buffer.data[counter];
    int buffer_value_enabled = cluster_buffer.data[counter + 1];

    clusters[buffer_value_tile] = buffer_value_enabled;

    counter += 2;
  }

  AgentMetricsPrinter::print_cluster_metrics(clusters);
}

void AgentMetrics::timer_metrics()
{
  if (!metrics_enabled)
    return;

  uint64_t time;

  if (start_timer <= 0 || stop_timer <= 0)
  {
    time = -1;
  }

  time = stop_timer - start_timer;

  AgentMetricsPrinter::print_timer_metrics(time);
}

void AgentMetrics::claim_metrics(Agent &agent, const ClaimID &id)
{
  if (!metrics_enabled)
    return;

  int used_cores = 0;

  auto map = agent.get_agent_claims();
  auto claim = map->find(id);

  int claim_arr_size = hw::hal::Tile::getTileCount() * os::dev::HWInfo::Inst().getCoreCount();
  int claim_arr[claim_arr_size];
  memset(claim_arr, 0, claim_arr_size * sizeof(int));

  auto it = (*claim).value->begin();

	while(it != (*claim).value->end())
  {
		for(unsigned int i = 0 ; i < os::dev::HWInfo::Inst().getCoreCount(); i++)
    {
      if ((*it).value.res_info[i].get_claimed())
      {
        claim_arr[(*it).key * os::dev::HWInfo::Inst().getCoreCount() + i] = 15;
        used_cores++;
      }
		}
		++it;
	}

  AgentMetricsPrinter::print_claim_metrics(claim_arr, used_cores);
}


/* Prints the data which is inside the buffer. */
void AgentMetrics::print_metrics(Agent &agent, const ClaimID &id, uint8_t &options)
{
  general_metrics(options);
  cluster_metrics();
  timer_metrics();
  claim_metrics(agent, id);
}