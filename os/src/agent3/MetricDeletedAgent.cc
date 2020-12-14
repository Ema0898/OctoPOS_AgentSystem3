#include "os/agent3/MetricDeletedAgent.h"
#include "lib/printk.h"

#include <octo_clock.h>
#include "os/agent3/AgentMemory.h"

void os::agent::MetricDeletedAgent::init()
{
    timestamp = clock();
}

os::agent::MetricDeletedAgent::MetricDeletedAgent()
{
    init();
}

os::agent::MetricDeletedAgent::MetricDeletedAgent(int agentId)
{
    init();
    agentId = agentId;
}

char *os::agent::MetricDeletedAgent::package()
{
    char *buffer = (char *)os::agent::AgentMemory::agent_mem_allocate(getPackageSize());
    if (buffer == nullptr)
    {
        panic("agent_mem_allocate failed");
    }

    int payloadSize = 1 * sizeof(int32_t);
    int payloadStartIndex = packageHelper(buffer, metricId, timestamp, payloadSize);

    flatten(agentId, buffer, payloadStartIndex);

    return buffer;
}
