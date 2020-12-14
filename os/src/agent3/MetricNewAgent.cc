#include "os/agent3/MetricNewAgent.h"
#include "lib/printk.h"

#include <octo_clock.h>
#include "os/agent3/AgentMemory.h"

void os::agent::MetricNewAgent::init()
{
    timestamp = clock(); // clock() gives back macroseconds since system startup
}

os::agent::MetricNewAgent::MetricNewAgent()
{
    init();
}

os::agent::MetricNewAgent::MetricNewAgent(int agentId)
{
    init();
    this->agentId = agentId;
}

char *os::agent::MetricNewAgent::package()
{
    char *buffer = (char *)os::agent::AgentMemory::agent_mem_allocate(getPackageSize());
    if (buffer == nullptr)
    {
        panic("agent_mem_allocate failed");
    }

    int payloadSize = 1 * sizeof(int32_t);
    int payloadStartIndex = packageHelper(buffer, metricId, timestamp, payloadSize);

    // Actual Payload unique to the "New Agent Metric" - here, it is the AgentID
    //buffer[payloadStartIndex] = agentId;
    flatten(agentId, buffer, payloadStartIndex);

    return buffer;
}
