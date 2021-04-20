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
    public:
      static void print_general_metrics(SerializationBuffer &metric_buffer, uint32_t &buffer_size, uint8_t &options);
      static void print_cluster_metrics(const int *clusters);
      static void print_timer_metrics(const uint64_t &time);
      static void print_claim_metrics(const int *claim, const int &used_cores);
    };
  } // namespace agent2
} // namespace os

#endif