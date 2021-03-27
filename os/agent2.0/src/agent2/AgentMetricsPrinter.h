#ifndef AGENT_METRICS_PRINTER
#define AGENT_METRICS_PRINTER

#include "AgentSerialization.h"

#include <stdint.h>

namespace os
{
  namespace agent2
  {
    class AgentMetricsPrinter
    {
    private:
      static int enabled;

    public:
      static void basic_print(SerializationBuffer &metric_buffer, uint32_t &buffer_size, uint8_t &options, const int &clusters, const uint64_t &time);
      static void enable_metrics();
    };
  } // namespace agent2
} // namespace os

#endif