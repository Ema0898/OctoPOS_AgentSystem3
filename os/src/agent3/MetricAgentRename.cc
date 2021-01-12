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

os::agent::MetricAgentRename::MetricAgentRename(uint8_t id, const char *name)
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

  uint8_t offset1 = flatten(metricId, buffer, 0);
  uint8_t offset2 = flatten(agentId, buffer, offset1 + 1);
  uint8_t length = strlen(newName);

  memcpy(buffer + offset1 + offset2 + 2, newName, length);

  return buffer;
}
