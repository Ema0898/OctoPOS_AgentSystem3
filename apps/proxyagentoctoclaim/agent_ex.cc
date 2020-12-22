#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "octopos.h"
#include "octo_agent3.h"

/* App to test access to iLet's AgentOctoClaim on tiles 0 and higher. Ilets corresponding to a ProxyClaim which is defined on tile != 0 need to use ProxyAgentOctoClaim to access its AgentOctoClaim functions. (Reason: Its AgentOctoClaim etc. is defined via main_ilet on tile 0 and is therefore not accessible on other tiles directly, because of different address spaces on different tiles)

You need to shutdown the app yourself [CTRL + C].
Claims total: 1 initial claim on tile 0; non-initial claims on tile [0..(claimsTotal-1)]. By default there are non-initial claims on tile 0 and 1.
*/

#define ALWAYS_USE_PROXY 1																	 // 1 means use ProxyAgentOctoClaim even on tiles we actually have direct access to our AgentOctoClaim. The scenario "ALWAYS_USE_PROXY 1" sounds reasonable as sometimes we might not have the pointer to our original AgentOctoClaim in the iLet even on same tile our AgentOctoClaim has been defined.
#define PROVOKE_CRASH_AGENT_CLAIM_GET_AGENT 0								 // when set to >0, calls agent_claim_get_agent function even from a tile other than where the objects are located which leads to exiting the application with a meaningful error message.
#define PROVOKE_CRASH_AGENT_CLAIM_GET_CONSTR 0							 // when set to >0, calls agent_claim_get_constr function even from a tile other than where the objects are located which leads to exiting the application with a meaningful error message.
#define PROVOKE_CRASH_AGENT_CLAIM_INVADE_PARENTCLAIM 0			 // when set to >0, calls agent_claim_invade_parentclaim function from a tile other than where the objects are located which leads to exiting the application with a meaningful error message. Does not call from same object's tile because we cant invade the already invaded AgentOctoClaim.
#define PROVOKE_CRASH_AGENT_CLAIM_REINVADE_CONSTRAINTS 0		 // when set to >0, calls agent_claim_reinvade_constraints function.
#define PROVOKE_CRASH_AGENT_CLAIM_GET_PROXYCLAIM_TILE_TYPE 0 // when set to >0, calls agent_claim_get_proxyclaim_tile_type function even from a tile other than where the objects are located which leads to exiting the application with a meaningful error message.
#define PROVOKE_CRASH_AGENT_CLAIM_RETREAT 0									 // when set to >0, calls agent_claim_retreat function from a tile other than where the objects are located which leads to exiting the application with a meaningful error message. Does not call from same object's tile because we cant retreat our own AgentOctoClaim in the iLet.
#define PROVOKE_CRASH_AGENT_CLAIM_REINVADE 0								 // when set to >0, calls agent_claim_reinvade function AND executes another function thereafter. The latter function leads to existing the application with a meaningful error message, if a ProxyAgentOctoClaim is used.
#define PROVOKE_CRASH_PROXY_DELETE_PROXYAGENTOCTOCLAIM 0		 // when set to >0, calls proxy_delete_proxyagentoctoclaim function even on AbstractAgentOctoClaim objects that are castable to AgentOctoClaims.

const int claimsTotal = 2;													// non initial claims on tile [0..(claimsTotal-1)]. So for claimsTotal=2, creates claims on tiles 0 and 1. If you enter a higher number here, the app tries to allocate claims on higher tile numbers. Should work for "3", too, if we have 4 cores with 1 AGENT_SYSTEM Tile.
int pes = 1;																				// 1 processing element per claim
constraints_t myConstr[claimsTotal];								// the constraints handle array
agentclaim_t myAbstractAgentOctoClaim[claimsTotal]; // the AbstractAgentOctoClaim handle array
proxy_claim_t myProxyClaim[claimsTotal];						// the ProxyClaim handle array
uint32_t myUcidArray[claimsTotal];									// array that contains the Ucid of each AgentOctoClaim
int myObjectsTile = -1;

// Ilet which gets infected on a tile corresponding to its ProxyClaim.
void HelloWorldILet(void *ucid, void *objects_tile);

void HelloWorldILet(void *ucid, void *objects_tile)
{
	int objects_tile_param = *((int *)objects_tile);
	uint32_t ucid_param = *((uint32_t *)ucid);
	printf("Hello from iLet on tile %u running on core %u called with parameters: objects_tile = %i, octo_ucid = %" PRIu32 "\n", get_tile_id(), get_cpu_id(), objects_tile_param, ucid_param);
	bool isProxy = false; // only used to not accidentally call agent_proxy_delete_proxyagentoctoclaim. You can force to call that function by setting PROVOKE_CRASH_PROXY_DELETE_PROXYAGENTOCTOCLAIM >0
	agentclaim_t myClaim = myAbstractAgentOctoClaim[get_tile_id()];

	if (ALWAYS_USE_PROXY || myClaim == NULL)
	{
		printf("Uses ProxyAOC to access AOC.\n");
		isProxy = true;

		myClaim = agent_proxy_get_proxyagentoctoclaim(objects_tile_param, ucid_param); // gets a new proxyagentoctoclaim which allows access to the iLet's original AgentOctoClaim which is located at the objects_tile (runs RPCs targeted at its objects_tile and octo_ucid). Works on any tile.
		printf("agent_proxy_get_proxyagentoctoclaim called and received a ProxyAgentOctoClaim!\n");
		int objects_tile_received = agent_proxy_get_objectstile(myClaim); // Crashes when called with AOC
		printf("agent_proxy_get_objectstile = %i\n", objects_tile_received);
	}
	else
	{
		printf("Accesses AOC directly.\n");
	}

	uint32_t ucid_received = agent_claim_get_ucid(myClaim);
	printf("agent_claim_get_ucid = %" PRIu32 "\n", ucid_received);

	int resource_count_received = agent_claim_get_pecount(myClaim);
	printf("agent_claim_get_pecount = %u\n", resource_count_received);

	agent_claim_print(myClaim); // don't forget to turn #define SUB_AGENT_ON 1 in src/lib/debug-cfg.h
	printf("agent_claim_print has been called.\n");

	bool isempty_received = agent_claim_isempty(myClaim);
	printf("agent_claim_isempty = %s\n", isempty_received ? "Yes" : "No");

	int pecounttype_received = agent_claim_get_pecount_type(myClaim, 0); // Gets the total number of resources of type RISC (==0) in claim myClaim.
	printf("agent_claim_get_pecount_type = %d\n", pecounttype_received);

	int pecounttile_received = agent_claim_get_pecount_tile(myClaim, get_tile_id()); // Gets the total number of resources of type RISC (==0) on THIS tile in claim myClaim.
	printf("agent_claim_get_pecount_tile = %d\n", pecounttile_received);

	int pecounttiletype_received = agent_claim_get_pecount_tile_type(myClaim, get_tile_id(), 0); // Gets the total number of resources of type RISC (==0) on THIS tile in claim myClaim.
	printf("agent_claim_get_pecount_tile_type = %d\n", pecounttiletype_received);

	int tilecount_received = agent_claim_get_tilecount(myClaim); // Gets the total number of resources of type RISC (==0) on THIS tile in claim myClaim.
	printf("agent_claim_get_tilecount = %d\n", tilecount_received);

	int iterator = 0;
	int tilecountiter_received_0 = agent_claim_get_tileid_iterative(myClaim, iterator); // If myClaim hasTile(i) at (iterator'th) position counting 0th position to nth not by tileID but by tiles that myClaim has. Else returns 0xFF (255). Hard to explain; just look at code in AgentClaim::getTileID. A comment in the code is: "this function doesn't make too much sense as it is..". Anyhow I wanted to implement all AgentOctoClaim's functions for ProxyAgentOctoClaim.
	printf("agent_claim_get_tileid_iterative (0)= %d\n", tilecountiter_received_0);
	iterator = 1;
	int tilecountiter_received_1 = agent_claim_get_tileid_iterative(myClaim, iterator);
	printf("agent_claim_get_tileid_iterative (1)= %d\n", tilecountiter_received_1);

	if ((objects_tile_param == get_tile_id()) || PROVOKE_CRASH_AGENT_CLAIM_GET_AGENT)
	{
		agent_t owning_agent_received = agent_claim_get_agent(myClaim); // Returns the AgentInstance handle of myClaim. Panics when tries to access AgentInstance from another tile than where the object is located
		printf("agent_claim_get_agent called. If you see this message, you called this function from the tile where the AOC objects are located.\n");
	}

	if ((objects_tile_param == get_tile_id()) || PROVOKE_CRASH_AGENT_CLAIM_GET_CONSTR)
	{
		constraints_t constraints_received = agent_claim_get_constr(myClaim); // Returns the constraints_t of myClaim. Panics when tries to access Constraints from another tile than where the object is located
		printf("agent_claim_get_constr called. If you see this message, you called this function from the tile where the AOC objects are located.\n");
	}

	if ((objects_tile_param != get_tile_id()) && PROVOKE_CRASH_AGENT_CLAIM_INVADE_PARENTCLAIM)
	{
		// invading its own parent claim does not work on objects_tile_param == get_tile_id() either. (results in "Assertion failed: (check_sticky_invariant(slot))") Therefore only test this on objects_tile_param != get_tile_id()
		constraints_t constraints_new = agent_constr_create();	 // Creates new constraints object on this tile. Problematic if we want to invade distant Agent(Octo)Claim objects with that.
		agent_constr_set_quantity(constraints_new, pes, pes, 0); // minimum 1, maximum 1 PEs
		agent_constr_set_tile_shareable(constraints_new, 1);
		agentclaim_t claim_received = agent_claim_invade_parentclaim(myClaim, constraints_new); // Invades new resources and returns the OctoAgentClaim handle. Panics when tries to access myClaim from another tile than where the object is located
																																														// btw you will see the error message of ProxyAgentOctoClaim::getOwningAgent() because this function is called first in agent_claim_invade_parentclaim
	}

	if (PROVOKE_CRASH_AGENT_CLAIM_REINVADE_CONSTRAINTS)
	{
		constraints_t constraints_new = agent_constr_create();	 // Creates new constraints object on this tile. Problematic if we want to invade distant Agent(Octo)Claim objects with that.
		agent_constr_set_quantity(constraints_new, pes, pes, 0); // minimum 1, maximum 1 PEs
		agent_constr_set_tile_shareable(constraints_new, 1);
		int changes_sum_received = agent_claim_reinvade_constraints(myClaim, constraints_new); // Reinvades myClaim with new constraints, returns sum of changes. Panics when tries to access myClaim from another tile than where the object is located
		// btw you will see the error message of ProxyAgentOctoClaim::getOriginalAgentOctoClaim because this function is called first in agent_claim_reinvade_constraints on a ProxyAgentOctoClaim.
		printf("agent_claim_reinvade_constraints called. The reinvade process changes the AgentOctoClaim's AgentClaim, so the ProxyAgentOctoClaim will be not valid anymore. Will produce error with the next simple ProxyAgentOctoClaim function isEmpty in case of using a PAOC. Else will produce error later if functions with are executed with PAOCs in another iLet.\n");
		printf("After reinvade: agent_claim_isempty = %s\n", agent_claim_isempty(myClaim) ? "Yes" : "No");
	}

	if ((objects_tile_param == get_tile_id()) || PROVOKE_CRASH_AGENT_CLAIM_GET_PROXYCLAIM_TILE_TYPE)
	{
		proxy_claim_t proxyclaim_received = agent_claim_get_proxyclaim_tile_type(myClaim, objects_tile_param, 0); // Gets an OctoPOS ProxyClaim handle for resources on tile objects_tile_param of type RISC=0 in a claim.. Panics when tries to access AgentInstance from another tile than where the object is located
		printf("agent_claim_get_proxyclaim_tile_type called. If you see this message, you called this function from the tile where the AOC objects are located.\n");
	}

	if ((objects_tile_param != get_tile_id()) && PROVOKE_CRASH_AGENT_CLAIM_RETREAT)
	{
		// retreating its own claim does not work on objects_tile_param == get_tile_id() either. (results in "error: Application wants to retreat claim from within") Therefore only test this on objects_tile_param != get_tile_id()
		agent_claim_retreat(myClaim);
		// btw you will see the error message of ProxyAgentOctoClaim::getOriginalAgentOctoClaim because this function is called first on a ProxyAgentOctoClaim in agent_claim_retreat
	}

	if (PROVOKE_CRASH_AGENT_CLAIM_REINVADE)
	{
		printf("Before reinvade: agent_claim_get_ucid = %" PRIu32 "\n", agent_claim_get_ucid(myClaim));
		int changes_sum_received = agent_claim_reinvade(myClaim);		 // Reinvades myClaim with old constraints, returns sum of changes.
		printf("agent_claim_reinvade = %d\n", changes_sum_received); // prints absolute sum of changes (i.e., gained and lost resources)
		printf("agent_claim_reinvade called. The reinvade process changes the AgentOctoClaim's AgentClaim, so the ProxyAgentOctoClaim will be not valid anymore. Will produce error with the next function in case of using a ProxyAgentOctoClaim.\n");
		// no Error is thrown up to here, reinvades are allowed. But all PAOC functions (thereafter) check whether the AgentOctoClaim object has changed its ucid or address. A reinvade changes the ucid. To check for changes, another function call is made here to provoke a crash.
		printf("After reinvade: agent_claim_get_ucid = %" PRIu32 "\n", agent_claim_get_ucid(myClaim)); // it does not matter which functions are tested. All C interface functions are able to handle a changed PAOC and produce an error message in all cases, because its ucid has changed.

		// another function we could use to test the PAOC for changes
		// printf("After reinvade: agent_claim_isempty = %s\n", agent_claim_isempty(myClaim) ? "Yes" : "No");
	}

	if (isProxy || PROVOKE_CRASH_PROXY_DELETE_PROXYAGENTOCTOCLAIM)
	{
		agent_proxy_delete_proxyagentoctoclaim(myClaim); // Helps avoiding memory leaks. Produces error message if called with an AOC.
		printf("Deleted ProxyAgentOctoClaim.\n");
	}

	printf("End of iLet on tile %u running on core %u\n", get_tile_id(), get_cpu_id());

	// Do not shutdown because shutting down kills any other ilets, too.
	//shutdown(0);
}

extern "C" void main_ilet(claim_t claim)
{
	printf("***Main iLet on tile %d, core %d\n", get_tile_id(), get_cpu_id());
	int computeTileCount = get_compute_tile_count(); // Gets the number of compute tiles in the system. This number does not include I/O or TCPA tiles.

	if (claimsTotal > computeTileCount)
	{
		printf("***Error! Total number of claims exceeds compute tile count. Shutting down.\n");
	}

	agentclaim_t initialClaim = agent_claim_get_initial(claim);

	printf("***Invading %d Claims:\n", claimsTotal);
	for (int i = 0; i < claimsTotal; i++)
	{
		myConstr[i] = agent_constr_create();
		agent_constr_set_quantity(myConstr[i], pes, pes, 0); // minimum 1, maximum 1 PEs
		agent_constr_set_tile_shareable(myConstr[i], 1);
		int allowedTile = i;														 // allows only tile i for the ith claim
		agent_constr_set_tile(myConstr[i], allowedTile); // allows only tile allowedTile for myConstr[i]
		myAbstractAgentOctoClaim[i] = agent_claim_invade(NULL, myConstr[i]);
		myUcidArray[i] = agent_claim_get_ucid(myAbstractAgentOctoClaim[i]); // ucid is needed for letting the iLet know what AgentOctoClaim it belonged to

		if (!myAbstractAgentOctoClaim[i])
		{
			fprintf(stderr, "***Invade operation unsuccessful.\n");
			abort();
		}

		printf("***Returned Claim %d of Size %d with ucid %" PRIu32 "\n", i, agent_claim_get_pecount(myAbstractAgentOctoClaim[i]), myUcidArray[i]);
	}

	int tile;
	res_type_t risc = 0;
	simple_ilet ILet[claimsTotal][pes];
	myObjectsTile = get_tile_id(); // defined where the iLets will access their AgentOctoClaims via proxies

	for (int i = 0; i < claimsTotal; ++i)
	{
		int currentTile = i;
		myProxyClaim[i] = agent_claim_get_proxyclaim_tile_type(myAbstractAgentOctoClaim[i], currentTile, risc); // get OctoPOS ProxyClaim handle for the AgentOctoClaim handle myAbstractAgentOctoClaim[i], on tile id currentTile, type RISC. The ProxyClaim handle is necessary for actual infecting the previously invaded resources.
		printf("***Got Proxy Claim %p on tile %d\n", myProxyClaim[i], currentTile);															// any iLet corresponding to a ProxyClaim which is defined on tile != 0 won't be able to access its AgentOctoClaim(/ProxyClaim/Constraints) handle without a ProxyAgentClaim mechanism.
		for (int iletnr = 0; iletnr < pes; ++iletnr)
		{
			dual_ilet_init(&ILet[i][iletnr], HelloWorldILet, (void *)&myUcidArray[i], (void *)&myObjectsTile); // transfers myUcidArray[i] and myObjectsTile to iLet. This allows for ProxyAgentClaim creation.
		}

		printf("***Infecting %d iLets on tile %d\n", pes, currentTile);
		proxy_infect(myProxyClaim[i], &ILet[i][0], pes); // infects the iLet on the tile defined by its ProxyClaim (see agent_claim_get_proxyclaim_tile_type)
	}

	// Does not retreat claims. If you want to retreat, you must wait for the ilets to finish their work of course.
	// You need to do signaling for that. See "reinvade" app e.g. But here we already gave our iLet 2 parameters. Need 1 more for the signal. Not possible to do it like that yet.

	// Do not shutdown because shutting down kills any other ilets, too, if we do not wait for the ilets to finish their work (via signals, look above).
	//shutdown(0);
	return;
}
