#include <stdio.h>

#include "aes.h"

#include "octopos.h"
#include "octo_agent2.h"

#define CORE_RISC 0
#define MIN_RES 5
#define MAX_RES 10

/* Amount of agents to be created */
const int NUM_PROCESS = 4;

void doSignal(void *arg);
void aes_main(void *parm);

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
    simple_ilet_init(&my_ilet[i], aes_main, &signal[i]);

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

  /* Prints the main result */
  printf("\nMAIN RESULT = %d\n", main_result);

  /* Prints and calculates the execution time */
  double time = ((double) (stop - start)) / 1000000000;
  printf("TIME = %f\n", time);

  /* Calls the system metrics */
  print_metrics(my_claim[0], METRICS_NEW | METRICS_DELETE | METRICS_RETREAT | METRICS_INVADE);

  while(1);
}

/* Function to be executed in each agent */
void aes_main(void *parm)
{
/*
+--------------------------------------------------------------------------+
| * Test Vectors (added for CHStone)                                       |
|     statemt, key : input data                                            |
+--------------------------------------------------------------------------+
*/
  statemt[0] = 50;
  statemt[1] = 67;
  statemt[2] = 246;
  statemt[3] = 168;
  statemt[4] = 136;
  statemt[5] = 90;
  statemt[6] = 48;
  statemt[7] = 141;
  statemt[8] = 49;
  statemt[9] = 49;
  statemt[10] = 152;
  statemt[11] = 162;
  statemt[12] = 224;
  statemt[13] = 55;
  statemt[14] = 7;
  statemt[15] = 52;

  key[0] = 43;
  key[1] = 126;
  key[2] = 21;
  key[3] = 22;
  key[4] = 40;
  key[5] = 174;
  key[6] = 210;
  key[7] = 166;
  key[8] = 171;
  key[9] = 247;
  key[10] = 21;
  key[11] = 136;
  key[12] = 9;
  key[13] = 207;
  key[14] = 79;
  key[15] = 60;

  for (int i = 0; i < 250; ++i)
  {
    encrypt (statemt, key, 128128);
    decrypt (statemt, key, 128128);
  }

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
