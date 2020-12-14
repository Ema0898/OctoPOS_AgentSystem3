#include "os/agent3/MetricSystemArchitecture.h"

#include <stdlib.h>
#include <octo_clock.h>
#include "os/agent3/AgentMemory.h"
#include "os/dev/HWInfo.h"

void os::agent::MetricSystemArchitecture::init()
{
    timestamp = 0; // Set this artficially to zero because it really should be
                   // the first information the consumer (visualization)
                   // should process
    numberOfTiles = hw::hal::Tile::MAX_TILE_COUNT;
    numberOfRessourcesPerTile = os::agent::MAX_RES_PER_TILE;
    sessionId = clock();
    agentTileId = os::agent::AgentSystem::AGENT_TILE;
    ioTileId = hw::hal::Tile::getIOTileID();
    dev::HWInfo &hwinfo = dev::HWInfo::Inst();
    cols = hwinfo.getSizeX();
    rows = hwinfo.getSizeY();
#ifdef cf_hw_tile_icore_support
    for (int tid = 0; tid < numberOfTiles; tid++)
    {
        for (int core = 0; core < numberOfRessourcesPerTile; core++)
        {
            if (os::agent::HardwareMap[tid][core].type == ResType::iCore)
            {
                iCoreTile = tid;
                iCoreProc = core;
            }
        }
    }
#else
    iCoreTile = numberOfTiles + 1;
    iCoreProc = numberOfRessourcesPerTile + 1;
#endif
#if cf_hw_tile_tcpa_support
    for (int tid = 0; tid < numberOfTiles; tid++)
    {
        for (int core = 0; core < numberOfRessourcesPerTile; core++)
        {
            if (os::agent::HardwareMap[tid][core].type == ResType::TCPA)
            {
                tcpaTile = tid;
                break;
            }
        }
    }
#else
    tcpaTile = numberOfTiles + 1;
#endif
}

os::agent::MetricSystemArchitecture::MetricSystemArchitecture()
{
    init();
}

char *os::agent::MetricSystemArchitecture::package()
{
    char *buffer = (char *)os::agent::AgentMemory::agent_mem_allocate(getPackageSize());
    if (buffer == nullptr)
    {
        panic("agent_mem_allocate failed");
    }
    int payloadSize = 3 * sizeof(int32_t);
    int payloadStartIndex = packageHelper(buffer, metricId, timestamp, payloadSize);

    flatten(numberOfTiles, buffer, payloadStartIndex);
    flatten(numberOfRessourcesPerTile, buffer, payloadStartIndex + sizeof(int));
    flatten(sessionId, buffer, payloadStartIndex + sizeof(int) * 2);
    flatten(agentTileId, buffer, payloadStartIndex + sizeof(int) * 3);
    // agentProcessor, + 16
    flatten(ioTileId, buffer, payloadStartIndex + sizeof(int) * 5);
    // ioProcessor, + 24
    flatten(iCoreTile, buffer, payloadStartIndex + sizeof(int) * 6);
    flatten(iCoreProc, buffer, payloadStartIndex + sizeof(int) * 7);
    flatten(tcpaTile, buffer, payloadStartIndex + sizeof(int) * 8);
    flatten(cols, buffer, payloadStartIndex + sizeof(int) * 9);
    flatten(rows, buffer, payloadStartIndex + sizeof(int) * 10);

    return buffer;
}
