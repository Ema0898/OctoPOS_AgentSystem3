#ifndef METRIC_AGENT_RETREAT_H
#define METRIC_AGENT_RETREAT_H

#include "os/agent3/Metric.h"

namespace os
{
    namespace agent
    {

        class MetricAgentRetreat : public Metric
        {
        public:
            MetricAgentRetreat();
            MetricAgentRetreat(int claimId, int agentId);
            char *package();

            int claimId;
            int agentId;

        private:
            void init();

            const int metricId = 23;
            int timestamp;
        };
    } // namespace agent
} // namespace os

#endif // METRIC_AGENT_RETREAT_H
