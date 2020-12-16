// The main OctoPOS header.
#include <octopos.h>

// C-library headers. Note that OctoPOS supports only a subset of the C runtime
// library.
#include <stdio.h>
#include <stdlib.h>

//based on drr-demo

static void dummyILetFunc(void *arg);

//entry point - Main iLet
extern "C" void main_ilet(claim_t claim)
{
	printf("[C-bomb]: Running on tile %d \n", get_tile_id());
	//get number of available tiles
	uint8_t TILES = get_tile_count();

	// Future invades and claims arrays
	invade_future *futures = static_cast<invade_future *>(malloc((TILES - 1) * sizeof(invade_future)));
	proxy_claim_t *pcs = static_cast<proxy_claim_t *>(malloc((TILES - 1) * sizeof(proxy_claim_t)));

	// Create iLet for dummy function
	simple_ilet iLet;
	simple_ilet_init(&iLet, dummyILetFunc, NULL);

	// Now get all resources
	for (uint8_t i = 1; i < TILES; i++)
	{
		//Invade call on tile i
		invade_future future = futures[i - 1];
		if (proxy_invade(i, &future, get_tile_core_count()) != 0)
		{
			printf("[C-bomb]: proxy_invade failed - does tile %i even exist?\n", i);
			abort();
		}
		//Checking the claim
		pcs[i - 1] = invade_future_force(&future);
		// If we have this tile, then run dummy function
		if (pcs[i - 1] != 0)
		{
			//Run a dummy function on tile i, just to be "parallel"
			proxy_infect(pcs[i - 1], &iLet, 1);
		}
	}
	//Wait forever
	simple_signal signal;
	simple_signal_wait(&signal);
	shutdown(0);
}

// This is the function we execute on tile X.
static void dummyILetFunc(void *arg)
{
	printf("[C-bomb]: Got tile %u!\n", get_tile_id());
	// Just wait forever
	while (1)
		;
}
