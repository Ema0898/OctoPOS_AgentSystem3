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

os::agent::MetricDeletedAgent::MetricDeletedAgent(uint8_t agentId)
{
  init();
  this->agentId = agentId;
}

char *os::agent::MetricDeletedAgent::package()
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
