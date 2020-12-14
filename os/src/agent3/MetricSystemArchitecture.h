#ifndef METRIC_SYS_ARCH_H
#define METRIC_SYS_ARCH_H

#include "os/agent3/Metric.h"
#include "os/agent3/Platform.h"
#include "os/agent3/AgentSystem.h"

namespace os
{
    namespace agent
    {

        class MetricSystemArchitecture : public Metric
        {
        public:
            MetricSystemArchitecture();
            char *package();

            int numberOfTiles;
            int numberOfRessourcesPerTile;
            int rows;
            int cols;
            int sessionId;
            int agentTileId;
            // [TODO] int agentProcessor;
            int ioTileId;
            // [TODO] int ioProcessor;
            int iCoreTile;
            int iCoreProc;
            int tcpaTile;
            /*int tcpaCores[os::agent::MAX_RES_PER_TILE];
                For now every Tile is marked as TCPA-Core */

        private:
            void init();

            const int metricId = 10;
            int timestamp;
        };
    } // namespace agent
} // namespace os

#endif // METRIC_SYS_ARCH_H Include-Guard
