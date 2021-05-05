#include <stdio.h>

#include "sha.h"

#include "octopos.h"
#include "octo_agent2.h"

#define CORE_RISC 0
#define MIN_RES 2
#define MAX_RES 10

void doSignal(void *arg);
void sha_main(void *parm);



void main_ilet(claim_t claim)
{
  metrics_timer_init();	
	enable_metrics();

  constraints_t my_constr = agent_constr_create();
  agent_constr_set_quantity(my_constr, MIN_RES, MAX_RES, CORE_RISC);

  agentclaim_t my_claim = agent_claim_invade(NULL, my_constr);

  simple_ilet my_ilet;
  simple_signal signal;

  simple_ilet_init(&my_ilet, sha_main, &signal);
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

  print_metrics(my_claim, METRICS_NEW | METRICS_DELETE | METRICS_RETREAT | METRICS_INVADE);

  shutdown(0);
}

void sha_main(void *parm)
{
  int main_result = 0;
  const INT32 outData[5] =
  { 0x006a5a37UL, 0x93dc9485UL, 0x2c412112UL, 0x63f7ba43UL, 0xad73f922UL };

  sha_stream();

  for (int i = 0; i < 5; i++)
	{
	  main_result += (sha_info_digest[i] != outData[i]);
	}

  printf("\nMAIN RESULT = %d\n", main_result);

  simple_ilet reply;
	simple_ilet_init(&reply, doSignal, parm);
	dispatch_claim_send_reply(&reply);
}

void doSignal(void *arg)
{
  simple_signal *signal = (simple_signal *) arg;
	simple_signal_signal(signal);
}