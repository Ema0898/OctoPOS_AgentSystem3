#ifndef AGENT_METRICS
#define AGENT_METRICS

#include "AgentID.h"
#include "AgentClaim.h"

class AgentID;
class ClaimID;

/* Class to print system metrics */

namespace os
{
    namespace agent2
    {
        class AgentMetrics
        {
        public:
            static void new_agent(const AgentID &agent_id);
            static void delete_agent(const AgentID &agent_id);
            static void invade_agent(const AgentID &agent_id, const ClaimID &claim_id);
            static void retreat_agent(const AgentID &agent_id, const ClaimID &claim_id);
            static void new_cluster();
            static void print_metrics();
        };
    } // namespace agent2
} // namespace os

#endif