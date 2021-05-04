#include <stdio.h>

#include "aes.h"

#include "octopos.h"
#include "octo_agent2.h"

#define CORE_RISC 0
#define MIN_RES 2
#define MAX_RES 5

void doSignal(void *arg);
void aes_main(void *parm);

void main_ilet(claim_t claim)
{
  metrics_timer_init();	
	enable_metrics();

  constraints_t my_constr = agent_constr_create();
  agent_constr_set_quantity(my_constr, MIN_RES, MAX_RES, CORE_RISC);

  agentclaim_t my_claim = agent_claim_invade(NULL, my_constr);

  simple_ilet my_ilet;
  simple_signal signal;

  simple_ilet_init(&my_ilet, aes_main, &signal);
  simple_signal_init(&signal, 1);

  metrics_timer_start();

  for (int tile = 0; tile < get_tile_count(); tile++)
  {
    int pes;
    pes = agent_claim_get_pecount_tile_type(my_claim, tile, CORE_RISC);
    printf("Tile: %d PEs: %d\n", tile, pes);
    if (pes > 0)
    {
      proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(my_claim, tile, CORE_RISC);
      proxy_infect(pClaim, &my_ilet, 1);

      break;
    }
  }

  simple_signal_wait(&signal);

  metrics_timer_stop();

  printf("\nMAIN RESULT = %d\n", main_result);

  print_metrics(my_claim, METRICS_NEW | METRICS_DELETE | METRICS_RETREAT | METRICS_INVADE);

  shutdown(0);
}

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

  encrypt (statemt, key, 128128);
  decrypt (statemt, key, 128128);

  simple_ilet reply;
	simple_ilet_init(&reply, doSignal, parm);
	dispatch_claim_send_reply(&reply);
}

void doSignal(void *arg)
{
  simple_signal *signal = (simple_signal *) arg;
	simple_signal_signal(signal);
}
