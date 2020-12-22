#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "octopos.h"
#include "octo_agent3.h"

#define MALLEABLE 1 // 1 means activated malleability.

const int parnum = 2;

// A simple resize handler which does nothing but print a line when it is called. Needed when enabling malleability.
static void myResizeHandler(const agentclaim_t claim, const size_t tile_count, const size_t res_per_tile, const gain_t mygain, const loss_t myloss, const resize_env_t myenvt);

// for information check comment above
void myResizeHandler(const agentclaim_t claim, const size_t tile_count, const size_t res_per_tile, const gain_t mygain, const loss_t myloss, const resize_env_t myenvt)
{
	printf("Malleability: Resize handler called.\n");
}

extern "C" void main_ilet(claim_t claim)
{
	printf("main ilet\n");

	int tilecount = get_compute_tile_count(); // Gets the number of compute tiles in the system. This number does not include I/O or TCPA tiles.
	unsigned int coreSum = 0;
	for (int tile = 0; tile < tilecount; ++tile)
	{
		coreSum += get_tile_core_count();
	}
	coreSum -= 1; // -1 because it is reserved for the initial claim
	printf("Total compute cores: %u.\nMALLEABLE is: %d.\n", coreSum, MALLEABLE);

	agentclaim_t initialClaim = agent_claim_get_initial(claim);

	printf("** Invading %d Claims for Stresstest:\n", parnum);

	constraints_t myConstr[parnum];
	agentclaim_t myClaim[parnum];

	printf("CORESUM = %d\n", coreSum);

	for (int i = 0; i < parnum; i++)
	{
		myConstr[i] = agent_constr_create();
		agent_constr_set_quantity(myConstr[i], 1, coreSum, 0); // min 2, max coreSum, type 0 = RISC. Adjust this to your needs, whether you want to need to use malleability. With max = coreSum, you need to define MALLEABLE 1.
		agent_constr_set_tile_shareable(myConstr[i], 1);
		agent_constr_set_appnumber(myConstr[i], i);
		if (MALLEABLE)
		{
			int myResizeEnv; // becomes &myResizeEnv: just a user defined pointer
			agent_constr_set_malleable(myConstr[i], true, myResizeHandler, &myResizeEnv);
		}
		myClaim[i] = agent_claim_invade(NULL, myConstr[i]);

		if (!myClaim[i])
		{
			fprintf(stderr, "Invade operation unsuccessful.");
			abort();
		}

		printf("* Returned Claim %d of Size %d\n", i, agent_claim_get_pecount(myClaim[i]));
	}

	for (int run = 0; run < 5; run++)
	{
		printf("Run %d:\n", run);
		for (int i = 0; i < parnum; i++)
		{
			int ret = agent_claim_reinvade(myClaim[i]);

			if (-1 == ret)
			{
				fprintf(stderr, "Reinvade operation unsuccessful.");
				abort();
			}

			printf("* Reinvaded Claim %d to Size %d\n", i, agent_claim_get_pecount(myClaim[i]));
		}
		printf("\n");
	}

	printf("Shutting down.\n");
	shutdown(0);

	return;
}
