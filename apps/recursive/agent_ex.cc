#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "octopos.h"
#include "octo_agent3.h"

void signaler(void *sig)
{
    simple_signal *s = reinterpret_cast<simple_signal *>(sig);
    printf("Signalling Signal %p\n", s);
    simple_signal_signal_and_exit(s);
}

void RecursiveILet(agent_t myAgent, void *signal)
{

    printf("Launching RecursiveILet on %d,%d with parameters Agent %p, sync %p\n", get_tile_id(), get_cpu_id(), myAgent, signal);

    constraints_t myConstraints;
    agentclaim_t myClaim;

    myConstraints = agent_constr_create();
    agent_constr_set_quantity(myConstraints, 1, 2, 0);
    agent_constr_set_tile_shareable(myConstraints, 1);
    myClaim = agent_claim_invade(myAgent, myConstraints); // creates an agentclaim_t object!

    if (!myClaim)
    {
        fprintf(stderr, "Invade operation unsuccessful.");
        abort();
    }

    printf("* iLet got Claim of Size %d\r\n", agent_claim_get_pecount(myClaim));

    if (agent_claim_get_pecount(myClaim) > 0)
    {
        // Spawn work on my new claim
        simple_signal sync;
        simple_signal_init(&sync, agent_claim_get_pecount(myClaim));

        for (int tile = 0; tile < get_tile_count(); tile++)
        {
            if (int pes = agent_claim_get_pecount_tile_type(myClaim, tile, 0))
            { // Type = 0 ^= RISC
                proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(myClaim, tile, 0);
                printf("* Got Proxy Claim %p\r\n", pClaim);

                simple_ilet ILet[pes];
                for (int i = 0; i < pes; ++i)
                {
                    //dual_ilet_init(&ILet[i], RecursiveILet, (void *)myAgent, &sync);
                    dual_ilet_init(&ILet[i], RecursiveILet, myAgent, &sync);
                }

                printf("Infecting %d Ilets on Tile %d\n", pes, tile);
                proxy_infect(pClaim, &ILet[0], pes);
            }
        }

        printf("### I am doing some work\r\n");

        printf("Waiting on Signal %p...\r\n", &sync);
        simple_signal_wait(&sync);
        printf("All children finished!\r\n");
    }
    else
    {
        printf("### I have to do all the work myself :(\r\n");
    }

    agent_claim_retreat(myClaim); // deletes the claim object!
    agent_constr_delete(myConstraints);

    if (signal)
    {
        simple_ilet answer;
        simple_ilet_init(&answer, signaler, signal);
        printf("Sending signal...\r\n");
        dispatch_claim_send_reply(&answer);
    }
    else
    {
        printf("\r\nAll Done!\r\n");
        shutdown(0);
    }
}

extern "C" void main_ilet(claim_t claim)
{
    printf("main ilet\n");

    agentclaim_t initialClaim = agent_claim_get_initial(claim);

    for (int tile = 0; tile < get_tile_count(); tile++)
    {
        if (int pes = agent_claim_get_pecount_tile_type(initialClaim, tile, 0))
        { // Type = 0 ^= RISC
            proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(initialClaim, tile, 0);
            printf("* Got Proxy Claim %p\n", pClaim);

            simple_ilet ILet[pes];
            for (int iletnr = 0; iletnr < pes; ++iletnr)
            {
                //dual_ilet_init(&ILet[iletnr], RecursiveILet, (void *)agent_claim_get_agent(initialClaim), NULL);
                dual_ilet_init(&ILet[iletnr], RecursiveILet, agent_claim_get_agent(initialClaim), NULL);
            }

            printf("Infecting %d Ilets on Tile %d\n", pes, tile);
            proxy_infect(pClaim, &ILet[0], pes);
        }
    }

    return; // allows the infected iLet to start execution
}

#if false

FAKE-Recursive ;-)

extern "C" void main_ilet(claim_t claim) {
    printf("main ilet\n");

    agentclaim_t initialClaim = agent_claim_get_initial(claim);
    agent_claim_print(initialClaim);

    constraints_t testconstr[10];
    agentclaim_t testclaim[10];
    for (int i=0; i<10; i++) {
        printf("Invading %i, all children of initial\r\n", i);
        testconstr[i]=agent_constr_create();
        agent_constr_set_quantity(testconstr[i], 1, 1, 0);
        agent_constr_set_tile_shareable(testconstr[i], 1);
        testclaim[i]=agent_claim_invade(initialClaim, testconstr[i]);

        if (!testclaim[i]) {
            fprintf(stderr, "Invade operation unsuccessful.");
            abort();
        }

        agent_claim_print(testclaim[i]);
    }

    for (int i=0; i<10; i++) {
        printf("Retreating %i, all children of initial\r\n", i);
        agent_claim_retreat(testclaim[i]);
        agent_constr_delete(testconstr[i]);
    }



    printf("Invading %i, all children of each other\r\n", 0);
    testconstr[0]=agent_constr_create();
    agent_constr_set_quantity(testconstr[0], 1, 1, 0);
    agent_constr_set_tile_shareable(testconstr[0], 1);
    testclaim[0]=agent_claim_invade(initialClaim, testconstr[0]);

    if (!testclaim[0]) {
        fprintf(stderr, "Invade operation unsuccessful.");
        abort();
    }

    agent_claim_print(testclaim[0]);
    for (int i=1; i<10; i++) {
        printf("Invading %i, all children of each other\r\n", i);
        testconstr[i]=agent_constr_create();
        agent_constr_set_quantity(testconstr[i], 1, 1, 0);
        agent_constr_set_tile_shareable(testconstr[i], 1);
        testclaim[i]=agent_claim_invade(testclaim[i-1], testconstr[i]);

        if (!testclaim[i]) {
            fprintf(stderr, "Invade operation unsuccessful.");
            abort();
        }

        agent_claim_print(testclaim[i]);
    }
    for (int i=0; i<10; i++) {
        printf("Retreating %i, all children of each other\r\n", i);
        agent_claim_retreat(testclaim[i]);
        agent_constr_delete(testconstr[i]);
    }


    printf("All Done!");


    shutdown(0);
}

#endif
