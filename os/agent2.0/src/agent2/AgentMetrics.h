#ifndef AGENT_METRICS
#define AGENT_METRICS

#include "AgentID.h"
#include "Agent2.h"
#include "AgentClaim.h"
#include "AgentSerialization.h"
#include "HashMap.h"

#include <stdint.h>

/* Class to process the system metrics */
namespace os
{
    namespace agent2
    {
        class AgentMetrics
        {
        private:
            static SerializationBuffer metric_buffer;
            static SerializationBuffer cluster_buffer;
            static uint32_t buffer_size;
            static uint32_t cluster_buffer_size;
            static uint64_t start_timer;
            static uint64_t stop_timer;
            static bool metrics_enabled; 

        public:
            static void enable_metrics();
            static void new_agent(const AgentID &agent_id);
            static void delete_agent(const AgentID &agent_id);
            static void invade_agent(const AgentID &agent_id, const ClaimID &claim_id);
            static void retreat_agent(const AgentID &agent_id, const ClaimID &claim_id);
            static void new_cluster(const int &tile, const int &enabled);

            static void metrics_timer_init();
            static uint64_t metrics_timer_start();
            static uint64_t metrics_timer_stop();

            static void general_metrics(uint8_t &options);
            static void cluster_metrics();
            static void timer_metrics();
            static void claim_metrics(Agent &agent, const ClaimID &id);
            static void print_metrics(Agent &agent, const ClaimID &id, uint8_t &options);
        };
    } // namespace agent2
} // namespace os

#endif