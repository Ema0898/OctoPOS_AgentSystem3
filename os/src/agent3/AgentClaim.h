#ifndef OS_AGENT_CLAIM_H
#define OS_AGENT_CLAIM_H

#include "os/agent3/AgentRPCHeader.h"
#include "lib/adt/AtomicID.h"

#include "hw/hal/Tile.h"

#include "os/rpc/RPCStub.h"

#include "lib/adt/AtomicID.h"
#include "lib/debug.h"

#include "os/res/ProxyClaim.h"
#include "os/res/DispatchClaim.h"

#include "octo_types.h"

#include "os/agent3/Platform.h"

#include "os/agent3/AbstractAgentOctoClaim.h"

namespace os
{
	namespace agent
	{

		//class AgentMalleabilityClaim;
		//class ActorConstraint;
		//class AgentConstraint;

		class AgentClaim
		{

		public:
			static const uint32_t INVALID_CLAIM = 0xFFFFFF00;

			uint8_t operatingPointIndex = 100;

			AgentClaim(bool newucid = true)
			{
				ucid = INVALID_CLAIM;
				reset(newucid);

				unsetAgentOctoClaim();

				for (uint8_t type = 0; type < HWTypes; ++type)
				{
					peCount[type] = 0;
				}
			}

			AgentClaim(uintptr_t resources[hw::hal::Tile::MAX_TILE_COUNT], bool newucid = true)
					: AgentClaim(newucid)
			{
				for (uint32_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; ++i)
				{
					this->resources[i] = resources[i];
					ResourceID res;
					if (resources[i] != 0)
					{
						res.tileID = i;
						for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
						{
							if (TEST_BIT(resources[res.tileID], res.resourceID))
							{
								peCount[HardwareMap[res.tileID][res.resourceID].type]++;
							}
						}
					}
				}
			}

			void reset(bool newucid = true);
			bool contains(const ResourceID &resource, const bool print_val = false) const;
			bool containsTile(os::agent::TileID tileID) const;
			void add(const ResourceID &resource);
			void addClaim(const AgentClaim &otherClaim);
			void removeClaim(const AgentClaim &other);
			void remove(const ResourceID &resource);
			void removeTile(const os::agent::TileID &tileID);
			void intersectClaim(const AgentClaim &other);

			uint8_t getTypeCount(int type) const
			{
				return peCount[type];
			}

			void resetTypeCounter()
			{
				for (uint8_t type = 0; type < HWTypes; ++type)
				{
					peCount[type] = 0;
				}
				ResourceID res;
				for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
				{
					for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
					{
						if (TEST_BIT(resources[res.tileID], res.resourceID))
						{
							peCount[HardwareMap[res.tileID][res.resourceID].type]++;
						}
					}
				}
			}

			AgentClaim buildComplement(uintptr_t resources[hw::hal::Tile::MAX_TILE_COUNT])
			{
				AgentClaim claim = *this;
				for (uint8_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; ++i)
				{
					// AND both flags to clear those that aren't in the basic claim
					uintptr_t mask = claim.resources[i] & resources[i];
					// XOR basic flags with mask, to clear flags
					claim.resources[i] ^= mask;

					// no we correct the tileCount
					if (!claim.resources[i] && this->resources[i])
					{
						claim.tileCount--;
					}
				}
				claim.resetTypeCounter();
				return claim;
			}

			bool isEmpty() const
			{
				for (TileID tile = 0; tile < hw::hal::Tile::getTileCount(); tile++)
				{
					if (resources[tile])
						return false;
				}
				return true;
			}

			uint8_t resourcesInTile(TileID tileID) const;
			bool containsAllResourcesOfTile(TileID tileID) const;
			uint8_t getResourceCount(int *PECount = NULL, int *TileCount = NULL, ResType type = TYPE_ALL) const;
			uint8_t getOperatingPointIndex() const;
			void setOperatingPointIndex(uint8_t index);
			uint8_t getDifference(AgentClaim *otherClaim) const;

			uintptr_t getTileFlags(TileID tileID) const
			{
				return resources[tileID];
			}

			void moveAllResourcesToOtherClaim(AgentClaim &otherClaim);

			uint8_t getQuantity(TileID tileID, HWType type) const
			{ // Hackaround ;-)
				uint8_t quantity = 0;
				ResourceID res;

				TileID start_tile = 0;
				TileID stop_tile = hw::hal::Tile::getTileCount() - 1;

				if (tileID != NOTILE)
				{
					start_tile = tileID;
					stop_tile = tileID;
				}

				for (res.tileID = start_tile; res.tileID <= stop_tile; res.tileID++)
				{
					for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
					{
						if ((os::agent::HardwareMap[res.tileID][res.resourceID].type == type || (type == TYPE_ALL && os::agent::HardwareMap[res.tileID][res.resourceID].type != none)) && contains(res))
						{
							quantity++;
						}
					}
				}

				return quantity;
			}

			/**
	 * \brief Returns a map containing all resources of a given type on a given
	 *        tile which are associated with this AgentClaim.
	 */
			uintptr_t getResourceMap(TileID tileID, int type) const
			{
				uintptr_t map = 0x00;
				for (uint32_t i = 0; i < MAX_RES_PER_TILE; ++i)
				{
					if (HardwareMap[tileID][i].type == type)
					{
						map |= (1UL << i);
					}
				}
				return map & resources[tileID];
			}

			uint8_t getTileCount() const
			{
				uint8_t retVal = 0;
				for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
				{
					if (hasTile(tileID))
						retVal++;
				}
				return retVal;
			}

			bool hasTile(TileID tile) const
			{
				return resources[tile] != 0;
			}

			uintptr_t getTileFlags(uint32_t tileID) const
			{
				return resources[tileID];
			}

			void print() const
			{
				ResourceID res;
				DBG(SUB_AGENT, "===CLAIM==[%4" PRIu32 "]==================== (%" PRIuPTR ")\n", ucid, sizeof(AgentClaim));
				for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
				{
					/* This debug statement merely prints the "prelude" for every new line. */
					DBG(SUB_AGENT, "");
					for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
					{
						if (os::agent::HardwareMap[res.tileID][res.resourceID].type == none)
							continue;
						if (!contains(res))
							continue;
						DBG_RAW(SUB_AGENT, "  %d/%d(%d)", res.tileID, res.resourceID, os::agent::HardwareMap[res.tileID][res.resourceID].type);
					}
					DBG_RAW(SUB_AGENT, "\n");
				}
			}

			void *operator new(size_t s) throw();
			void operator delete(void *p);

			uint32_t getUcid() const { return ucid; }
			AgentInstance *getOwningAgent() const { return owningAgent; }
			void setOwningAgent(AgentInstance *ag) { owningAgent = ag; }

			AgentOctoClaim *getAgentOctoClaim() const
			{
				return (myAgentOctoClaim);
			}

			void setAgentOctoClaim(AgentOctoClaim &octo_claim)
			{
				myAgentOctoClaim = &octo_claim;
			}

			void unsetAgentOctoClaim()
			{
				myAgentOctoClaim = NULL;
			}

			/**
     * \brief creates an AgentMalleabilityClaim based on this claim.
     *
     * The created MalleabilityClaim is stored at the provided location.
     * If the given pointer is NULL, the necessary space will be allocated.
     */
			void createMalleabilityClaim(AgentMalleabilityClaim **claim, int slot, uint8_t numAbdicableResources[HWTypes]);

		private:
			AgentOctoClaim *myAgentOctoClaim;

			uint32_t ucid; // Unique Claim ID
			AgentInstance *owningAgent;
			uint8_t coreCount;
			uint8_t tileCount;
			uint8_t peCount[HWTypes];
			uintptr_t resources[hw::hal::Tile::MAX_TILE_COUNT];

			static lib::adt::AtomicID ucidCreator;
			static uint32_t getNewUCID()
			{
				return ucidCreator.getNextID();
			}
		};

		class AgentMalleabilityClaim
		{

		public:
			AgentMalleabilityClaim(AgentClaim *claim, int slot, uintptr_t resources[hw::hal::Tile::MAX_TILE_COUNT],
														 uint8_t numAbdicableResources[HWTypes])
					: claim(claim), agent(claim->getOwningAgent()), slot(slot)
			{
				for (uint32_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; ++i)
				{
					this->resources[i] = resources[i];
				}
				for (int i = 0; i < HWTypes; ++i)
				{
					this->numAbdicableResources[i] = numAbdicableResources[i];
				}
			}

			AgentMalleabilityClaim(const AgentMalleabilityClaim &other)
					: claim(other.claim), agent(other.agent), slot(other.slot)
			{
				for (uint32_t i = 0; i < hw::hal::Tile::MAX_TILE_COUNT; ++i)
				{
					this->resources[i] = other.resources[i];
				}
				for (int i = 0; i < HWTypes; ++i)
				{
					this->numAbdicableResources[i] = other.numAbdicableResources[i];
				}
			}

			void *operator new(size_t s) throw();
			void operator delete(void *p);
			AgentMalleabilityClaim &operator=(const AgentMalleabilityClaim &rhs) = default;

			bool isEmpty() const
			{
				for (TileID tileID = 0; tileID < hw::hal::Tile::MAX_TILE_COUNT; tileID++)
				{
					if (resources[tileID])
					{
						return false;
					}
				}
				return true;
			}

			bool contains(const ResourceID &res) const
			{
				return TEST_BIT(resources[res.tileID], res.resourceID);
			}

			void refund(const ResourceID &res)
			{
				if (!claim->contains(res))
				{
					panic("trying to add a resource to an AgentMalleabilityClaim, which was not initially in the claim");
				}

				add(res);
				numAbdicableResources[HardwareMap[res.tileID][res.resourceID].type]++;
			}

			bool take(const ResourceID &res)
			{
				if (numAbdicableResources[HardwareMap[res.tileID][res.resourceID].type] == 0)
				{
					return false;
				}
				strip(res);
				numAbdicableResources[HardwareMap[res.tileID][res.resourceID].type]--;
				return true;
			}

			/*
     * leaves the intersection of this claim and the provided other claim
     */
			void intersect(const AgentClaim &other);

			/*
     * Removes a resource without reducing the type's number of abdicable resources.
     */
			void strip(const ResourceID &res)
			{
				CLEAR_BIT(resources[res.tileID], res.resourceID);
			}

			/*
     * Adds a resource without increasing the type's number of abdicable resources.
     * This can be used as an inverse function of strip();
     */
			void add(const ResourceID &res)
			{
				SET_BIT(resources[res.tileID], res.resourceID);
			}

			int getSlot() const
			{
				return slot;
			}

			ResourceRating costOfLosingResource(const ResourceID &res);

			AgentClaim buildLossClaim()
			{
				AgentClaim claim = AgentClaim(resources);
				return claim;
			}

			AgentClaim buildRemainClaim()
			{
				return claim->buildComplement(resources);
			}

			AgentClaim *getAgentClaim()
			{
				return claim;
			}

			AgentInstance *getAgent()
			{
				return agent;
			}

		private:
			AgentClaim *claim;
			AgentInstance *agent;

			uintptr_t resources[hw::hal::Tile::MAX_TILE_COUNT];
			uint8_t numAbdicableResources[HWTypes];

			int slot;
		};

		class AgentOctoClaim : public AbstractAgentOctoClaim
		{ // It would be nice to just inherit from AgentClaim.. But let's keep it like this, who knows how the allocator deals with that
		public:
			/**
	 * \brief Returns NULL.
	 *
	 * Allows for distinguishing between a ProxyAgentOctoClaim and an AgentOctoClaim in the C interface, needed for downcasting, because iRTSS currently does not implement dynamic casting.
	 * Returns NULL, because this AgentOctoClaim is not a ProxyAgentOctoClaim (and therefore cannot be returned as one).
	 *
	 * \return NULL.
	 */
			virtual ProxyAgentOctoClaim *asPAOC() { return NULL; }

			/**
	 * \brief Returns a non-NULL pointer to this AgentOctoClaim instance.
	 *
	 * Returns a non-NULL pointer to this AgentOctoClaim instance. Allows for distinguishing between a ProxyAgentOctoClaim and an AgentOctoClaim to enable downcasting from an AbstractAgentOctoClaim in the C interface. Function is needed because iRTSS currently does not implement dynamic casting. Returns a non-NULL pointer, because this AgentOctoClaim is a AgentOctoClaim (and therefore can naturally be returned as one).
	 *
	 * \return Non-NULL pointer to this AgentOctoClaim instance.
	 */
			virtual AgentOctoClaim *asAOC() { return this; }

			AgentOctoClaim(AgentClaim &claim, AgentConstraint *constraints, ActorConstraint *actorConstraints, bool initial = true)
			{

				DBG(SUB_AGENT, "New AgentOctoClaim %p, AgentClaim %p, Constraints %p, initial %d, Agent %p\n", this, &claim, constraints, initial, claim.getOwningAgent());

				myAgentClaim = &claim;
				claim.setAgentOctoClaim(*this);

				adapting = false;
				if ((constraints != NULL) && (actorConstraints == NULL))
				{
					this->constraints = constraints;
				}
				if ((constraints == NULL) && (actorConstraints != NULL))
				{
					this->actorConstraints = actorConstraints;
				}
				for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
				{
					for (int type = 0; type < HWTypes; type++)
					{
						proxyClaims[tileID][type] = NULL;
					}
				}
				if (!initial)
				{ // wrap the initial OctoPOS-Claim in this AgentOctoClaim
					proxyClaims[0][0] = new os::res::ProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim());
				}
				else
				{
					invadeAgentClaim_prepare();
				}
			}

			~AgentOctoClaim()
			{
				DBG(SUB_AGENT, "Delete AgentOctoClaim %p\n", this);
				for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
				{
					for (int type = 0; type < HWTypes; type++)
					{
						if (proxyClaims[tileID][type])
						{
							proxyClaims[tileID][type]->retreat();
						}
					}
				}
			}

			int adaptToAgentClaim_prepare(AgentClaim &newClaim);
			int adaptToAgentClaim_finish(AgentClaim &newClaim, int numberCoresGained);

			void invadeAgentClaim_prepare();
			void invadeAgentClaim_finish();

			void retreat();

			void print() const;

			void *operator new(size_t s) throw();
			void operator delete(void *p);

			TileID getTileID(uint8_t iterator) const
			{ // this function doesn't make too much sense as it is..

				uint8_t count = 0;
				for (TileID tileID = 0; tileID < hw::hal::Tile::getTileCount(); tileID++)
				{
					if (myAgentClaim->hasTile(tileID))
					{
						if (count == iterator)
						{
							return tileID;
						}
						count++;
					}
				}
				return 0xFF;
			}

			os::res::ProxyClaim *getProxyClaim(TileID tileID, HWType type) const
			{
				return proxyClaims[tileID][type];
			}

			// gna.. Schon mal was von Vererbung gehört, herr Kobbe?
			uint32_t getUcid() const { return myAgentClaim->getUcid(); }
			AgentInstance *getOwningAgent() const { return myAgentClaim->getOwningAgent(); }
			void setOwningAgent(AgentInstance *ag) { myAgentClaim->setOwningAgent(ag); }
			bool isEmpty() const { return myAgentClaim->isEmpty(); }
			uint8_t getResourceCount() const { return myAgentClaim->getResourceCount(); }
			uint8_t getOperatingPointIndex() const { return myAgentClaim->getOperatingPointIndex(); }
			uint8_t getQuantity(TileID tileID, HWType type) const { return myAgentClaim->getQuantity(tileID, type); }
			AgentClaim *getAgentClaim() const { return myAgentClaim; }
			AgentConstraint *getConstraints() const { return constraints; }
			void setConstraints(AgentConstraint *constr) { this->constraints = constr; }
			uint8_t getTileCount() const { return myAgentClaim->getTileCount(); }

		private:
			void retreatFromTile(TID tileID,
													 os::res::ProxyClaim::Future retfutures[],
													 int &futurecounter);

			AgentClaim *myAgentClaim;
			AgentConstraint *constraints;
			ActorConstraint *actorConstraints;

			os::res::ProxyClaim::InvFuture futures[hw::hal::Tile::MAX_TILE_COUNT * HWTypes];
			os::res::ProxyClaim *proxyClaims[hw::hal::Tile::MAX_TILE_COUNT][os::agent::HWTypes];

			// Required for "adapt to claim"
			uint8_t is_count[hw::hal::Tile::MAX_TILE_COUNT][os::agent::HWTypes];
			uint8_t should_count[hw::hal::Tile::MAX_TILE_COUNT][os::agent::HWTypes];
			bool adapting;
		};

	} // namespace agent
} // namespace os

// =========== Agent Claim ===========

inline void os::agent::AgentClaim::add(const os::agent::ResourceID &resource)
{
	//TODO: Lock

	if (!contains(resource))
	{
		coreCount++;
		if (resources[resource.tileID] == 0)
		{
			tileCount++;
		}
		SET_BIT(resources[resource.tileID], resource.resourceID);
		peCount[HardwareMap[resource.tileID][resource.resourceID].type]++;
	}
	else
	{
		panic("os::agent::AgentClaim::add: Agent System Inconsistent\n");
	}

	//TODO: Unlock
}

inline void os::agent::AgentClaim::remove(const os::agent::ResourceID &resource)
{
	//TODO: Lock
	if (contains(resource))
	{
		coreCount--;
		CLEAR_BIT(resources[resource.tileID], resource.resourceID);
		if (resources[resource.tileID] == 0)
		{
			tileCount--;
		}
		peCount[HardwareMap[resource.tileID][resource.resourceID].type]--;
	}
	else
	{
		panic("os::agent::AgentClaim::remove: Agent System Inconsistent\n");
	}
	//TODO: Unlock
}

inline void os::agent::AgentClaim::removeTile(const os::agent::TileID &tileID)
{
	//TODO: Lock
	if (containsTile(tileID))
	{
		ResourceID res;
		for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
		{
			if (TEST_BIT(resources[tileID], res.resourceID))
			{
				peCount[HardwareMap[tileID][res.resourceID].type]--;
				coreCount--;
			}
		}
		resources[tileID] = 0;
		tileCount--;
	}
	//TODO: Unlock
}

inline bool os::agent::AgentClaim::contains(const os::agent::ResourceID &resource, const bool print_val) const
{
	/*
	 * This is really just a hack to get a nice representation of the bitset that "resources" is.
	 * It relies on "a bit" of system-internal knowledge and can and will easily break as soon as
	 * the internal data layout or other properties change.
	 *
	 * It's seriously best to not use this.
	 * At all.
	 * Never.
	 */
	if (print_val)
	{
		/* resources is a uintptr_t array. Change type_size if the definition changes. */
		const size_t type_size = sizeof(resources[0]) * 8;

		char bitfield_str[type_size + 1] = {};
		memset(bitfield_str, '0', type_size);

		for (size_t cur_bit = type_size; cur_bit > 0; --cur_bit)
		{
			if (resources[resource.tileID] & (1 << (type_size - cur_bit)))
			{
				bitfield_str[cur_bit - 1] = '1';
			}
		}
		DBG(SUB_AGENT_NOISY, "AgentClaim::contains: bitfield (tile %d): %s "
												 "(value %" PRIuPTR ")\n",
				resource.tileID, bitfield_str,
				resources[resource.tileID]);
	}
	return TEST_BIT(resources[resource.tileID], resource.resourceID);
}

inline bool os::agent::AgentClaim::containsTile(os::agent::TileID tileID) const
{
	return resources[tileID] != 0;
}

inline uint8_t os::agent::AgentClaim::resourcesInTile(TileID tileID) const
{
	uint8_t sum = 0;
	for (uint8_t resource = 0; resource < os::agent::MAX_RES_PER_TILE; resource++)
		if (TEST_BIT(resources[tileID], resource))
			++sum;
	return sum;
}

inline bool os::agent::AgentClaim::containsAllResourcesOfTile(os::agent::TileID tileID) const
{
	for (uint8_t resource = 0; resource < os::agent::MAX_RES_PER_TILE; resource++)
	{
		if (!TEST_BIT(resources[tileID], resource) && os::agent::HardwareMap[tileID][resource].type != ResType::none)
		{
			return false;
		}
	}
	return true;
}

inline uint8_t os::agent::AgentClaim::getResourceCount(int *PECount, int *TileCount, ResType type) const
{

	if (!PECount && type == TYPE_ALL)
	{ // Speed-up default case
		if (TileCount)
		{
			*TileCount = tileCount;
		}
		DBG(SUB_AGENT_NOISY, "getResourceCount: returning coreCount == %d\n", coreCount);
		return coreCount;
	}

	uint8_t Count = 0;
	os::agent::ResourceID res;

	if (PECount)
	{
		for (int i = 0; i < os::agent::HWTypes; i++)
		{
			PECount[i] = 0;
		}
	}

	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
	{
		bool hasTile = false;
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
		{
			if ((type == TYPE_ALL && os::agent::HardwareMap[res.tileID][res.resourceID].type != none) ||
					(os::agent::HardwareMap[res.tileID][res.resourceID].type == type))
			{
				if (contains(res))
				{
					Count++;
					if (PECount)
						PECount[os::agent::HardwareMap[res.tileID][res.resourceID].type]++;
					if (TileCount && !hasTile)
					{
						(*TileCount)++;
						hasTile = true;
					}
				}
			}
		}
	}

	return Count;
}

inline uint8_t os::agent::AgentClaim::getOperatingPointIndex() const
{

	return operatingPointIndex;
}

inline void os::agent::AgentClaim::setOperatingPointIndex(uint8_t index)
{

	operatingPointIndex = index;
}

inline uint8_t os::agent::AgentClaim::getDifference(os::agent::AgentClaim *otherClaim) const
{

	uint8_t Count = 0;

	os::agent::ResourceID res;

	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
		{
			if (os::agent::HardwareMap[res.tileID][res.resourceID].type != none)
			{
				if (contains(res) != otherClaim->contains(res))
				{
					Count++;
				}
			}
		}
	}

	return Count;
}

inline void os::agent::AgentClaim::addClaim(const os::agent::AgentClaim &otherClaim)
{

	os::agent::ResourceID res;

	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
		{
			if (os::agent::HardwareMap[res.tileID][res.resourceID].type != none)
			{
				if (otherClaim.contains(res))
				{
					this->add(res);
				}
			}
		}
	}
}

inline void os::agent::AgentClaim::reset(bool newucid)
{
	if (newucid)
	{
		ucid = os::agent::AgentClaim::getNewUCID();
	}

	for (TileID tile = 0; tile < hw::hal::Tile::MAX_TILE_COUNT; tile++)
		resources[tile] = 0;

	owningAgent = nullptr;

	coreCount = 0;
	tileCount = 0;

	resetTypeCounter();

	this->unsetAgentOctoClaim();
}

inline void os::agent::AgentClaim::moveAllResourcesToOtherClaim(os::agent::AgentClaim &otherClaim)
{
	ResourceID res;
	for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++)
	{
		for (res.resourceID = 0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++)
		{
			if (contains(res))
			{
				this->remove(res);
				otherClaim.add(res);
			}
		}
	}
}

inline void *os::agent::AgentClaim::operator new(size_t s) throw()
{
	return os::agent::AgentMemory::agent_mem_allocate(s);
}

inline void os::agent::AgentClaim::operator delete(void *p)
{
	os::agent::AgentMemory::agent_mem_free(p);
}

inline void *os::agent::AgentMalleabilityClaim::operator new(size_t s) throw()
{
	return os::agent::AgentMemory::agent_mem_allocate(s);
}

inline void os::agent::AgentMalleabilityClaim::operator delete(void *p)
{
	os::agent::AgentMemory::agent_mem_free(p);
}

inline void *os::agent::AgentOctoClaim::operator new(size_t s) throw()
{
	return os::agent::AgentMemory::agent_mem_allocate(s);
}

inline void os::agent::AgentOctoClaim::operator delete(void *p)
{
	os::agent::AgentMemory::agent_mem_free(p);
}

#endif // OS_AGENT_CLAIM_H
