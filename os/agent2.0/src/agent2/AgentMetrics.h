#ifndef AGENT_METRICS
#define AGENT_METRICS

#include "AgentID.h"
#include "AgentClaim.h"

#include <stdarg.h>

namespace os
{
    namespace agent2
    {
        class AgentMetrics
        {
        public:
            static void new_agent(AgentID *agent_id);
            static void delete_agent(AgentID *agent_id);
            static void invade_agent(AgentID *agent_id, ClaimID *claim_id);
            static void retreat_agent(AgentID *agent_id, ClaimID *claim_id);
            static void print_metrics();
        };
    } // namespace agent2
} // namespace os

#endif