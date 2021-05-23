#include "AgentMetricsPrinter.h"
#include "MetricsEnum.h"
#include "os/dev/HWInfo.h"

#include <stdio.h>

using namespace os::agent2;

/* Prints the data which is inside the buffer. */
void AgentMetricsPrinter::print_general_metrics(SerializationBuffer &metric_buffer, uint32_t &buffer_size, uint8_t &options)
{
  METRICS buffer_value;
  int counter = 0;

  printf("---------------------General Metrics---------------------\n");
  printf("    METRIC      AGENT ID      CLAIM ID    \n");
  printf("---------------------------------------------------------\n");

  while (counter < buffer_size)
  {
    buffer_value = static_cast<METRICS>(metric_buffer.data[counter]);

    /* Classifies the data based in the metric id */
    if (buffer_value == METRICS::NEW)
    {
      if (options & static_cast<int>(OPTIONS::NEW))
        printf("NEW \t\t %d   \t      \n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == METRICS::DELETE)
    {
      if (options & static_cast<int>(OPTIONS::DELETE))
        printf("DELETE \t\t %d   \t      \n", metric_buffer.data[counter + 1]);
      counter += 2;
    }
    else if (buffer_value == METRICS::INVADE)
    {
      if (options & static_cast<int>(OPTIONS::INVADE))
        printf("INVADE \t\t %d   \t\t %d \n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
    else if (buffer_value == METRICS::RETREAT)
    {
      if (options & static_cast<int>(OPTIONS::RETREAT))
        printf("RETREAT     \t %d   \t\t %d \n", metric_buffer.data[counter + 1], metric_buffer.data[counter + 2]);
      counter += 3;
    }
  }

  printf("---------------------------------------------------------\n");
}

/* Prints the clusters and its status */
void AgentMetricsPrinter::print_cluster_metrics(const int *clusters)
{
  printf("---------------Clusters-------------------\n");
  
  for (int i = 0; i < hw::hal::Tile::getTileCount(); ++i)
  {
    printf("Agent %d is marked as %s \n", i, clusters[i] == 1 ? "Active" : "Not Active");
  }

  printf("------------------------------------------\n");
}

/* Prints the delta time */
void AgentMetricsPrinter::print_timer_metrics(const uint64_t &time)
{
  printf("-----------------Time---------------------\n");

  if (time < 0)
  {
    printf("Error in time metric\n");
  }

  printf("Execution Time %ld cycles\n", time);

  printf("------------------------------------------\n");
}

/* Prints the resources for a specific claim. */
void AgentMetricsPrinter::print_claim_metrics(const int *claim, const int &used_cores)
{
  int totalCores = hw::hal::Tile::getTileCount() * os::dev::HWInfo::Inst().getCoreCount();

  printf("-----------------------Cores used by claim id---------------------------\n");
  printf("\t ");
  for (int i = 0; i < os::dev::HWInfo::Inst().getCoreCount(); ++i)
  {
    printf("CPU %d   ", i);
  }
  printf("\n");

  for (int i = 0; i < hw::hal::Tile::getTileCount(); ++i)
  {
    printf("Tile %d   ", i);
    for (int j = 0; j < os::dev::HWInfo::Inst().getCoreCount(); ++j)
    {
      printf("%d\t   ", claim[i * os::dev::HWInfo::Inst().getCoreCount() + j]);
    }
    printf("\n");
  }
  printf("\n");

  printf("Using %d of %d total cores\n", used_cores, totalCores);
  printf("------------------------------------------------------------------------\n");
}