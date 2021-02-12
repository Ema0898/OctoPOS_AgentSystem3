#ifndef METRIC_NEW_AGENT_H
#define METRIC_NEW_AGENT_H

#include "os/agent3/Metric.h"

namespace os
{
    namespace agent
    {

        class MetricNewAgent : public Metric
        {
        public:
            MetricNewAgent();
            MetricNewAgent(uint8_t agentId);
            char *package();

            uint8_t agentId;

        private:
            void init();

            const int metricId = 20;
            int timestamp;
        };
    } // namespace agent
} // namespace os

#endif
