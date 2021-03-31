#include "AgentMetricsPrinter.h"
#include "MetricsEnum.h"
#include "hw/dev/TSCDeadlineTimer.h"

#include <stdio.h>

using namespace os::agent2;

int AgentMetricsPrinter::enabled;

void AgentMetricsPrinter::enable_metrics()
{
  enabled = 1;
}

void AgentMetricsPrinter::basic_print(SerializationBuffer &metric_buffer, uint32_t &buffer_size, uint8_t &options, const uint64_t &time, const int *cluster_arr)
{

  if (!enabled)
    return;

  METRICS buffer_value;
  int counter = 0;

  // printf("Cluster is %d on tile %d\n", clusters, hw::hal::Tile::getTileID());

  printf("------------------------------------------\n");
  printf("    METRIC      AGENT ID      CLAIM ID \n");
  printf("------------------------------------------\n");

  while (counter < buffer_size)
  {
    buffer_value = static_cast<METRICS>(metric_buffer.data[counter]);

    /* Classifies the data based in the metric id */
    if (buffer_value == METRICS::NEW)
    {
      if (options & static_cast<int>(OPTIONS::NEW))
        printf("NEW \t\t %d   \t      \n", metric_buffer.data[counter + 1]);
      //printf("---<metric>--- An Agent with id %d was created\n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == METRICS::DELETE)
    {
      if (options & static_cast<int>(OPTIONS::DELETE))
        printf("DELETE \t\t %d   \t      \n", metric_buffer.data[counter + 1]);
      //printf("---<metric>--- An Agent with id %d was deleted\n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == METRICS::INVADE)
    {
      if (options & static_cast<int>(OPTIONS::INVADE))
        printf("INVADE \t\t %d   \t\t %d \n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      //printf("---<metric>--- An Agent with id %d made an invasion in claim %d\n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
    else if (buffer_value == METRICS::RETREAT)
    {
      if (options & static_cast<int>(OPTIONS::RETREAT))
        printf("RETREAT     \t %d   \t\t %d \n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      //printf("---<metric>--- An Agent with id %d made a retreat in claim %d\n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
  }

  printf("---------------Clusters-------------------\n");
  
  for (int i = 0; i < hw::hal::Tile::getTileCount(); ++i)
  {
    // printf("Agent %d is %s\n ", i, cluster_arr[i] == 1 ? "Active" : "Not Active");
    printf("Agent %d is marked as %s \n", i, cluster_arr[i] == 1 ? "Active" : "Not Active");
  }

  printf("-----------------Time---------------------\n");
  printf("Execution Time %ld cycles\n", time);
  printf("------------------------------------------\n");
}