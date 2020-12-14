#include "os/agent3/MetricAgentInvade.h"

#include <octo_clock.h>
#include "os/agent3/AgentMemory.h"
#include "octo_agent3.h"
#include "os/agent3/AgentClaim.h"
#include "os/agent3/Agent.h"
#include "hw/hal/Tile.h"

void os::agent::MetricAgentInvade::init()
{
    timestamp = clock(); // clock() gives back macroseconds since system startup
}

os::agent::MetricAgentInvade::MetricAgentInvade()
{
    init();
}

os::agent::MetricAgentInvade::MetricAgentInvade(AgentClaim *agentClaim)
{
    init();
    agentId = (int)(agentClaim->getOwningAgent()); //->getId();
    claimId = agentClaim->getUcid();
    // JK: To reduce the footprint, we could also skip empty maps here
    //     For now, transfer the empty ones also
    // TODO: We are not sure if this is completely functionally correct
    for (uint32_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; i++)
    {
        resourceMaps[i] = agentClaim->getResourceMap(i, ResType::RISC);
        resourceMaps[i] |= agentClaim->getResourceMap(i, ResType::iCore);
        resourceMaps[i] |= agentClaim->getResourceMap(i, ResType::TCPA); // 2. Parameter: Processor-Type RISC
    }
    isInitialClaim = 0;
}

char *os::agent::MetricAgentInvade::package()
{
    char *buffer = (char *)os::agent::AgentMemory::agent_mem_allocate(getPackageSize());
    if (buffer == nullptr)
    {
        panic("agent_mem_allocate failed");
    }

    int payloadSize = hw::hal::Tile::MAX_TILE_COUNT * sizeof(uintptr_t);
    int payloadStartIndex = packageHelper(buffer, metricId, timestamp, payloadSize);

    flatten(agentId, buffer, payloadStartIndex);
    flatten(claimId, buffer, payloadStartIndex + sizeof(int));
    for (uint32_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; i++)
    {
        flatten(resourceMaps[i], buffer, payloadStartIndex + 2 * sizeof(int) + i * sizeof(uintptr_t));
    }
    flatten(isInitialClaim, buffer, payloadStartIndex + 2 * sizeof(int) + hw::hal::Tile::MAX_TILE_COUNT * sizeof(uintptr_t) + sizeof(int));

    return buffer;
}
