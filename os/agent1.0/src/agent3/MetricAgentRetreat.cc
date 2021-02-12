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

  uint8_t offset1 = flatten(metricId, buffer, 0);
  uint8_t offset2 = flatten(claimId, buffer, offset1 + 1);
  uint8_t offset3 = flatten(agentId, buffer, offset1 + offset2 + 2);
  buffer[offset1 + offset2 + offset3 + 2] = '\0';

  return buffer;
}
