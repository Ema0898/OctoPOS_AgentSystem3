#ifndef AGENT_METRICS
#define AGENT_METRICS

#include "AgentID.h"
#include "AgentClaim.h"
#include "AgentSerialization.h"

#include <stdint.h>

/* Class to print system metrics */
namespace os
{
    namespace agent2
    {
        class AgentMetrics
        {
        private:
            static int clusters;
            static uint32_t buffer_size;
            static SerializationBuffer metric_buffer;
            static uint64_t start_timer;
            static uint64_t stop_timer;

        public:
            static void new_agent(const AgentID &agent_id);
            static void delete_agent(const AgentID &agent_id);
            static void invade_agent(const AgentID &agent_id, const ClaimID &claim_id);
            static void retreat_agent(const AgentID &agent_id, const ClaimID &claim_id);
            static void new_cluster(int tile);
            static void metrics_timer_init();
            static uint64_t metrics_timer_start();
            static uint64_t metrics_timer_stop();
            static void print_metrics(uint8_t &options);
        };
    } // namespace agent2
} // namespace os

#endif