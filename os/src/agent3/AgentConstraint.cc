#include "os/agent3/Agent.h"
#include "os/agent3/AgentConstraint.h"

#include "os/dev/WallClock.h"

#include "lib/kassert.h"

//#include "os/agent/ActorConstraint.h"

const uint8_t os::agent::PEQuantityConstraint::EMPTY_CLAIM_RATING_MULTIPLIER = 50;

const uint16_t os::agent::DowneySpeedupConstraint::defaultA = 1500;		 // value scaled by factor 100, real parameter A for Downey model is '15'
const uint16_t os::agent::DowneySpeedupConstraint::defaultSigma = 100; // value scaled by factor 100, real parameter Sigma for Downey model is '1'

const uint32_t os::agent::SearchLimitConstraint::DEFAULT_TIME_LIMIT = 120000; // 2 minutes
const uint8_t os::agent::SearchLimitConstraint::DEFAULT_RATING_LIMIT = 99;		// 99 percent of maximum rating

const uint8_t os::agent::MalleableRatingConstraint::DEFAULT_MALLEABLE_RATING_MULTIPLIER = 95; // 5 percent penalty for malleable resources (compared to free ones)

void os::agent::ConstraintList::setImplicitConstraints()
{
	addConstraint(new os::agent::TileSharingConstraint(this));
	addConstraint(new os::agent::SearchLimitConstraint(this));
	addConstraint(new os::agent::MalleableRatingConstraint(this));
}

bool os::agent::AgentConstraint::isAgentConstraint() const
{
	DBG(SUB_AGENT, "It's AGENT CONSTRAINT\n");
	return true;
}

bool os::agent::AgentConstraint::isOptional() const
{
	return this->optional;
}

void os::agent::AgentConstraint::setOptional(const bool optional)
{
	this->optional = optional;
}

void os::agent::AgentConstraint::setDefaults()
{
	this->setOptional(false);
}

os::agent::AgentConstraint *os::agent::ConstraintList::searchConstraint(const ConstrType type)
{
	if (type == this->getCType())
	{
		return this;
	}
	else if (type == constraintCache.type)
	{
		return constraintCache.c;
	}

	//if this object is not meant, search in the list
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();

	for (; current != end; ++current)
	{
		AgentConstraint *dummy = (*current)->searchConstraint(type);
		if (dummy)
		{
			setConstraintCache(type, dummy);
			return dummy;
		}
	}

	return NULL;
}

const os::agent::AgentConstraint *os::agent::ConstraintList::searchConstraint(const ConstrType type) const
{
	if (type == this->getCType())
	{
		return this;
	}
	else if (type == constraintCache.type)
	{
		return constraintCache.c;
	}

	//if this object is not meant, search in the list
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();

	for (; current != end; ++current)
	{
		const AgentConstraint *dummy = (*current)->searchConstraint(type);
		if (dummy)
		{
			ConstraintList *nonConstThis = const_cast<ConstraintList *>(this);
			AgentConstraint *nonConstDummy = const_cast<AgentConstraint *>(dummy);
			nonConstThis->setConstraintCache(type, nonConstDummy);
			return dummy;
		}
	}

	return NULL;
}

bool os::agent::ConstraintList::addConstraint(AgentConstraint *c)
{
	//TODO: replace return parameter with ExceptionHandling?
	if (this->itemsUsed < this->arraySize)
	{
		list[this->itemsUsed] = c;
		this->itemsUsed++;
		return true;
	}
	else
	{
		panic("Constraint list is full!\n");
		return false;
	}
}

os::agent::ConstraintList::Iterator os::agent::ConstraintList::begin() const
{
	return ConstraintList::Iterator(this);
}

os::agent::ConstraintList::Iterator os::agent::ConstraintList::end() const
{
	return ConstraintList::Iterator(this, this->itemsUsed);
}

void os::agent::AndConstraintList::siftPool(AgentClaim &pool) const
{
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	for (; current != end; ++current)
	{
		(*current)->siftPool(pool);
	}
}

bool os::agent::AndConstraintList::fulfilledBy(const AgentClaim &claim, bool *improvable,
																							 const bool respect_max) const
{
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	for (; current != end; ++current)
	{
		bool check = (*current)->fulfilledBy(claim, improvable, respect_max);
		if (!check)
		{
			return false;
		}
	}

	return true; //if the list is empty, all Constraints are fulfilled by convention
}

bool os::agent::AndConstraintList::violatedBy(const AgentClaim &claim, const bool claimNotFulfills, const bool malleableContext) const
{
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	for (; current != end; ++current)
	{
		bool check = (*current)->violatedBy(claim, claimNotFulfills, malleableContext);
		if (check)
		{
			return true;
		}
	}

	return false;
}

os::agent::ResourceRating os::agent::AndConstraintList::determineMaxRating() const
{
	os::agent::ResourceRating rating = 100;
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	uint8_t cCounter = 0;
	for (; current != end; ++current)
	{
		rating *= (*current)->determineMaxRating();
		cCounter++;
		if (rating == 0)
		{
			return 0;
		}
		// prevent overflow
		if (cCounter >= 7)
		{
			rating /= 100;
			cCounter--;
		}
	}
	// normalize at the end to keep the precision high
	uint64_t divider = 1;
	for (; cCounter > 0; cCounter--)
	{
		divider *= 100;
	}
	rating /= divider;
	return rating;
}

os::agent::ResourceRating os::agent::AndConstraintList::rateClaim(const os::agent::AgentClaim &claim, const bool claimFulfills) const
{
	os::agent::ResourceRating rating = 100;
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	uint8_t cCounter = 0;
	for (; current != end; ++current)
	{
		os::agent::ResourceRating singleRating = (*current)->rateClaim(claim, claimFulfills);
		if (singleRating == 100)
		{
			continue;
		}
		else
		{
			rating *= singleRating;
			cCounter++;
		}

		if (rating == 0)
		{
			return 0;
		}
		// prevent overflow
		if (cCounter >= 7)
		{
			rating /= 100;
			cCounter--;
		}
	}
	// normalize at the end to keep the precision high
	uint64_t divider = 1;
	for (; cCounter > 0; cCounter--)
	{
		divider *= 100;
	}
	rating /= divider;
	return rating;
}

const os::agent::AgentConstraint *os::agent::AndConstraintList::__searchConstraintInternally(
		const ConstrType type, const AgentConstraint *caller) const
{
	// if the type is AND
	if (type == this->getCType())
	{
		return this;
	}

	// else if one of the list entries is of, or holds type
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	for (; current != end; ++current)
	{
		if (*current == caller)
		{
			// don't ask the one who asked us
			continue;
		}
		const AgentConstraint *dummy = (*current)->__searchConstraintInternally(type, this);
		if (dummy)
		{
			return dummy;
		}
	}

	// else if we are a list entry ourself, ask our parent (except if he called us in the first place)
	if (this->parent && this->parent != caller)
	{
		return this->parent->__searchConstraintInternally(type, this);
	}

	// else the type is simply not there
	return NULL;
}

bool os::agent::AndConstraintList::claimResources(const os::agent::AgentClaim &claim,
																									os::agent::AgentInstance *agent, int slot) const
{
	ConstraintList::Iterator current = this->begin();
	ConstraintList::Iterator end = this->end();
	for (; current != end; ++current)
	{
		bool success = (*current)->claimResources(claim, agent, slot);
		if (!success)
		{
			/*
			 * TODO: what do we do in this case? resources claimed earlier would need
			 * to release their stuff
			 * of course, we could just assume that this is done outside of this call,
			 * and that the caller simple checks the return value and goes on to free
			 * resources.
			 */
			return false;
		}
	}

	return true;
}

void os::agent::PEQuantityConstraint::siftPool(AgentClaim &pool) const
{
	os::agent::ResourceID res;

	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (pool.contains(res))
			{
				if (this->peConstr[HardwareMap[res.tileID][res.resourceID].type].max == 0)
				{
					pool.remove(res);
				}
			}
		}
	}
}

bool os::agent::PEQuantityConstraint::fulfilledBy(const AgentClaim &claim, bool *improvable,
																									const bool respect_max) const
{
	using namespace os::agent;

	for (uint8_t type = 0; type < HWTypes; ++type)
	{
		if (claim.getTypeCount(type) < this->peConstr[type].min // not enough resources
				|| claim.getTypeCount(type) > this->peConstr[type].max)
		{ // too many resources
			return false;
		}
	}

	return true;
}

bool os::agent::PEQuantityConstraint::violatedBy(const AgentClaim &claim, const bool claimNotFulfills, const bool malleableContext) const
{
	using namespace os::agent;

	for (uint8_t type = 0; type < HWTypes; ++type)
	{
		if (claim.getTypeCount(type) > this->peConstr[type].max)
		{
			return true;
		}
	}

	return false;
}

os::agent::ResourceRating os::agent::PEQuantityConstraint::determineMaxRating() const
{

	ResourceRating maxRating = 100;
	uint16_t a = parent->getDowneyA();
	uint16_t sigma = parent->getDowneySigma();
	for (uint8_t type = 0; type < HWTypes; ++type)
	{
		if (peConstr[type].max > 0)
		{
			maxRating *= downey_rate(peConstr[type].max, a, sigma);
			maxRating /= 100;
		}
	}
	return maxRating;
}

os::agent::ResourceRating os::agent::PEQuantityConstraint::rateClaim(const os::agent::AgentClaim &claim, const bool claimFulfills) const
{
	using namespace os::agent;
	ResourceRating rating = 100;
	uint16_t a = parent->getDowneyA();
	uint16_t sigma = parent->getDowneySigma();

	for (uint8_t type = 0; type < HWTypes; ++type)
	{
		uint16_t count = claim.getTypeCount(type);
		// return rating 0, when claim contains too many...
		if (count > this->peConstr[type].max)
		{
			DBG(SUB_AGENT, "peCount[%" PRIu8 "] is bigger than this->peConstr[%" PRIu8 "].max (%" PRIu8 " > %" PRIu8 ")\n", type, type, count, this->peConstr[type].max);
			return 0;

			// ...or too little resources
		}
		else if (count < this->peConstr[type].min)
		{
			DBG(SUB_AGENT, "peCount[%" PRIu8 "] is smaller than this->peConstr[%" PRIu8 "].min (%" PRIu8 " < %" PRIu8 ")\n", type, type, count, this->peConstr[type].min);
			return 0;
		}

		if (count > 0)
		{
			ResourceRating dRating = downey_rate(count, a, sigma);
			DBG(SUB_AGENT, "Downey rating %" PRIu64 " (percent) with parameters [#cores - downey A (percent) - downey sigma (percent)] = [%" PRIu16 " - %" PRIu16 " - %" PRIu16 "]\n", dRating, count, a, sigma);
			rating *= dRating;
			rating /= 100;
			// rate 90 if "no resources" is a valid solution (a valid claim with at least one resource will always rate >= 100)
		}
		else if (this->peConstr[type].max > 0 && this->peConstr[type].min == 0)
		{
			rating = rating * EMPTY_CLAIM_RATING_MULTIPLIER / 100;
		}
	}

	DBG(SUB_AGENT, "PEQuantity rates the claim at %" PRIu64 ".\n", rating);
	return rating;
}

bool os::agent::PEQuantityConstraint::claimResources(const os::agent::AgentClaim &claim,
																										 os::agent::AgentInstance *agent, int slot) const
{
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!claim.containsTile(res.tileID))
		{
			continue;
		}

		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (claim.contains(res))
			{
				if (!agent->claimOwnership(res, slot))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool os::agent::PEQuantityConstraint::canLoseResource(const os::agent::AgentClaim &claim,
																											const os::agent::ResourceID &resource) const
{
	typedef os::agent::AgentSystem AS;
	bool ret = true;

	if (claim.contains(resource))
	{
		ret = (this->peConstr[AS::getHWType(resource)].min < claim.getQuantity(os::agent::NOTILE,
																																					 AS::getHWType(resource)));
	}
	return ret;
}

bool os::agent::PEQuantityConstraint::isResourceUsable(const os::agent::ResourceID &resource) const
{
	typedef os::agent::AgentSystem AS;
	kassert(this->peConstr[AS::getHWType(resource)].min <= this->peConstr[AS::getHWType(resource)].max);
	bool ret = true;

	if (0 == this->peConstr[AS::getHWType(resource)].max)
	{
		/* Resource type not marked as usable. */
		ret = false;
	}
	return ret;
}

void os::agent::TileConstraint::siftPool(AgentClaim &pool) const
{
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (isTileAllowed(res.tileID))
		{
			continue;
		}
		else
		{
			DBG(SUB_AGENT_NOISY, "siftPool: removing tile %d due to TileConstraint\n", res.tileID);
			pool.removeTile(res.tileID);
		}
	}
}

bool os::agent::TileConstraint::fulfilledBy(const AgentClaim &claim, bool *improvable, const bool respect_max) const
{
	os::agent::ResourceID res;

	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (this->isTileAllowed(res.tileID))
		{
			continue;
		}

		if (claim.containsTile(res.tileID))
		{
			return false;
		}
	}

	return true;
}

void os::agent::TileSharingConstraint::siftPool(AgentClaim &pool) const
{
	if (isTileShareable())
	{
		// We share, so we don't care
		return;
	}

	const os::agent::AgentInstance *agent = pool.getOwningAgent();

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!pool.containsTile(res.tileID) || pool.containsAllResourcesOfTile(res.tileID))
		{
			continue;
		}

		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			os::agent::AgentInstance *owner = os::agent::AgentSystem::getOwner(res);
			/*
			 * We don't allow sharing our tiles. All resources of tiles we own must thus be either
			 * - owned by us
			 * - owned by the idle agent
			 * - owned by no one (which, technically, shouldn't happen)
			 */
			if (owner != agent && owner != NULL && owner != os::agent::AgentSystem::idleAgent)
			{
				pool.removeTile(res.tileID);
				break;
			}
		}
	}
}

bool os::agent::TileSharingConstraint::violatedBy(const AgentClaim &claim, const bool claimNotFulfills, const bool malleableContext) const
{
	if (malleableContext)
	{
		/*
	 * in a malleable context, it is possible that we will acquire
	 * further (and maybe all) resources of a tile. We don't know
	 * that yet, so it would be harsh to say it is violating if we
	 * don't have all resources at this point.
	 */
		return false;
	}
	else if (claimNotFulfills)
	{
		return false;
	}
	else
	{
		return !fulfilledBy(claim);
	}
}

bool os::agent::TileSharingConstraint::fulfilledBy(const AgentClaim &claim, bool *improvable,
																									 const bool respect_max) const
{
	if (isTileShareable())
	{
		// We share, so we don't care
		return true;
	}

	const os::agent::AgentInstance *agent = claim.getOwningAgent();

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!claim.containsTile(res.tileID) || claim.containsAllResourcesOfTile(res.tileID))
		{
			continue;
		}

		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{
			os::agent::AgentInstance *owner = os::agent::AgentSystem::getOwner(res);
			/*
			 * We don't allow sharing our tiles. All resources of tiles we own must thus be either
			 * - owned by us
			 * - owned by the idle agent
			 * - owned by no one (which, technically, shouldn't happen)
			 */
			if (owner != agent && owner != NULL && owner != os::agent::AgentSystem::idleAgent)
			{
				return false;
			}
		}
	}

	return true;
}

/*
 * Though the TileSharingConstraint does not explicitly specify resources we need, the
 * situation may arise, where the claim just contains some of the PEs of a tile. If
 * TileSharing is disabled, we need to block the remaining PEs from being used by others.
 */
bool os::agent::TileSharingConstraint::claimResources(const os::agent::AgentClaim &claim,
																											os::agent::AgentInstance *agent, int slot) const
{
	if (!isTileShareable())
	{
		os::agent::ResourceID res;
		for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
		{
			if (!claim.containsTile(res.tileID))
			{
				continue;
			}

			for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
			{
				if (claim.contains(res))
				{
					continue;
				}

				if (os::agent::HardwareMap[res.tileID][res.resourceID].type != none)
				{
					DBG(SUB_AGENT, "Blocking resource %d/%d due to TileShareableConstraint\n", res.tileID, res.resourceID);
					if (!agent->blockResource(res, slot))
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool os::agent::LocalMemoryConstraint::fulfilledBy(const AgentClaim &claim, bool *improvable,
																									 const bool respect_max) const
{

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!claim.containsTile(res.tileID))
		{
			continue;
		}

		uint8_t peOnTile = claim.resourcesInTile(res.tileID);
		int minForTile = peOnTile * min;
		int maxForTile = peOnTile * max;
		(void)minForTile; //supress compiler warnings while the hal
		(void)maxForTile; //is not used to really test the constraint

		//TODO: if (!hasLocalMemory(minForTile)) { return false; }
	}

	return true;
}

bool os::agent::LocalMemoryConstraint::claimResources(const os::agent::AgentClaim &claim,
																											os::agent::AgentInstance *agent, int slot) const
{
	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!claim.containsTile(res.tileID))
		{
			continue;
		}

		uint8_t peOnTile = claim.resourcesInTile(res.tileID);
		int minForTile = peOnTile * min;
		int maxForTile = peOnTile * max;
		(void)minForTile; //supress compiler warnings while the hal
		(void)maxForTile; //is not used to really test the constraint

		//TODO: find a way to reserve memory
	}

	return true;
}

void os::agent::SearchLimitConstraint::setDefault()
{
	this->timeLimit = DEFAULT_TIME_LIMIT;
	this->ratingLimit = DEFAULT_RATING_LIMIT;
}

void os::agent::MalleableRatingConstraint::setDefault()
{
	this->activated = false;
	this->ratingMultiplier = DEFAULT_MALLEABLE_RATING_MULTIPLIER;
	this->claimingAgent = NULL;
}

os::agent::ResourceRating os::agent::MalleableRatingConstraint::rateClaim(const os::agent::AgentClaim &claim, const bool claimFulfills) const
{
	using namespace os::agent;
	ResourceRating rating = 100;
	uint16_t malleableResCounter = 0;
	if (!activated || !claimingAgent)
	{
		return rating;
	}

	os::agent::ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
	{
		if (!claim.containsTile(res.tileID))
		{
			continue;
		}
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; ++res.resourceID)
		{

			if (claim.contains(res))
			{
				AgentInstance *owner = os::agent::AgentSystem::getOwner(res);
				if (owner != NULL && owner != os::agent::AgentSystem::idleAgent && owner != claimingAgent)
				{
					rating *= ratingMultiplier;
					malleableResCounter++;
				}
				// prevent overflow
				if (malleableResCounter >= 7)
				{
					rating /= 100;
					malleableResCounter--;
				}
			}
		}
	}
	// normalize at the end to keep the precision high
	uint64_t divider = 1;
	for (; malleableResCounter > 0; malleableResCounter--)
	{
		divider *= 100;
	}
	rating /= divider;
	return rating;
}

size_t os::agent::AndConstraintList::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::AndConstraintList) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *cType = (os::agent::ConstrType *)buf;
	*cType = this->getCType();
	buf += sizeof(os::agent::ConstrType);

	os::agent::AndConstraintList *c = (os::agent::AndConstraintList *)buf;
	*c = *this;
	buf += sizeof(os::agent::AndConstraintList);

	os::agent::ConstraintList::Iterator current = this->begin();
	os::agent::ConstraintList::Iterator end = this->end();
	for (; current != end; ++current)
	{
		size_t memberSize = (*current)->flatten(buf, (max - needed));
		if (memberSize == 0)
		{
			// which means the last call didn't succeed
			return 0;
		}
		needed += memberSize;
		buf += memberSize;
	}

	return needed;
}

size_t os::agent::PEQuantityConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::PEQuantityConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::PEQuantityConstraint *c = (os::agent::PEQuantityConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::DowneySpeedupConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::DowneySpeedupConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::DowneySpeedupConstraint *c = (os::agent::DowneySpeedupConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::TileConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::TileConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::TileConstraint *c = (os::agent::TileConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::StickyConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::StickyConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::StickyConstraint *c = (os::agent::StickyConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::MalleabilityConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::MalleabilityConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::MalleabilityConstraint *c = (os::agent::MalleabilityConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::AppClassConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::AppClassConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::AppClassConstraint *c = (os::agent::AppClassConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::AppNumberConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::AppNumberConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::AppNumberConstraint *c = (os::agent::AppNumberConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::TileSharingConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::TileSharingConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::TileSharingConstraint *c = (os::agent::TileSharingConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::ViPGConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::ViPGConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::ViPGConstraint *c = (os::agent::ViPGConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::LocalMemoryConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::LocalMemoryConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::LocalMemoryConstraint *c = (os::agent::LocalMemoryConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::ReinvadeHandlerConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::ReinvadeHandlerConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::ReinvadeHandlerConstraint *c =
			(os::agent::ReinvadeHandlerConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::SearchLimitConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::SearchLimitConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::SearchLimitConstraint *c = (os::agent::SearchLimitConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::MalleableRatingConstraint::flatten(char *buf, size_t max) const
{
	size_t needed = sizeof(os::agent::MalleableRatingConstraint) + sizeof(os::agent::ConstrType);
	if (max < needed)
	{
		return 0;
	}

	os::agent::ConstrType *ctype = (os::agent::ConstrType *)buf;
	*ctype = this->getCType();

	os::agent::MalleableRatingConstraint *c = (os::agent::MalleableRatingConstraint *)(buf + sizeof(os::agent::ConstrType));
	*c = *this;

	return needed;
}

size_t os::agent::FlatConstraints::unflattenOutside(AgentConstraint **target, size_t offset) const
{
	using namespace os::agent;

	if ((size_t)this->dataUsed <= offset)
	{
		DBG(SUB_AGENT, "UnflattenOutside returns NULL (end of data reached)\n");
		*target = NULL;
		return 0;
	}

	ConstrType *cType = (ConstrType *)((size_t) & (this->data[0]) + offset);

	size_t consumed = sizeof(ConstrType);
	AgentConstraint *c = NULL;

	switch (*cType)
	{
	case ConstrType::NONE:
		break;
	case ConstrType::AND:
	{
		AndConstraintList *temp = (AndConstraintList *)((size_t) & (this->data[0]) + offset + consumed);
		AndConstraintList *cl;
		consumed += sizeof(AndConstraintList);
		cl = new AndConstraintList(NULL, false);
		c = cl;

		for (uint8_t i = 0; i < temp->getSize(); ++i)
		{
			AgentConstraint *member = NULL;
			consumed += this->unflattenOutside(&member, offset + consumed);
			if (!member)
			{
				*target = NULL;
				return 0;
			}
			cl->addConstraint(member);
			member->parent = cl;
		}
	} //
	break;
	case ConstrType::PEQUANTITY:
	{
		PEQuantityConstraint *temp = (PEQuantityConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new PEQuantityConstraint(*temp);
		consumed += sizeof(PEQuantityConstraint);
	}
	break;
	case ConstrType::DOWNEYSPEEDUP:
	{
		DowneySpeedupConstraint *temp = (DowneySpeedupConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new DowneySpeedupConstraint(*temp);
		consumed += sizeof(DowneySpeedupConstraint);
	}
	break;
	case ConstrType::TILE:
	{
		TileConstraint *temp = (TileConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new TileConstraint(*temp);
		consumed += sizeof(TileConstraint);
	}
	break;
	case ConstrType::STICKY:
	{
		StickyConstraint *temp = (StickyConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new StickyConstraint(*temp);
		consumed += sizeof(StickyConstraint);
	}
	break;
	case ConstrType::MALLEABILITY:
	{
		MalleabilityConstraint *temp = (MalleabilityConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new MalleabilityConstraint(*temp);
		consumed += sizeof(MalleabilityConstraint);
	}
	break;
	case ConstrType::APPCLASS:
	{
		AppClassConstraint *temp = (AppClassConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new AppClassConstraint(*temp);
		consumed += sizeof(AppClassConstraint);
	}
	break;
	case ConstrType::APPNUMBER:
	{
		AppNumberConstraint *temp = (AppNumberConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new AppNumberConstraint(*temp);
		consumed += sizeof(AppNumberConstraint);
	}
	break;
	case ConstrType::TILESHARING:
	{
		TileSharingConstraint *temp = (TileSharingConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new TileSharingConstraint(*temp);
		consumed += sizeof(TileSharingConstraint);
	}
	break;
	case ConstrType::VIPG:
	{
		ViPGConstraint *temp = (ViPGConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new ViPGConstraint(*temp);
		consumed += sizeof(ViPGConstraint);
	}
	break;
	case ConstrType::REINVADE:
	{
		ReinvadeHandlerConstraint *temp = (ReinvadeHandlerConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new ReinvadeHandlerConstraint(*temp);
		consumed += sizeof(ReinvadeHandlerConstraint);
	}
	break;
	case ConstrType::LOCALMEMORY:
	{
		LocalMemoryConstraint *temp = (LocalMemoryConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new LocalMemoryConstraint(*temp);
		consumed += sizeof(LocalMemoryConstraint);
	}
	break;
	case ConstrType::SEARCHLIMIT:
	{
		SearchLimitConstraint *temp = (SearchLimitConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new SearchLimitConstraint(*temp);
		consumed += sizeof(SearchLimitConstraint);
	}
	break;
	case ConstrType::MALLEABLERATING:
	{
		MalleableRatingConstraint *temp = (MalleableRatingConstraint *)((size_t) & (this->data[0]) + offset + consumed);
		c = new MalleableRatingConstraint(*temp);
		consumed += sizeof(MalleableRatingConstraint);
	}
	break;
	}

	*target = c;

	return consumed;
}

uint16_t os::agent::SolverClaim::debugPrint(ResType HWOrder[])
{
	uint16_t resCount = 0;
	for (uint8_t orderNo = 0; orderNo < HWTypes; ++orderNo)
	{
		DBG_RAW(SUB_AGENT, "Resources of type %d:\t", HWOrder[orderNo]);
		for (uint8_t listIdx = 0; listIdx < resList[orderNo].coreCount; ++listIdx)
		{
			os::agent::ResourceID res = resList[orderNo].list[listIdx];
			DBG_RAW(SUB_AGENT, "(%d-%d) ", res.tileID, res.resourceID);
		}
		resCount += resList[orderNo].coreCount;
		DBG_RAW(SUB_AGENT, "\n");
	}
	return resCount;
}

void os::agent::ConstraintSolver::determineMaxRating()
{
	const os::agent::SearchLimitConstraint *slc = static_cast<const os::agent::SearchLimitConstraint *>(
			constraints->searchConstraint(os::agent::ConstrType::SEARCHLIMIT));

	uint8_t multiplier = slc->getRatingLimit();

	maxRating = constraints->determineMaxRating() * multiplier / 100;
	DBG(SUB_AGENT, "Maximum rating for solver set to %" PRIu64 "\n", maxRating);
}

bool os::agent::ConstraintSolver::moveFromPoolToClaim(const os::agent::ResourceID &res)
{
	bool moveSuccessful = moveFromTo(pool, *claim, res);
	if (moveSuccessful)
	{
		myAgent->invade_bargain_transfer_resource(res);
	}
	return moveSuccessful;
}

os::agent::ResourceRating os::agent::ConstraintSolver::backtrack(os::agent::AgentClaim &claim,
																																 os::agent::ResourceIndex &resIdx, os::agent::ResourceRating oldRating, uint8_t depth)
{
	DBG(SUB_AGENT, "Entering backtrack (depth %" PRIu8 ")\n", depth);

	using namespace os::agent;

	ResourceRating bestRating = oldRating;
	bool didBacktrack = false;

	for (; resIdx.orderIdx < HWTypes; ++resIdx.orderIdx)
	{

		ResourceID *list = solverPool->resList[resIdx.orderIdx].list;
		for (; resIdx.listIdx < solverPool->resList[resIdx.orderIdx].coreCount; ++resIdx.listIdx)
		{
			if (stopAlgo)
			{
				return this->claimRating;
			}
			ResourceID res = list[resIdx.listIdx];

			if (!moveFromTo(pool, claim, res))
			{
				continue;
			}
			DBG(SUB_AGENT, "Trying Resource (%d/%d)\n", res.tileID, res.resourceID);
			ResourceRating rating = 0;
			bool fulfilled = constraints->fulfilledBy(claim);
			if (fulfilled)
			{
				rating = constraints->rateClaim(claim, true);
				if (rating > oldRating)
				{
					if (rating > getRating())
					{
						updateClaim(claim, rating);
						if (rating >= maxRating)
						{
							DBG(SUB_AGENT, "Stopping algorithm because current rating of %" PRIu64 " "
														 "is not lower than rating limit of %" PRIu64 ".\n",
									rating, maxRating);
							stopAlgo = true;
							return rating;
						}
					}
					DBG(SUB_AGENT, "backtrack: fulfilled; trying to "
												 "improve current rating of %" PRIu64 "\n",
							rating);
					ResourceIndex nextIdx = resIdx;
					nextIdx.listIdx++;
					rating = backtrack(claim, nextIdx, rating, depth + 1);
					if (rating > bestRating)
					{
						bestRating = rating;
					}
					didBacktrack = true;
				}
				else
				{
					DBG(SUB_AGENT, "backtrack: fulfilled, but this resource didn't improve the rating.\n");
				}
			}
			else if (constraints->violatedBy(claim, true))
			{
				DBG(SUB_AGENT, "backtrack: violated!\n");
				violationAppeared = true;
				// Do not try the next resource of same type when claim is full.
				if (constraints->violatesMaxPE(claim, HWOrder[resIdx.orderIdx]))
				{
					moveFromTo(claim, pool, res);
					DBG(SUB_AGENT, "backtrack: jumping to next resource type as claim is full.\n");
					break;
				}
			}
			else
			{
				DBG(SUB_AGENT, "backtrack: neither fulfilled nor violating. Keep on going\n");
				ResourceIndex nextIdx = resIdx;
				nextIdx.listIdx++;
				rating = backtrack(claim, nextIdx, rating, depth + 1);
				didBacktrack = true;
				if (rating > bestRating)
				{
					bestRating = rating;
				}
			}

			moveFromTo(claim, pool, res);
		}
		resIdx.listIdx = 0;
	}

	if (!didBacktrack)
	{
		// we're at the end of a backtracking-path

		if (malleability)
		{
			// Continue backtracking with the malleability pools.
			ResourceID nextRes;
			nextRes.tileID = 0;
			nextRes.resourceID = 0;
			ResourceRating rating = backtrackMalleableResources(claim, nextRes, bestRating, depth + 1);
			if (rating > bestRating)
			{
				bestRating = rating;
			}
			// As no violation appeared, all resources of pool had been used.
			// From now on the solver would check subsets of this claim. A better rating would be unlikely.
		}
		else if (!violationAppeared && constraints->rateClaim(claim) > 0)
		{
			DBG(SUB_AGENT, "Stopping algorithm because all resources of pool had been used.\n");
			stopAlgo = true;
		}

		uint64_t duration = uint64_t(os::dev::WallClock::Inst().milliseconds()) - startTime;
		if (duration > timeLimit)
		{
			DBG(SUB_AGENT, "Stopping solving because duration (%" PRIu64 " milliseconds) exceeds time limit (%" PRIu64 " milliseconds)\n", duration, startTime);
			stopAlgo = true;
		}
	}

	DBG(SUB_AGENT, "Leaving backtrack (depth %" PRIu8 ")\n", depth);
	return bestRating;
}

/*
 * basically, this works exactly like the backtracking algorithm without malleable resources.
 * It just uses the malleabilityPools instead of the pool of free resources.
 * However, some adjustments are done, and marked with comments.
 */
os::agent::ResourceRating os::agent::ConstraintSolver::backtrackMalleableResources(os::agent::AgentClaim &claim,
																																									 os::agent::ResourceID &res, os::agent::ResourceRating oldRating, uint8_t depth)
{
	DBG(SUB_AGENT, "Entering backtrackMalleableResources (depth %" PRIu8 ")\n", depth);
	using namespace os::agent;

	ResourceRating bestRating = oldRating;

	for (; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++, res.resourceID = 0)
	{
		bool fulfilled = false;

		for (; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
		{
			if (this->claim && this->claim->contains(res))
			{
				continue;
			}
			if (claim.contains(res))
			{
				continue;
			}

			AgentMalleabilityClaim *partOfPool = NULL;
			for (uint8_t i = 0; i < numMalleabilityPools; ++i)
			{
				if (malleabilityPools[i]->contains(res))
				{
					partOfPool = malleabilityPools[i];
					break;
				}
			}
			if (partOfPool == NULL)
			{
				continue;
			}

			/*
			 * here, it actually gets interesting. in the backtracking function without malleability,
			 * moveFromTo should not fail. But here, it might fail, if we already used all resources
			 * from the malleabilityPool that the respective application allows us.
			 */
			if (!moveFromTo(*partOfPool, claim, res))
			{
				continue;
			}
			DBG(SUB_AGENT, "Trying Resource (%d/%d)\n", res.tileID, res.resourceID);

			fulfilled = constraints->fulfilledBy(claim);
			if (fulfilled)
			{
				/*
				 * Our current policy is to just take the first working claim we find, as to
				 * minimize our impact on other applications.
				 */
				bestRating = constraints->rateClaim(claim, true);
				updateClaim(claim, bestRating);
				//				stopAlgo = true; // can be used when the solving process should completely be stopped.
				malleability = false; // can be used to try improving the claim rating with free resources
				break;
			}
			else if (constraints->violatedBy(claim, true, true))
			{
				DBG(SUB_AGENT, "backtrackMalleability: violated!\n");
			}
			else
			{
				DBG(SUB_AGENT, "backtrackMalleability: neither fulfilled nor violating. Keep on going\n");
				ResourceID nextRes = res;
				nextRes.resourceID++;
				ResourceRating rating = backtrackMalleableResources(claim, nextRes, bestRating, depth + 1);

				if (rating > 0)
				{
					/*
					 * in this case, we found a fulfilling claim somewhere down the line.
					 * We thus break up the backtracking.
					 */
					bestRating = rating;
					fulfilled = true;
					break;
				}
			}

			claim.remove(res);
			partOfPool->refund(res);
		}

		if (fulfilled)
		{
			break;
		}
	}

	DBG(SUB_AGENT, "Leaving backtrackMalleability (depth %" PRIu8 ")\n", depth);
	return bestRating;
}

os::agent::ResourceRating os::agent::ConstraintSolver::solve()
{
	state = SOLVING;

	/*
     * idea of this whole mess:
     * The solver is given the Constraints and a resource pool. If no claim is provided,
     * the solver starts from scratch, that is, with an empty claim.
     *
     * Step 1: sift the pool
     * - all resources in the pool that "obviously" don't serve a purpose, are filtered out,
     *   for example, resources on tiles that are not allowed
     *
     * Step 2: Call the backtracking algorithm
     * - this tries to resolve the Constraints with the resources given to us.
     * - if this succeeds, go to Step 5
     * - otherwise, continue
     *
     * Step 3: Collect malleable resources
     * - calls the AgentSystem to collect all possible malleable resources
     *
     * Step 4: Call the backtracking algorithm with malleability
     * - this tries to resolve the Constraints with the resources given to us and malleable candidates.
     *
     * Step 5: Cleanup
     * - release allocated resources (such as the malleabilityPools)
     *
     * Step 6: Return
     * - return with the Rating we have for our Claim
     */

	constraints->siftPool(getPool());

	solverPool = new SolverClaim(getPool(), HWOrder, &(myAgent->myResourcePool));
	DBG(SUB_AGENT, "Solver will use pool with %" PRIu16 " resources\n", solverPool->debugPrint(HWOrder));

	os::agent::AgentClaim tempClaim = *claim; // get sticky resources

	tempClaim.setOwningAgent(myAgent);
	os::agent::ResourceIndex resIdx;
	resIdx.orderIdx = 0;
	resIdx.listIdx = 0;
	stopAlgo = false;
	malleability = false;
	violationAppeared = false;
	startTime = uint64_t(os::dev::WallClock::Inst().milliseconds());
	const os::agent::SearchLimitConstraint *slc = static_cast<const os::agent::SearchLimitConstraint *>(
			constraints->searchConstraint(os::agent::ConstrType::SEARCHLIMIT));
	timeLimit = slc->getTimeLimit();
	backtrack(tempClaim, resIdx, this->claimRating);

	if (this->claimRating > 0)
	{
		state = SOLVED;
		return this->claimRating;
	}

	DBG(SUB_AGENT, "Didn't find a solution in phase 1. Calling other agents\n");
	os::agent::AgentSystem::dumpAgents();
	numMalleabilityPools = os::agent::AgentSystem::collectMalleabilityClaims(&malleabilityPools, myAgent, slot);
	DBG(SUB_AGENT, "We got %" PRIu8 " MalleabilityPools at adress %p\n", numMalleabilityPools, malleabilityPools);
	resIdx.orderIdx = 0;
	resIdx.listIdx = 0;
	malleability = true;
	if (numMalleabilityPools > 0)
	{
		tempClaim.reset();
		stopAlgo = false;
		backtrack(tempClaim, resIdx, this->claimRating);
	}

	if (this->claimRating > 0)
	{
		state = MOLDING;
	}
	else
	{
		cleanUpMalleabilityPools();
		state = FAILED;
		os::agent::AgentInstance::debugPrintResourceDistribution();
	}
	delete solverPool;
	return this->claimRating;
}

bool os::agent::AgentConstraint::isSticky() const
{
	const os::agent::StickyConstraint *sc = static_cast<const os::agent::StickyConstraint *>(
			searchConstraint(os::agent::ConstrType::STICKY));

	// default is 'true'
	return (!sc || sc->isSticky());
}

bool os::agent::AgentConstraint::isMalleable() const
{
	const os::agent::MalleabilityConstraint *mc = static_cast<const os::agent::MalleabilityConstraint *>(
			searchConstraint(os::agent::ConstrType::MALLEABILITY));

	// default is 'false'
	return (mc && mc->isMalleable());
}

resize_handler_t os::agent::AgentConstraint::getResizeHandler() const
{
	const os::agent::MalleabilityConstraint *mc = static_cast<const os::agent::MalleabilityConstraint *>(
			searchConstraint(os::agent::ConstrType::MALLEABILITY));

	if (!mc)
	{
		return NULL;
	}
	else
	{
		return mc->getResizeHandler();
	}
}

resize_env_t os::agent::AgentConstraint::getResizeEnvPointer() const
{
	const os::agent::MalleabilityConstraint *mc = static_cast<const os::agent::MalleabilityConstraint *>(
			searchConstraint(os::agent::ConstrType::MALLEABILITY));

	if (!mc)
	{
		return NULL;
	}
	else
	{
		return mc->getResizeEnvPointer();
	}
}

bool os::agent::AgentConstraint::isTileShareable() const
{
	const os::agent::TileSharingConstraint *tsc = static_cast<const os::agent::TileSharingConstraint *>(
			searchConstraint(os::agent::ConstrType::TILESHARING));

	// default is 'false'
	return (tsc && tsc->isTileShareable());
}

bool os::agent::AgentConstraint::isViPGBlocking() const
{
	const os::agent::ViPGConstraint *vipgc = static_cast<const os::agent::ViPGConstraint *>(
			searchConstraint(os::agent::ConstrType::VIPG));

	// default is 'true'
	return (vipgc && vipgc->isViPGEnabled());
}

bool os::agent::AgentConstraint::isTileAllowed(const os::agent::TileID tileID) const
{
	const os::agent::TileConstraint *tc = static_cast<const os::agent::TileConstraint *>(
			searchConstraint(os::agent::ConstrType::TILE));

	// default is that all tiles are allowed
	return (!tc || tc->isTileAllowed(tileID));
}

int os::agent::AgentConstraint::getAppClass() const
{
	const os::agent::AppClassConstraint *acc = static_cast<const os::agent::AppClassConstraint *>(
			searchConstraint(os::agent::ConstrType::APPCLASS));

	if (acc)
		return acc->getAppClass();
	else
		return 0;
}

int os::agent::AgentConstraint::getAppNumber() const
{
	const os::agent::AppNumberConstraint *anc = static_cast<const os::agent::AppNumberConstraint *>(
			searchConstraint(os::agent::ConstrType::APPNUMBER));

	if (anc)
		return anc->getAppNumber();
	else
		return 23;
}

uint16_t os::agent::AgentConstraint::getDowneyA() const
{
	const os::agent::DowneySpeedupConstraint *dsc = static_cast<const os::agent::DowneySpeedupConstraint *>(
			searchConstraint(os::agent::ConstrType::DOWNEYSPEEDUP));
	if (dsc)
		return dsc->getA();
	else
		return os::agent::DowneySpeedupConstraint::defaultA;
}

uint16_t os::agent::AgentConstraint::getDowneySigma() const
{
	const os::agent::DowneySpeedupConstraint *dsc = static_cast<const os::agent::DowneySpeedupConstraint *>(
			searchConstraint(os::agent::ConstrType::DOWNEYSPEEDUP));
	if (dsc)
		return dsc->getSigma();
	else
		return os::agent::DowneySpeedupConstraint::defaultSigma;
}

os::agent::ResourceRating os::agent::AgentConstraint::downey_rate(uint16_t n, uint16_t A, uint16_t s) const
{
	/* This function takes the function of same name in Agent.cc as basis. Some corrections were made.
	 * Compared to the original Downey formula, all numbers are scaled to 100 to avoid floating point values.
	 */
	/*
	 * Use these wide variants when calculating stuff to not run into
	 * overflowing issues.
	 */
	uint64_t n_wide = static_cast<uint64_t>(n);
	uint64_t A_wide = static_cast<uint64_t>(A);
	uint64_t s_wide = static_cast<uint64_t>(s);

	n_wide *= 100;

	// low variance model
	if (s < 100)
	{
		if (n_wide <= 100)
		{
			return n_wide;
		}
		else if (n_wide <= A)
		{
			return 100 * (A_wide * n_wide) / (100 * A_wide + (n_wide - 100) * s_wide / (2));
		}
		else if (n_wide < A * 2)
		{
			return 100 * A_wide * n_wide / (s_wide * A_wide + 100 * n_wide - (s_wide * (n_wide + 100) / 2));
		}
		else
		{
			kassert(n_wide >= A_wide * 2);
			return A_wide;
		}

		// high variance model
	}
	else
	{
		kassert(s >= 100);
		uint64_t cutoff = A_wide + A_wide * s_wide / 100 - s_wide;
		if (n_wide < cutoff)
		{
			return n_wide * A_wide * (s_wide + 100) / (s_wide * (n_wide + A_wide - 100) + 100 * A_wide);
		}
		else
		{
			return A_wide;
		}
	}
}

bool os::agent::AgentConstraint::isResourceAllowed(const os::agent::ResourceID &res) const
{
	typedef os::agent::AgentSystem AS;

	const os::agent::PEQuantityConstraint *peqc = static_cast<const os::agent::PEQuantityConstraint *>(
			searchConstraint(os::agent::ConstrType::PEQUANTITY));

	// default is, that all resources are allowed without limit
	if (!peqc)
	{
		return true;
	}

	kassert(peqc->getMin(AS::getHWType(res)) <= peqc->getMax(AS::getHWType(res)));

	return (peqc->isResourceUsable(res));
}

bool os::agent::AgentConstraint::canLoseResource(const os::agent::AgentClaim &claim,
																								 const os::agent::ResourceID &res) const
{
	typedef os::agent::AgentSystem AS;

	if (!claim.contains(res))
	{
		return true;
	}

	const os::agent::PEQuantityConstraint *peqc = static_cast<const os::agent::PEQuantityConstraint *>(
			searchConstraint(os::agent::ConstrType::PEQUANTITY));

	if (!peqc)
	{
		// default is, that no restrictions apply (this can mean that a claim is stripped its only resource)
		return true;
	}
	else
	{
		return (peqc->getMin(AS::getHWType(res)) < claim.getQuantity(os::agent::NOTILE, AS::getHWType(res)));
	}
}

int os::agent::AgentConstraint::getMinOfType(const os::agent::HWType type) const
{
	const os::agent::PEQuantityConstraint *peqc = static_cast<const os::agent::PEQuantityConstraint *>(
			searchConstraint(os::agent::ConstrType::PEQUANTITY));

	if (!peqc)
	{
		return 0; //default
	}
	else
	{
		return peqc->getMin(type);
	}
}

int os::agent::AgentConstraint::getMaxOfType(const os::agent::HWType type) const
{
	const os::agent::PEQuantityConstraint *peqc = static_cast<const os::agent::PEQuantityConstraint *>(
			searchConstraint(os::agent::ConstrType::PEQUANTITY));

	if (!peqc)
	{
		return 0; //default
	}
	else
	{
		return peqc->getMax(type);
	}
}

bool os::agent::AgentConstraint::violatesMaxPE(const os::agent::AgentClaim &claim, const os::agent::HWType type) const
{
	using namespace os::agent;
	const PEQuantityConstraint *peqc = static_cast<const PEQuantityConstraint *>(searchConstraint(ConstrType::PEQUANTITY));
	if (!peqc)
	{
		panic("Couldn't find PEQuantityConstraint.\n");
	}

	if (claim.getTypeCount(type) > peqc->getMax(type))
	{
		return true;
	}
	else
	{
		return false;
	}
}

os::agent::ResourceRating os::agent::AgentConstraint::rateAdditionalResource(os::agent::AgentClaim &claim,
																																						 const os::agent::ResourceID &res) const
{
	typedef os::agent::AgentSystem AS;

	// first check if the new resource would conflict with any constraints
	if (!isTileAllowed(res.tileID) || !isResourceAllowed(res))
	{
		return 0;
	}

	/* Given claim already contains the resource. */
	if (claim.contains(res))
	{
		return 0;
	}

	const os::agent::PEQuantityConstraint *peqc = static_cast<const os::agent::PEQuantityConstraint *>(
			searchConstraint(os::agent::ConstrType::PEQUANTITY));

	if (!peqc)
	{
		DBG(SUB_AGENT_NOISY, "Can't rate additional resource, no PEQuantityConstraint found.\n");
		return 0;
	}

	/* Resource not useful for claim. */
	if (peqc->getMax(AS::getHWType(res)) <= claim.getQuantity(os::agent::NOTILE, AS::getHWType(res)))
	{
		DBG(SUB_AGENT_NOISY, "Maximum number of resources for type already reached: Rating for additional resource is 0\n");
		return (0);
	}

	// TODO: Real rating function + Monitoring stuff..

	os::agent::ResourceRating r_without = rateClaim(claim);
	claim.add(res);
	os::agent::ResourceRating r_with = rateClaim(claim);
	claim.remove(res);

	return (r_with - r_without); // seems usable ;)
}

os::agent::ResourceRating os::agent::AgentConstraint::rateLosingResource(os::agent::AgentClaim &claim,
																																				 const os::agent::ResourceID &res) const
{
	typedef os::agent::AgentSystem AS;

	/* Cannot lose a not-owned resource. */
	if (!claim.contains(res))
	{
		return (0);
	}

	/*
     * It is not particularaly efficient to search for this Constraint here, as we need it only for the
     * Debug output later on :/
     */
	const os::agent::PEQuantityConstraint *peqc = static_cast<const os::agent::PEQuantityConstraint *>(
			searchConstraint(os::agent::ConstrType::PEQUANTITY));

	if (!peqc)
	{
		return 0;
	}

	if (peqc->getMin(AS::getHWType(res)) >= claim.getQuantity(os::agent::NOTILE, AS::getHWType(res)))
	{
		DBG(SUB_AGENT, "WARNING: rateLosingResource: min constraint of %p for resource type %" PRIu8 " barely or not satisfied, yet something checks for losing such a resource!\n", this, AS::getHWType(res));
	}

	// TODO: Real rating function + Monitoring stuff..

	os::agent::ResourceRating r_with = rateClaim(claim);
	claim.remove(res);
	os::agent::ResourceRating r_without = rateClaim(claim);
	claim.add(res);

	return (r_with - r_without);
}
