#include "os/agent3/MetricAgentRetreat.h"
#include "lib/printk.h"

#include <octo_clock.h>
#include "os/agent3/AgentConstraint.h"

void os::agent::MetricAgentRetreat::init()
{
    timestamp = clock(); // clock() gives back macroseconds since system startup
}

os::agent::MetricAgentRetreat::MetricAgentRetreat()
{
    init();
}

os::agent::MetricAgentRetreat::MetricAgentRetreat(int claimId, int agentId)
{
    init();
    this->claimId = claimId;
    this->agentId = agentId;
}

char *os::agent::MetricAgentRetreat::package()
{
    char *buffer = (char *)os::agent::AgentMemory::agent_mem_allocate(getPackageSize());
    if (buffer == nullptr)
    {
        panic("agent_mem_allocate failed");
    }

    int payloadSize = 2 * sizeof(uintptr_t);
    int payloadStartIndex = packageHelper(buffer, metricId, timestamp, payloadSize);

    flatten(claimId, buffer, payloadStartIndex);
    flatten(agentId, buffer, payloadStartIndex + sizeof(int));

    return buffer;
}
