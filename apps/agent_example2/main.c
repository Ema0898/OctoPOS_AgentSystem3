#include <stdio.h>
#include <stdlib.h>

#include "octopos.h"
#include "octo_agent3.h"

#define PE_TYPE 0 // "RISC type PE"
#define DOWNEY_A 20000
#define DOWNEY_SIGMA 20
#define MIN_PE 1         // minimum required PE
#define MAX_PE 8         // maximum required PE
#define TILE_BIT_MASK 15 // 000...1111 - Invade on 4 tiles

void HelloWorldILet(void *parm)
{
  printf("Hello World from iLet on tile %u running on cpu %u with parameter %p agent example 2\n", get_tile_id(), get_cpu_id(), parm);
}

void main_ilet(claim_t claim)
{
  //agentclaim_t ag_claim = agent_claim_get_initial(claim);

  //constraints_t constrain = agent_claim_get_constr(ag_claim);
  constraints_t constrain = agent_constr_create();

  agent_constr_set_downey_speedup_parameter(constrain, DOWNEY_A, DOWNEY_SIGMA);

  agent_constr_set_quantity(constrain, MIN_PE, MAX_PE, PE_TYPE);

  agent_constr_set_tile_bitmap(constrain, TILE_BIT_MASK);

  //agent_constr_set_tile_shareable(constrain, 1);

  //agent_constr_set_notontile(constrain, 1);

  //agent_constr_set_tile_shareable(constrain, 1);

  //agent_constr_set_appnumber(constrain, 0);

  //agent_claim_reinvade_constraints(ag_claim, constrain);

  agentclaim_t ag_claim = agent_claim_invade(NULL, constrain);

  // constraints_t constr2 = agent_claim_get_constr(ag_claim);
  // agent_constr_set_tile_bitmap(constr2, TILE_BIT_MASK);
  // agent_claim_reinvade_constraints(ag_claim, constr2);

  if (!ag_claim)
  {
    printf("Error in invasion\n");
    shutdown(0);
  }

  printf("TILE COUNT = %d agent example 2\n", agent_claim_get_tilecount(ag_claim));

  for (int tile = 0; tile < get_tile_count(); tile++)
  {
    // Get available PEs
    int pes = agent_claim_get_pecount_tile_type(ag_claim, tile, PE_TYPE);
    printf("Tile %d, PE's %d agent example 2\n", tile, pes);

    // If there are available PEs in this tile's claim.
    if (pes != 0)
    {
      // Get a proxy claim in tile for a "RISC" type PE
      proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(ag_claim, tile, PE_TYPE);
      if (pClaim == NULL)
      {
        printf("Couldn't get proxy claim \n");
      }
      // Create iLet team
      simple_ilet ILet[pes];
      // Initialize the iLet team with work (HelloWorld) function
      for (int iletnr = 0; iletnr < pes; ++iletnr)
      {
        simple_ilet_init(&ILet[iletnr], HelloWorldILet, NULL);
      }
      // Infect claim with Ilet team.
      printf("Infecting %d Ilets on Tile %d agent example 2\n", pes, tile);
      proxy_infect(pClaim, ILet, pes);
    }
  }
  while (1)
    ;

  //shutdown(0);
  return;
}