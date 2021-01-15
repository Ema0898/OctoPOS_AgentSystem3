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

os::agent::MetricNewAgent::MetricNewAgent(uint8_t agentId)
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

  uint8_t offset1 = flatten(metricId, buffer, 0);
  uint8_t offset2 = flatten(agentId, buffer, offset1 + 1);
  buffer[offset1 + offset2 + 1] = '\0';

  return buffer;
}
