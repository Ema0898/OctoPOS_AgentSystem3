#include "os/agent3/MetricAgentRename.h"

#include <octo_clock.h>
#include "os/agent3/AgentConstraint.h"

void os::agent::MetricAgentRename::init()
{
    timestamp = clock();
}

os::agent::MetricAgentRename::MetricAgentRename()
{
    init();
}

os::agent::MetricAgentRename::MetricAgentRename(int id, const char *name)
{
    init();
    agentId = id;
    strncpy(newName, name, os::agent::AgentInstance::maxNameLength);
    if (strlen(newName) >= os::agent::AgentInstance::maxNameLength)
        newName[os::agent::AgentInstance::maxNameLength - 1] = '\0';
}

char *os::agent::MetricAgentRename::package()
{

    char *buffer = (char *)os::agent::AgentMemory::agent_mem_allocate(getPackageSize());
    if (buffer == nullptr)
    {
        panic("agent_mem_allocate failed");
    }

    int payloadSize = 1 * sizeof(int32_t) + strlen(newName) + 1;
    int payloadStartIndex = packageHelper(buffer, metricId, timestamp, payloadSize);
    flatten(agentId, buffer, payloadStartIndex);
    int i = 0;
    for (i = 0; newName[i] != '\0'; i++)
    {
        flatten(newName[i], buffer, payloadStartIndex + sizeof(int) + i);
    }
    flatten('\0', buffer, payloadStartIndex + sizeof(int) + i);
    return buffer;
}
