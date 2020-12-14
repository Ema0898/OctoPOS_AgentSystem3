#ifndef METRIC_AGENT_INVADE_H
#define METRIC_AGENT_INVADE_H

#include "os/agent3/Metric.h"
#include "os/agent3/Platform.h"
#include "hw/hal/Tile.h"

namespace os
{
    namespace agent
    {

        class MetricAgentInvade : public Metric
        {
        public:
            MetricAgentInvade();
            MetricAgentInvade(AgentClaim *agentClaim);
            char *package();

            int agentId;
            int claimId;
            int isInitialClaim; // Interpreted as Bool, but for transport
                                // uniform use of Integers is convenient
                                //hw::hal::Tile::MAX_TILE_COUNT
            uintptr_t resourceMaps[hw::hal::Tile::MAX_TILE_COUNT];

        private:
            void init();

            const int metricId = 22;
            int timestamp;
        };
    } // namespace agent
} // namespace os

#endif // METRIC_AGENT_INVADE_H
