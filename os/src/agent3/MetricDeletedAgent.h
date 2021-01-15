#ifndef METRIC_DELETED_AGENT_H
#define METRIC_DELETED_AGENT_H

#include "os/agent3/Metric.h"

namespace os
{
    namespace agent
    {

        class MetricDeletedAgent : public Metric
        {
        public:
            MetricDeletedAgent();
            MetricDeletedAgent(uint8_t agentId);
            char *package();

            uint8_t agentId;

        private:
            void init();

            const int metricId = 21;
            int timestamp;
        };
    } // namespace agent
} // namespace os

#endif
