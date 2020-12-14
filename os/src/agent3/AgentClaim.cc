#include "os/agent3/AgentClaim.h"
#include "os/agent3/Agent.h"

lib::adt::AtomicID os::agent::AgentClaim::ucidCreator;

os::agent::ResourceRating os::agent::AgentMalleabilityClaim::costOfLosingResource(const os::agent::ResourceID &res)
{
	return claim->getOwningAgent()->costOfLosingResource(slot, res);
}

void os::agent::AgentClaim::removeClaim(const os::agent::AgentClaim &other)
{
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if ((none != os::agent::HardwareMap[res.tileID][res.resourceID].type) && (other.contains(res)) && (this->contains(res)))
			{
				this->remove(res);
			}
		}
	}
}

void os::agent::AgentClaim::intersectClaim(const os::agent::AgentClaim &other)
{
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if ((none != os::agent::HardwareMap[res.tileID][res.resourceID].type) && (this->contains(res)) && (!(other.contains(res))))
			{
				this->remove(res);
			}
		}
	}
}

void os::agent::AgentClaim::createMalleabilityClaim(os::agent::AgentMalleabilityClaim **claim, int slot,
																										uint8_t numAbdicableResources[os::agent::HWTypes])
{
	if (*claim)
	{
		(**claim) = AgentMalleabilityClaim(this, slot, resources, numAbdicableResources);
	}
	else
	{
		*claim = new AgentMalleabilityClaim(this, slot, resources, numAbdicableResources);
	}
}

void os::agent::AgentMalleabilityClaim::intersect(const os::agent::AgentClaim &other)
{
	for (uint32_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; ++i)
	{
		resources[i] = claim->getTileFlags(i) & other.getTileFlags(i);
	}
}

int os::agent::AgentOctoClaim::adaptToAgentClaim_prepare(AgentClaim &newClaim)
{

	if (adapting)
		panic("os::agent::AgentOctoClaim::adaptToAgentClaim_prepare: Already adapting claim!");
	adapting = true;

	int retVal = 0;

	os::res::ProxyClaim::Future reinvfutures[hw::hal::Tile::MAX_TILE_COUNT * HWTypes]; // = reinterpret_cast<os::res::ProxyClaim::ReInvFuture*>(fut);

	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			is_count[tileID][type] = myAgentClaim->getQuantity(tileID, (HWType)type);
			should_count[tileID][type] = newClaim.getQuantity(tileID, (HWType)type);
		}
	}

	int futurecounter = 0;
	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			int quantity = should_count[tileID][type] - is_count[tileID][type];
			if (quantity <= 0)
			{
				continue;
			}
			if (type != RISC && type != iCore && type != TCPA)
			{
				DBG(SUB_AGENT, "Invade Type != RISC or iCore => FAIL\n");
				continue;
			}
			retVal++;
			uintptr_t resourceMap = newClaim.getResourceMap(tileID, type);
			if (is_count[tileID][type] == 0)
			{ // use invade to create new proxyclaims
				os::res::ProxyClaim::invadeCores(tileID, &(futures[futurecounter]), resourceMap);
			}
			else
			{ // use reinvade to adapt existing proxyclaim
				proxyClaims[tileID][type]->reinvadeCores(&(reinvfutures[futurecounter]), resourceMap);
			}
			futurecounter++;
		}
	}

	// forcing Futures
	futurecounter = 0;

	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			int quantity = should_count[tileID][type] - is_count[tileID][type];
			if (quantity <= 0 || (type != RISC && type != iCore))
			{
				continue;
			}
			if (is_count[tileID][type] == 0)
			{ // used invade to create new proxyclaims
				futures[futurecounter].force();
				proxyClaims[tileID][type] = os::res::ProxyClaim::getClaim(&(futures[futurecounter]));
				if (!proxyClaims[tileID][type])
				{
					DBG(SUB_AGENT, "Tile %d, type %d, quantity %d\n", tileID, type, quantity);
					panic("os::agent::AgentOctoClaim::adaptToAgentClaim_prepare: Got NULL Proxyclaim!");
				}
			}
			else
			{
				reinvfutures[futurecounter].force();
			}
			futurecounter++;
		}
	}

	return retVal;
}

int os::agent::AgentOctoClaim::adaptToAgentClaim_finish(AgentClaim &newClaim, int numberCoresGained)
{

	if (!adapting)
		panic("os::agent::AgentOctoClaim::adaptToAgentClaim_finish: Finishing adaptation w/o adaptation going on!");

	int retVal = numberCoresGained;

	os::res::ProxyClaim::Future retfutures[hw::hal::Tile::MAX_TILE_COUNT * HWTypes];	 // = reinterpret_cast<os::res::ProxyClaim::Future*>(fut);
	os::res::ProxyClaim::Future reinvfutures[hw::hal::Tile::MAX_TILE_COUNT * HWTypes]; // = reinterpret_cast<os::res::ProxyClaim::ReInvFuture*>(fut);

	// Now it is safe to release Lost Cores.
	int futurecounter = 0;
	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			int quantity = should_count[tileID][type] - is_count[tileID][type];
			if (quantity >= 0)
			{
				continue;
			}
			if (type != RISC && type != iCore && type != TCPA)
			{
				DBG(SUB_AGENT, "Invade Type != RISC => FAIL\n");
				continue;
			}
			retVal++;
			if (should_count[tileID][type] == 0)
			{ // use retreat to releaseproxyclaims
				retfutures[futurecounter].init();
				proxyClaims[tileID][type]->retreat(&(retfutures[futurecounter]));
			}
			else
			{ // use reinvade to adapt existing proxyclaim
				reinvfutures[futurecounter].init();
				proxyClaims[tileID][type]->reinvadeCores(&(reinvfutures[futurecounter]), newClaim.getResourceMap(tileID, type));
			}

			futurecounter++;
		}
	}

	// forcing Futures
	futurecounter = 0;

	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			int quantity = should_count[tileID][type] - is_count[tileID][type];
			if (quantity >= 0 || (type != RISC && type != iCore && type != TCPA))
			{
				continue;
			}
			if (should_count[tileID][type] == 0)
			{ // used invade to create new proxyclaims
				retfutures[futurecounter].force();
				proxyClaims[tileID][type] = NULL;
			}
			else
			{
				reinvfutures[futurecounter].force();
			}
			futurecounter++;
		}
	}

	// TAKE CARE WITH THIS ONE! Maybe we should copy the AgentClaim..
	if (this->myAgentClaim != &newClaim)
	{
		delete this->myAgentClaim;
	}

	this->myAgentClaim = &newClaim;
	this->myAgentClaim->setAgentOctoClaim(*this);

	adapting = false;

	return retVal; // total sum of changes (i.e. losing 2, gaining 1 would return: 3)
}

void os::agent::AgentOctoClaim::invadeAgentClaim_prepare()
{ // After the Agent-System made decisions, the OctoPOS and CiC need to be updated

	int futurecounter = 0;
	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			proxyClaims[tileID][type] = NULL;
			uint8_t quantity = myAgentClaim->getQuantity(tileID, (HWType)type);
			if (quantity <= 0)
			{
				continue;
			}
			if (type != RISC && type != iCore && type != TCPA)
			{
				DBG(SUB_AGENT, "Invade Type != RISC || iCore=> FAIL\n");
				continue;
			}
			DBG(SUB_AGENT, "Quantity = %d\n", quantity);
			futures[futurecounter].init();
			os::res::ProxyClaim::invadeCores(tileID, &(futures[futurecounter]), myAgentClaim->getResourceMap(tileID, type));
			futurecounter++;
		}
	}
}

void os::agent::AgentOctoClaim::invadeAgentClaim_finish()
{
	int futurecounter = 0;
	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		for (int type = 0; type < HWTypes; type++)
		{
			if (myAgentClaim->getQuantity(tileID, (HWType)type) <= 0)
			{
				continue;
			}
			if (type != RISC && type != iCore && type != TCPA)
			{
				continue;
			}
			futures[futurecounter].force();
			proxyClaims[tileID][type] = os::res::ProxyClaim::getClaim(&(futures[futurecounter]));
			DBG(SUB_AGENT, "Got PC: Tile %d, Type %d, Count %d, proxyClaim %p, Tag %d\n", tileID, type, myAgentClaim->getQuantity(tileID, (HWType)type), proxyClaims[tileID][type], proxyClaims[tileID][type]->getDispatchInfo().getTag().value);
			futurecounter++;
			if (!proxyClaims[tileID][type])
			{
				DBG(SUB_AGENT, "Could not invade Cores my Agent said I could invade :(  Tile %d %d, %d\n", tileID, type, myAgentClaim->getQuantity(tileID, (HWType)type));
				DBG(SUB_AGENT, "Got PC: Tile %d, Type %d, Count %d, proxyClaim %p\n", tileID, type, myAgentClaim->getQuantity(tileID, (HWType)type), proxyClaims[tileID][type]);
				panic("os::agent::AgentOctoClaim::invadeAgentClaim_finish(): proxyClaim == NULL\n");
			}
		}
	}
}

void os::agent::AgentOctoClaim::retreat()
{ // this releases the OctoPOS invaded resources AND informs the agent.

	os::res::ProxyClaim::Future retfutures[hw::hal::Tile::MAX_TILE_COUNT * HWTypes]; // = reinterpret_cast<os::res::ProxyClaim::Future*>(fut);

	int futurecounter = 0;

	// First retreat from all other tiles, then from our own tile.
	// The order is crucial because waiting for a remote retreat is not possible
	// if our own claim has already been disposed of.
	TID ownTileID = os::dev::Tile::getTileID();
	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		if (tileID != ownTileID)
		{
			retreatFromTile(tileID, retfutures, futurecounter);
		}
	}
	retreatFromTile(ownTileID, retfutures, futurecounter);

	while (futurecounter)
	{
		futurecounter--;
		retfutures[futurecounter].force();
	}
}

void os::agent::AgentOctoClaim::print() const
{

	static const char *ResTypeName[] = {"RISC", "iCore", "TCPA", "--", "ALL"};

	DBG(SUB_AGENT, "===CLAIM==[%4" PRIu32 "]=[%p]===================\n", getUcid(), this->getOwningAgent());
	for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
	{
		bool hasTile = false;
		for (int type = 0; type < HWTypes; type++)
		{

			if (!proxyClaims[tileID][type])
			{
				continue;
			}

			if (!hasTile)
			{
				hasTile = true;
				DBG(SUB_AGENT, "Tile %d (pC-TID: %d):\n", tileID, proxyClaims[tileID][type]->getTID());
			}
			DBG(SUB_AGENT, "	 Resourcetype %s: %d [%p]\n", ResTypeName[type], myAgentClaim->getQuantity(tileID, (HWType)type), proxyClaims[tileID][type]); //tileResources[res.tileID][type]);
		}
		if (hasTile)
			DBG(SUB_AGENT, "\n");
	}
}

void os::agent::AgentOctoClaim::retreatFromTile(TID tileID,
																								os::res::ProxyClaim::Future retfutures[], int &futurecounter)
{
	for (int type = 0; type < HWTypes; ++type)
	{
		if (proxyClaims[tileID][type] == nullptr)
		{
			continue;
		}
		retfutures[futurecounter].init();
		proxyClaims[tileID][type]->retreat(&retfutures[futurecounter]);
		proxyClaims[tileID][type] = nullptr;
		++futurecounter;
	}
}
