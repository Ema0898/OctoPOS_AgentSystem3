#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    printf("Hello World from iLet on tile %u running on cpu %u with parameter %p agent example 1\n", get_tile_id(), get_cpu_id(), parm);
}

void main_ilet(claim_t claim)
{
    printf("Hello World from main ilet, have %d tiles\n", get_tile_count());
    // Wraps the initial claim with an agent
    agentclaim_t initialClaim = agent_claim_get_initial(claim);

    // Prints original the claim
    agent_claim_print(initialClaim);

    // Gets initial constraints
    constraints_t newConstr = agent_claim_get_constr(initialClaim);
    // Sets the Downey parameters (A and sigma)
    agent_constr_set_downey_speedup_parameter(newConstr, DOWNEY_A, DOWNEY_SIGMA);
    // Modifies the number of PE in constraints
    agent_constr_set_quantity(newConstr, MIN_PE, MAX_PE, PE_TYPE);
    // Allows invasion on 4 tiles (bit map: 000...1111)
    agent_constr_set_tile_bitmap(newConstr, TILE_BIT_MASK);

    //agent_constr_set_tile_shareable(newConstr, 1);
    //agent_constr_set_stickyclaim(newConstr, 0);

    //agent_constr_set_notontile(newConstr, 1);
    // Reinvades claim with updated constraints
    //agent_constr_set_appnumber(newConstr, 1);

    agent_claim_reinvade_constraints(initialClaim, newConstr);
    if (initialClaim == NULL)
    {
        printf("Reinvade failed... shutting off \n");
        shutdown(0);
    }

    printf("TILE COUNT = %d agent example 1\n", agent_claim_get_tilecount(initialClaim));

    for (int tile = 0; tile < get_tile_count(); tile++)
    {
        // Get available PEs
        int pes = agent_claim_get_pecount_tile_type(initialClaim, tile, PE_TYPE);
        printf("Tile %d, PE's %d agent example 1\n", tile, pes);

        // If there are available PEs in this tile's claim.
        if (pes != 0)
        {
            // Get a proxy claim in tile for a "RISC" type PE
            proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(initialClaim, tile, PE_TYPE);
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
            printf("Infecting %d Ilets on Tile %d agent example 1\n", pes, tile);
            proxy_infect(pClaim, ILet, pes);
        }
    }
    while (1)
        ;
    shutdown(0);
    return;
}
