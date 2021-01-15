#include "ProxyAgentOctoClaim.h"
#include "octo_agent3.h"
#include <inttypes.h>

// see ProxyAgentOctoClaim.h for code documentation

// First: initialization of static variables
os::agent::AgentOctoClaim *os::agent::ProxyAgentOctoClaim::octoclaim_for_rpc = NULL;

// Then: implementation of methods

int os::agent::ProxyAgentOctoClaim::getObjectsTile() const
{
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.
	return this->objects_tile;
}

uint32_t os::agent::ProxyAgentOctoClaim::getUcid() const
{
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.
	return this->octo_ucid;
}

// default value of tag is -1 (which means not initialized yet or undefined)
os::agent::ProxyAgentOctoClaim::ProxyAgentOctoClaim(int objects_tile_param, uint32_t octo_ucid_param)
{
	if (objects_tile_param < 0)
	{
		panic("ProxyAgentOctoClaim constructor called with objects_tile_param < 0");
	}

	this->objects_tile = objects_tile_param;
	this->octo_ucid = octo_ucid_param;

	// RPC call to objects_tile_param and try to access fields to check for access sanity and set this->originalAgentOctoClaim
	this->originalAgentOctoClaim = os::agent::Agent::proxyAOC_get_AOC_address(this->objects_tile, this->octo_ucid);
	if (this->originalAgentOctoClaim == 0)
	{
		panic("Could not create ProxyAgentOctoClaim. Has not found its AgentOctoClaim address. See DBG for more details.");
	}
	// no need to call deleteDispatchClaimForRPC() because we have not created a new DispatchClaim yet and just used the AgentSystem's DC in the code above.
}

void os::agent::ProxyAgentOctoClaim::checkAgentOctoClaimAlive() const
{
	uintptr_t newAddress = os::agent::Agent::proxyAOC_get_AOC_address(this->objects_tile, this->octo_ucid);

	if (newAddress != originalAgentOctoClaim)
	{
		DBG(SUB_AGENT, "checkAgentOctoClaimAlive newAddress is 0x%" PRIxPTR "; original AgentOctoClaim's address was 0x%" PRIxPTR "\n", newAddress, originalAgentOctoClaim);
		panic("checkAgentOctoClaimAlive: Could not verify AgentOctoClaims's existence. Has not found its address. See DBG for more details.");
	}
	// no need to call deleteDispatchClaimForRPC() because we have not created a new DispatchClaim yet and just used the AgentSystem's DC in the code above.
}

os::agent::ProxyAgentOctoClaim::~ProxyAgentOctoClaim()
{
	// don't do a RPC call to objects_tile_param and delete ProxyClaims etc. there, because maybe we just want to delete the ProxyAgentOctoClaim Object and not its AgentOctoClaim.
	// Deleting an AgentOctoClaim object should be done directly. So make a call to getOriginalAgentOctoClaim() first and then delete it like shown in C Interface in agent_claim_retreat.

	this->objects_tile = -1;
	this->octo_ucid = (uint32_t)0;
	this->originalAgentOctoClaim = (uint32_t)0;
}

void *os::agent::ProxyAgentOctoClaim::operator new(size_t size) throw()
{
	return os::agent::AgentMemory::agent_mem_allocate(size);
}

void os::agent::ProxyAgentOctoClaim::operator delete(void *p)
{
	os::agent::AgentMemory::agent_mem_free(p);
}

os::agent::AgentOctoClaim *os::agent::ProxyAgentOctoClaim::getOriginalAgentOctoClaim() const
{
	if (hw::hal::Tile::getTileID() != this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from other tile where the AgentOctoClaim object is
		DBG(SUB_AGENT, "ProxyAgentOctoClaim::getOriginalAgentOctoClaim: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
		panic("ProxyAgentOctoClaim::getOriginalAgentOctoClaim: ProxyAgentOctoClaim is accessed from other tile where the AgentOctoClaim object is!");
	}

	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getOriginalAgentOctoClaim\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	// now we're on same tile like objects_tile and access to original AgentOctoClaim is safe
	AgentOctoClaim *original_aoc = reinterpret_cast<AgentOctoClaim *>(this->originalAgentOctoClaim);
	/*TODO maybe better do it like this?
	volatile uintptr_t iptr = 0xdeadbeef;
	unsigned int *ptr = (unsigned int *)iptr;
	see https://www.securecoding.cert.org/confluence/display/c/INT36-C.+Converting+a+pointer+to+integer+or+integer+to+pointer */
	return original_aoc;
}

os::res::DispatchClaim os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(int objects_tile)
{
	os::agent::AndConstraintList *acl = new os::agent::AndConstraintList();
	os::agent::PEQuantityConstraint *peqc = new os::agent::PEQuantityConstraint(acl);
	os::agent::TileConstraint *tc = new os::agent::TileConstraint(acl);
	os::agent::TileSharingConstraint *tsc = new os::agent::TileSharingConstraint(acl);
	os::agent::AgentConstraint *constr_for_rpc = acl;

	peqc->setQuantity(1, 1, (os::agent::HWType)0); // 1 of type 0==RISC PE
	acl->addConstraint(peqc);

	tc->allowTile(objects_tile);
	acl->addConstraint(tc);

	acl->addConstraint(tsc); // TileSharingConstraint allows sharing per default

	os::agent::AgentInstance *agentinstance_for_rpc = os::agent::Agent::createAgent();
	os::agent::AgentClaim *agentclaim_for_rpc = new os::agent::AgentClaim();
	if (!agentclaim_for_rpc)
	{
		panic("ProxyAgentOctoClaim::getDispatchClaimForRPC: new os::agent:AgentClaim failed!");
	}
	(*agentclaim_for_rpc) = os::agent::Agent::invade_agent_constraints(agentinstance_for_rpc, constr_for_rpc); // do the Bargaining
	if (agentclaim_for_rpc->getOwningAgent() != agentinstance_for_rpc)
	{
		DBG(SUB_AGENT, "ProxyAgentOctoClaim::getDispatchClaimForRPC: AgentClaim Belongs to %p, expected %p\n", agentclaim_for_rpc->getOwningAgent(), agentinstance_for_rpc);
		panic("ProxyAgentOctoClaim::getDispatchClaimForRPC: Wrong agentclaim_for_rpc owner!");
	}
	if (agentclaim_for_rpc->isEmpty())
	{
		panic("ProxyAgentOctoClaim::getDispatchClaimForRPC: agentclaim_for_rpc is empty!");
	}
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getDispatchClaimForRPC: Transforming AgentClaim to AgentOctoClaim!\n");
	os::agent::AgentOctoClaim *octo_for_rpc = new os::agent::AgentOctoClaim(*agentclaim_for_rpc, constr_for_rpc, NULL); // initialize the Configuration of OctoPOS and CiC
	if (!octo_for_rpc)
	{
		panic("ProxyAgentOctoClaim::getDispatchClaimForRPC: new os::agent:AgentOctoClaim failed!");
	}
	if (octo_for_rpc->getOwningAgent() != agentinstance_for_rpc)
	{
		DBG(SUB_AGENT, "ProxyAgentOctoClaim::getDispatchClaimForRPC: OctoAgentClaim Belongs to %p, expected %p\n", octo_for_rpc->getOwningAgent(), agentinstance_for_rpc);
		panic("ProxyAgentOctoClaim::getDispatchClaimForRPC: Wrong octo_for_rpc owner!");
	}
	os::agent::Agent::register_AgentOctoClaim(agentinstance_for_rpc, agentclaim_for_rpc->getUcid(), octo_for_rpc, os::res::DispatchClaim::getOwnDispatchClaim());
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getDispatchClaimForRPC: Waiting for remote configurations to finish!\n");
	octo_for_rpc->invadeAgentClaim_finish(); // force the Remote-Configuration of OctoPOS and CiC to finish.
	os::agent::Agent::signalOctoPOSConfigDone(octo_for_rpc->getOwningAgent(), octo_for_rpc->getUcid());
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getDispatchClaimForRPC: Done!\n");

	os::res::ProxyClaim *pc_for_rpc = octo_for_rpc->getProxyClaim((unsigned int)objects_tile, ResType::RISC); // get OctoPOS ProxyClaim handle for the AgentOctoClaim handle octo_for_rpc, on tile id objects_tile, type 0==RISC.

	os::agent::ProxyAgentOctoClaim::octoclaim_for_rpc = octo_for_rpc;
	return *pc_for_rpc; // we want no pointer
}

void os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC()
{
	if (!os::agent::ProxyAgentOctoClaim::octoclaim_for_rpc)
		panic("ProxyAgentOctoClaim::deleteDispatchClaimForRPC octoclaim_for_rpc == NULL");

	os::agent::AgentOctoClaim *cl = os::agent::ProxyAgentOctoClaim::octoclaim_for_rpc; // shorter name
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::deleteDispatchClaimForRPC: retreating octoclaim on address %p\n", cl);

	// check if we are within the agentclaim
	for (int inType = 0; inType < os::agent::HWTypes; inType++)
	{
		if (cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType) &&
				cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType)->getTag().value == os::res::DispatchClaim::getOwnDispatchClaim().getTag().value)
		{

			DBG(SUB_AGENT, "Type %d, Tile %d, Tag %d/%d\n", inType,
					os::res::DispatchClaim::getOwnDispatchClaim().getTID(),
					cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType)->getTag().value,
					os::res::DispatchClaim::getOwnDispatchClaim().getTag().value);

			panic("ProxyAgentOctoClaim::deleteDispatchClaimForRPC wants to retreat claim from within.");
		}
	}

	if (cl->getOwningAgent() == NULL)
	{
		panic("ProxyAgentOctoClaim::deleteDispatchClaimForRPC: Retreating Claim which doesn't belong to any Agent.");
	}

	cl->retreat();																																	 // adapt OctoPOS configurations
	bool keep = os::agent::Agent::pure_retreat(cl->getOwningAgent(), cl->getUcid()); // give away all resources
	if (!keep)
	{
		os::agent::Agent::deleteAgent(cl->getOwningAgent());
	}

	// free local stuff
	delete cl->getAgentClaim(); // dangerous with the current way of retreating?
	delete cl;									// dangerous with the current way of retreating?

	os::agent::ProxyAgentOctoClaim::octoclaim_for_rpc = NULL; // TODO not sure if this line is needed
}

uint8_t os::agent::ProxyAgentOctoClaim::getResourceCount() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getResourceCount\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getResourceCount();
	}

	// RPC to this->objects_tile, execute getResourceCount() on the AgentOctoClaim object there and return its value
	uint8_t res_count = os::agent::Agent::proxyAOC_get_ResourceCount(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
	return res_count;
}

//Anuj: Do this function need to be implemented???
uint8_t os::agent::ProxyAgentOctoClaim::getOperatingPointIndex() const
{
	return 101;
}

void os::agent::ProxyAgentOctoClaim::print() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::print\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		this->getOriginalAgentOctoClaim()->print();
		return;
	}

	// RPC to this->objects_tile, execute print() on the AgentOctoClaim object there
	os::agent::Agent::proxyAOC_print(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
}

bool os::agent::ProxyAgentOctoClaim::isEmpty() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::isEmpty\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->isEmpty();
	}

	// RPC to this->objects_tile, execute isEmpty() on the AgentOctoClaim object there and return its value
	bool isempty = os::agent::Agent::proxyAOC_isEmpty(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
	return isempty;
}

uint8_t os::agent::ProxyAgentOctoClaim::getQuantity(os::agent::TileID tileID, os::agent::HWType type) const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getQuantity\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getQuantity(tileID, type);
	}

	// RPC to this->objects_tile, execute getQuantity() on the AgentOctoClaim object there and return its value
	uint8_t quantity = os::agent::Agent::proxyAOC_getQuantity(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim, tileID, type);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
	return quantity;
}

uint8_t os::agent::ProxyAgentOctoClaim::getTileCount() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getTileCount\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getTileCount();
	}

	// RPC to this->objects_tile, execute getTileCount() on the AgentOctoClaim object there and return its value
	uint8_t tileCount = os::agent::Agent::proxyAOC_getTileCount(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
	return tileCount;
}

os::agent::TileID os::agent::ProxyAgentOctoClaim::getTileID(uint8_t iterator) const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getTileID\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getTileID(iterator);
	}

	// RPC to this->objects_tile, execute getTileID(iterator) on the AgentOctoClaim object there and return its value
	uint8_t tileID = os::agent::Agent::proxyAOC_getTileID(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim, iterator);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
	return tileID;
}

os::agent::AgentInstance *os::agent::ProxyAgentOctoClaim::getOwningAgent() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getOwningAgent\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getOwningAgent();
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to receive its owning AgentInstance. What to do? Linking won't work because of different address spaces. Creating a (deep) copy of the AgentInstance? Does not make sense, need to create copy of all its related AgentOctoClaim, Constraints, ProxyClaims etc objects in the end. Cannot return a "ProxyAgentInstance" at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getOwningAgent: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::getOwningAgent() called from different tile than its object's location. Cannot return its AgentInstance because we have no proxy implementation for that.");

	return NULL; // never called. Returned either when PAOC is accessed from same tile where AOC object is, or panics with error message.
}

os::agent::AgentConstraint *os::agent::ProxyAgentOctoClaim::getConstraints() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getConstraints\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getConstraints();
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to receive its Constraints. What to do? Linking won't work because of different address spaces. Creating a (deep) copy of the Constraints? Does not make sense, need to create copy of all its related AgentOctoClaim, Constraints, ProxyClaims etc objects in the end. Cannot return a "ProxyConstraints" at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getConstraints: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::getConstraints() called from different tile than its object's location. Cannot return its Constraints because we have no proxy implementation for that.");

	return NULL; // never called. Returned either when PAOC is accessed from same tile where AOC object is, or panics with error message.
}

os::res::ProxyClaim *os::agent::ProxyAgentOctoClaim::getProxyClaim(os::agent::TileID tileID, os::agent::HWType type) const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getProxyClaim\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getProxyClaim(tileID, type);
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to receive a ProxyClaim. What to do? Creating a new ProxyClaim object? Results in related objects being scattered on different tiles -> inconsistencies. Cannot return a "ProxyProxyClaim" at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getProxyClaim: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::getProxyClaim() called from different tile than its object's location. Cannot return a ProxyClaim object because we have no proxy implementation for that.");
}

void os::agent::ProxyAgentOctoClaim::invadeAgentClaim_finish()
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::invadeAgentClaim_finish\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		this->getOriginalAgentOctoClaim()->invadeAgentClaim_finish();
		return;
	}

	//This function is usually called in the C Interface function agent_claim_invade and forces the Remote-Configuration of OctoPOS and CiC to finish. It is done in the end phase of an invade, as the name suggests. But we cannot invade the AgentOctoClaim from a different tile than its object location. The AgentOctoClaim has to be setup already in order to be able to be infected with an Ilet. So it is impossible that we need to run this invade-phase-function from a distant tile within an ilet that is running and associated with that AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::invadeAgentClaim_finish: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::invadeAgentClaim_finish() called from different tile than its object's location. Cannot run this function remotely, because this ilet is associated with an AgentOctoClaim that has been infected and we already have finished the Invade phase.");
}

void os::agent::ProxyAgentOctoClaim::invadeAgentClaim_prepare()
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::invadeAgentClaim_prepare\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		this->getOriginalAgentOctoClaim()->invadeAgentClaim_prepare();
		return;
	}

	//This function is usually called in the constructor of an AgentOctoClaim. Among other things, it sets its array of ProxyClaim to NULL. Here we try to prepare the AgentOctoClaim from a different tile than its object location. But actually the AgentOctoClaim has to be setup already in order to be able to be infected with an Ilet (which needs a ProxyClaim != NULL). So it is impossible that we need to prepare the AOC (and set all its ProxyClaims NULL) from a distant tile in an ilet. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::invadeAgentClaim_prepare: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::invadeAgentClaim_prepare() called from different tile than its object's location. Cannot prepare an AgentOctoClaim remotely and reset all its ProxyClaims which are needed for this ilet.");
}

void os::agent::ProxyAgentOctoClaim::retreat()
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::retreat\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		this->getOriginalAgentOctoClaim()->retreat();
		return;
	}

	// here we try to retreat the AgentOctoClaim from different tile than its object location. The ProxyAgentOctoClaim implementation is only usable for accessing its own AOC in an ilet. So retreating in this situation implicates retreating its own claim (that is used in the ilet running on current tile) which is not possible. Therefore we should not RPC to the objects_tile and call ->retreat() there on the original AOC. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::retreat: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::retreat() called from different tile than its object's location. Cannot retreat its own claim which is used in the ilet running on current tile."); // current tile is hw::hal::Tile::getTileID()
}

void os::agent::ProxyAgentOctoClaim::setConstraints(os::agent::AgentConstraint *constr)
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::setConstraints\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		this->getOriginalAgentOctoClaim()->setConstraints(constr);
		return;
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to set Constraints. Cannot copy and set them via RPC because else this results in related objects being scattered on different tiles -> inconsistencies. Cannot set "ProxyConstraints" at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::setConstraints: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::setConstraints() called from different tile than its object's location. Cannot set Constraints because we have no proxy implementation for that.");
}

void os::agent::ProxyAgentOctoClaim::setOwningAgent(os::agent::AgentInstance *ag)
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::setOwningAgent\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		this->getOriginalAgentOctoClaim()->setOwningAgent(ag);
		return;
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to set Constraints. Cannot set its new owning agent via RPC because else this results in related objects being scattered on different tiles -> inconsistencies. Cannot set "ProxyAgentInstance" as owning agent at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::setOwningAgent: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::setOwningAgent() called from different tile than its object's location. Cannot set AgentInstance as owning agent because we have no proxy implementation for that.");
}

os::agent::AgentClaim *os::agent::ProxyAgentOctoClaim::getAgentClaim() const
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getAgentClaim\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->getAgentClaim();
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to receive a ProxyClaim. What to do? Creating a new AgentClaim object? Results in related objects being scattered on different tiles -> inconsistencies. Cannot return a "ProxyAgentClaim" at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::getAgentClaim: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::getAgentClaim() called from different tile than its object's location. Cannot return a AgentClaim object because we have no proxy implementation for that.");
}

int os::agent::ProxyAgentOctoClaim::adaptToAgentClaim_finish(os::agent::AgentClaim &newClaim, int numberCoresGained)
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::adaptToAgentClaim_finish\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->adaptToAgentClaim_finish(newClaim, numberCoresGained);
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to involve a new AgentClaim. Cannot do this via RPC because else this results in related objects being scattered on different tiles -> inconsistencies. Cannot do it with a "ProxyAgentClaim" object at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::adaptToAgentClaim_finish: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::adaptToAgentClaim_finish() called from different tile than its object's location. Cannot do this for the AgentClaim object because we have no proxy implementation for that.");
}

int os::agent::ProxyAgentOctoClaim::adaptToAgentClaim_prepare(os::agent::AgentClaim &newClaim)
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::adaptToAgentClaim_prepare\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return this->getOriginalAgentOctoClaim()->adaptToAgentClaim_prepare(newClaim);
	}

	// here we try to access the AgentOctoClaim from different tile than its object location and want to involve a new AgentClaim. Cannot do this via RPC because else this results in related objects being scattered on different tiles -> inconsistencies. Cannot do it with a "ProxyAgentClaim" object at the moment, because we only have implemented proxy access to AgentOctoClaim. Prints error message.
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::adaptToAgentClaim_prepare: current tile = %" PRIu8 ", objects_tile = %i\n", hw::hal::Tile::getTileID(), this->objects_tile);
	panic("ProxyAgentOctoClaim::adaptToAgentClaim_prepare() called from different tile than its object's location. Cannot do this for the AgentClaim object because we have no proxy implementation for that.");
}

int os::agent::ProxyAgentOctoClaim::reinvadeSameConstr()
{
	DBG(SUB_AGENT, "ProxyAgentOctoClaim::reinvadeSameConstr. Be careful that this function changes the ucid of the AgentOctoClaim, so the ProxyAgentOctoClaim won't be usable anymore for any other function calls after the reinvade.\n");
	checkAgentOctoClaimAlive(); // checks if AgentOctoClaim has changed its ucid or address or is not registered in the AgentSystem anymore. Panics if AOC is not accessible.

	if (hw::hal::Tile::getTileID() == this->objects_tile)
	{
		// ProxyAgentOctoClaim is accessed from same tile where the AgentOctoClaim object is
		return agent_claim_reinvade(this->getOriginalAgentOctoClaim());
	}

	// RPC to this->objects_tile, execute agent_claim_reinvade with the AgentOctoClaim object there and return its value
	int rc = os::agent::Agent::proxyAOC_reinvadeSameConstraints(os::agent::ProxyAgentOctoClaim::getDispatchClaimForRPC(this->objects_tile), this->originalAgentOctoClaim);
	os::agent::ProxyAgentOctoClaim::deleteDispatchClaimForRPC(); // must be called ONCE every time after a RPC call to free resources
	return rc;
}
