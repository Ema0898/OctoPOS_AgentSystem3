#include <stdio.h>
#include <stdlib.h>

#include <octopos.h>
#include <octo_agent3.h>

#define MIN_PE 1         // minimum required PE
#define MAX_PE 64        // maximum required PE
#define TILE_BIT_MASK 15 // 000...1111 - Invade on 4 tiles

static void say_hello(void *parm)
{
  printf("Hello World form tile %d and core %d\n", get_tile_id(), get_cpu_id());
}

void main_ilet(claim_t claim)
{

  //agentclaim_t initClaim = agent_claim_get_initial(claim);

  agentclaim_t myClaim;
  constraints_t myConstr;

  myClaim = agent_claim_invade(NULL, myConstr);

  //myConstr = agent_constr_create();
  myConstr = agent_claim_get_constr(myClaim);
  agent_constr_set_quantity(myConstr, MIN_PE, MAX_PE, 0);
  // Allows invasion on 4 tiles (bit map: 000...1111)
  agent_constr_set_tile_bitmap(myConstr, TILE_BIT_MASK);
  // myClaim = agent_claim_get_initial(claim);
  // agent_claim_reinvade_constraints(myClaim, myConstr);

  if (!myClaim)
  {
    fprintf(stderr, "Invade operation unsuccessful.");
    abort();
  }

  for (int tile = 1; tile < get_tile_count(); tile++)
  {
    // Get available PEs
    int pes = agent_claim_get_pecount_tile_type(myClaim, tile, 0);
    printf("Tile %d, PE's %d \n", tile, pes);

    // If there are available PEs in this tile's claim.
    if (pes != 0)
    {
      // Get a proxy claim in tile for a "RISC" type PE
      proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(myClaim, tile, 0);
      if (pClaim == NULL)
      {
        printf("Couldn't get proxy claim \n");
      }
      // Create iLet team
      simple_ilet ILet[pes];
      // Initialize the iLet team with work (HelloWorld) function
      for (int iletnr = 0; iletnr < pes; ++iletnr)
      {
        simple_ilet_init(&ILet[iletnr], say_hello, NULL);
      }
      // Infect claim with Ilet team.
      printf("Infecting %d Ilets on Tile %d\n", pes, tile);
      proxy_infect(pClaim, ILet, pes);
    }
  }
  while (1)
    ;
  shutdown(0);
  return;
}