#include <stdio.h>

#include "sha.h"

#include "octopos.h"
#include "octo_agent2.h"

#define CORE_RISC 0
#define MIN_RES 5
#define MAX_RES 10

/* Amount of agents to be created */
const int NUM_PROCESS = 4;

void doSignal(void *arg);
void sha_main(void *parm);

void main_ilet(claim_t claim)
{
  agentclaim_t initial = agent_claim_get_initial(claim);

  /* Enables the metrics */
  metrics_timer_init();
	enable_metrics();

  /* Creates the needed structures */
  simple_ilet my_ilet[NUM_PROCESS];
  proxy_claim_t pClaim[NUM_PROCESS];
  simple_signal signal[NUM_PROCESS];
  agentclaim_t my_claim[NUM_PROCESS];
  agent_t agent[NUM_PROCESS];

  /* Creates and initialize the agent constraints */
  constraints_t my_constr;

  my_constr = agent_constr_create();
  agent_constr_set_quantity(my_constr, MIN_RES, MAX_RES, CORE_RISC);

  /* Initializes the structures */
  for (int i = 0; i < NUM_PROCESS; ++i)
  {
    simple_signal_init(&signal[i], 1);
    simple_ilet_init(&my_ilet[i], sha_main, &signal[i]);

    agent[i] = agent_agent_create();

    my_claim[i] = agent_claim_invade(agent[i], my_constr);
  }

  /* Takes the start time */
  uint64_t start = metrics_timer_start();

  /* Looks for the tile were the corresponding agents has its resources. */
  for (int i = 0; i < NUM_PROCESS; ++i)
  {
    for (int tile = 0; tile < get_tile_count(); tile++)
    {
      int pes = agent_claim_get_pecount_tile_type(my_claim[i], tile, CORE_RISC);
      pClaim[i] = agent_claim_get_proxyclaim_tile_type(my_claim[i], tile, CORE_RISC);

      if (pClaim[i] != 0)
      {
        /* Starts the infection */
        proxy_infect(pClaim[i], &my_ilet[i], 1);
        break;
      }
    }
  }

  /* Waits for the agents to complete its task */
  for (int i = 0; i < NUM_PROCESS; ++i)
  {
    simple_signal_wait(&signal[i]);
    print_claim_resources(my_claim[i]);
  }

  /* Takes the stop time */
  uint64_t stop = metrics_timer_stop();

  /* Prints and calculates the execution time */
  double time = ((double) (stop - start)) / 1000000000;
  printf("TIME = %f\n", time);

  /* Calls the system metrics */
  print_metrics(my_claim[0], METRICS_NEW | METRICS_DELETE | METRICS_RETREAT | METRICS_INVADE);

  shutdown(0);
}

/* Function to be executed in each agent */
void sha_main(void *parm)
{
  int main_result = 0;
  const INT32 outData[5] =
  { 0x006a5a37UL, 0x93dc9485UL, 0x2c412112UL, 0x63f7ba43UL, 0xad73f922UL };

  for (int i = 0; i < 250; ++i)
  {
    sha_stream();
  }

  for (int i = 0; i < 5; i++)
	{
	  main_result += (sha_info_digest[i] != outData[i]);
	}

  /* Prints the main result */
  printf("\nMAIN RESULT = %d\n", main_result);

  simple_ilet reply;
	simple_ilet_init(&reply, doSignal, parm);
	dispatch_claim_send_reply(&reply);
}

/* Down the each signal */
void doSignal(void *arg)
{
  simple_signal *signal = (simple_signal *) arg;
	simple_signal_signal(signal);
}