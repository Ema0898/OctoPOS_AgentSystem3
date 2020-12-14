#include "AgentClaim.h"
#include "AgentSystem.h"
#include "Agent.h"
#include "ActorConstraintSolver.h"

#include "os/dev/HWInfo.h"

os::agent::AgentInstance *os::agent::AgentSystem::idleAgent = NULL;
os::agent::AgentInstance *os::agent::AgentSystem::initialAgent = NULL;

os::agent::ActorConstraintSolver *os::agent::AgentSystem::solver = NULL;

uint8_t os::agent::AgentSystem::agentCount;
uint8_t os::agent::AgentSystem::nextAgent;
os::agent::AgentInstance *os::agent::AgentSystem::instances[MAX_AGENTS];

os::agent::TileConstraint *os::agent::AgentSystem::tc;
os::agent::PEQuantityConstraint *os::agent::AgentSystem::peqc;
os::agent::TileSharingConstraint *os::agent::AgentSystem::tsc;
os::agent::AndConstraintList *os::agent::AgentSystem::acl;

lib::adt::SimpleSpinlock os::agent::AgentSystem::agentLock;

os::agent::AgentSystem::sysRes_s os::agent::AgentSystem::systemResources[hw::hal::Tile::MAX_TILE_COUNT][os::agent::MAX_RES_PER_TILE];

uint32_t os::agent::AgentSystem::AGENT_TILE;
os::res::DispatchClaim os::agent::AgentSystem::AGENT_DISPATCHCLAIM;

void os::agent::AgentSystem::lockBargaining(AgentClaim *claim) {
	// make this claim unavailable for bargaining, e.g. because of octopos invade phase!
	ResourceID res;
	for (res.tileID=0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++ ) {
		for (res.resourceID=0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++) {
			if(os::agent::HardwareMap[res.tileID][res.resourceID].type != none) {
				if(claim->contains(res)) {
					os::agent::AgentSystem::clearFlag(res, os::agent::AgentSystem::FLAG_AVAILABLE_FOR_BARGAINING);
				}
			}
		}
	}
}

void os::agent::AgentSystem::unlockBargaining(AgentClaim *claim) {
	ResourceID res;
	for (res.tileID=0; res.tileID < hw::hal::Tile::getTileCount(); res.tileID++ ) {
		for (res.resourceID=0; res.resourceID < os::agent::MAX_RES_PER_TILE; res.resourceID++) {
			if(os::agent::HardwareMap[res.tileID][res.resourceID].type != none) {
				if(claim->contains(res)) {
					os::agent::AgentSystem::setFlag(res, os::agent::AgentSystem::FLAG_AVAILABLE_FOR_BARGAINING);
				}
			}
		}
	}
}

const os::agent::AgentInstance& os::agent::AgentSystem::get_other_agent(const TileID tile_id, const AgentInstance &agent_id) {
	/*
	 * Find out whether another agent is holding a resource on this tile or it's empty
	 * or only used by the specified agent.
	 * Note, that we only care about the first "foreign" match.
	 */
	const AgentInstance *otherAgent = NULL;
	os::agent::ResourceID res;
	res.tileID = tile_id;
	for (std::size_t i = 0; i < os::agent::MAX_RES_PER_TILE; ++i) {
		res.resourceID = i;
		AgentInstance *cur_agent = getOwner(res);
		if (!((cur_agent == &agent_id) || (!cur_agent) || (cur_agent == idleAgent))) {
			otherAgent = cur_agent;
			break;
		}
	}

	/* otherAgent is either pointing to the first different agent or still to NULL at this point. */
	if (!otherAgent) {
		otherAgent = &agent_id;
	}

	return(*otherAgent);
}

bool os::agent::AgentSystem::can_use_tile_exclusively(const TileID tile_id, const AgentInstance &agent_id) {
	const AgentInstance &otherAgent = get_other_agent(tile_id, agent_id);

	/*
	 * If the tile is empty or the only agent holding resources is the provided agent
	 * or the idle agent, it is available for exclusive use.
	 */
	if (&otherAgent == &agent_id) {
		return(true);
	}

	return(false);
}

uint8_t os::agent::AgentSystem::collectMalleabilityClaims(os::agent::AgentMalleabilityClaim ***ptrToBuffer,
	os::agent::AgentInstance *callingAgent, int slot) {
    uintptr_t size = sizeof(os::agent::AgentMalleabilityClaim*) * MAX_MALLEABILITY_CLAIMS;
    DBG(SUB_AGENT, "allocating %" PRIuPTR " bytes for malleability pools\n", size);
    *ptrToBuffer = (os::agent::AgentMalleabilityClaim**) os::agent::AgentMemory::agent_mem_allocate(size);
    if (!*ptrToBuffer) {
		DBG(SUB_AGENT, "agent_mem_allocate failed\n");
		return 0;
    }
    os::agent::AgentMalleabilityClaim **buffer = *ptrToBuffer;

    uint8_t numClaims = 0;

    uint8_t index = 2; // jump over idle and init agent
    uint8_t visited = 2;
    while (visited < agentCount && index < MAX_AGENTS) {
		while (instances[index] == NULL) index++;

		DBG(SUB_AGENT, "Calling getMalleabilityClaims on agent %p (%" PRIu8 ". agent of %" PRIu8 ")\n",
			instances[index], visited - 1, agentCount - 2);
		numClaims += instances[index]->getMalleabilityClaims(&buffer[numClaims], &buffer[MAX_MALLEABILITY_CLAIMS],
			callingAgent, slot);
		if (numClaims == MAX_MALLEABILITY_CLAIMS) {
			break;
		}

		index++;
		visited++;
    }

    return numClaims;
}

void os::agent::AgentSystem::dumpAgents() {
    DBG(SUB_AGENT, "This should print %" PRIu8 " agents.\n", agentCount);
    uint8_t count = 1;
    for (uint8_t i = 0; i < MAX_AGENTS; ++i) {
	if (instances[i] != NULL) {
	    DBG(SUB_AGENT, "%" PRIu8 ". agent: %p, id: %" PRIu8 "\n", count, instances[i], i);
	    count++;
	}
    }

}

void os::agent::AgentSystem::init() {
	if ( os_agent_agentsystem_no_init != 0 ) {
		return;
	}

	if (os::agent::MAX_RES_PER_TILE > 64){
		panic("Agent system does not support more than 64 PEs per tile\n");
	}

	std::size_t lastCore = hw::hal::CPU::getCPUCount() - 1;
	if ( lastCore >= 64 ) {
		panic("Agent system does not support more than 64 PEs per tile\n");
	}

	if (hw::hal::Tile::existsIOTile()) {
		os::agent::AgentSystem::AGENT_TILE=hw::hal::Tile::getIOTileID();
		os::agent::AgentSystem::AGENT_DISPATCHCLAIM = os::res::DispatchClaim(255);
	}
#ifdef cf_board_x64native
	else {
		os::agent::AgentSystem::AGENT_TILE = hw::hal::Tile::getTileCount() - 1;
		if (hw::hal::Tile::getTileID() == os::agent::AgentSystem::AGENT_TILE) {
			os::res::ResourceManager& resMgr = os::res::ResourceManager::Inst();
			os::proc::Claim *claim = resMgr.constructClaim();
			uintptr_t coreIDMask = 1 << lastCore;
			if (claim->invadeCores(coreIDMask) != 0) {
				panic("os::agent::AgentSystem::init: Cannot invade agent core!");
			}
			TID tileID = hw::hal::Tile::getTileID();
			os::agent::AgentSystem::AGENT_DISPATCHCLAIM = os::res::DispatchClaim(tileID, claim->getTag());
		}
		else {
			os::agent::AgentSystem::AGENT_DISPATCHCLAIM = os::res::DispatchClaim(255);
		}
	}
#endif

	DBG(SUB_AGENT, "Setting up os::agent::AgentSystem::AGENT_TILE %" PRIu32 "\n",os::agent::AgentSystem::AGENT_TILE);
	DBG(SUB_AGENT, "Size of Agent is %" PRIuPTR " \n", sizeof(os::agent::AgentInstance));
	//create idle agent

	dev::HWInfo &hwinfo = dev::HWInfo::Inst();
		// Fill hardware map
		DBG(SUB_AGENT, "filling hardware map\n");
		const uint8_t tileCount = hwinfo.getTileCount();
		for(TID tid = 0; tid < dev::HWInfo::SYS_MAX_TILES; ++tid) {
			if(tid >= tileCount){
				for(std::size_t core = 0 ; core < dev::HWInfo::SYS_MAX_CORES_PER_TILE; ++core) {
					os::agent::HardwareMap[tid][core].type = ResType::none;
					os::agent::HardwareMap[tid][core].flags = 0;
				}
			}else{
				const std::size_t coreCount = hwinfo.getRemoteCoreCount(tid);
				DBG(SUB_AGENT, "%" PRIuPTR " cores on tile %" PRIu8 "\n", coreCount, tid);
				for(std::size_t core = 0 ; core < dev::HWInfo::SYS_MAX_CORES_PER_TILE; ++core) {
					if (core >= coreCount || (tid == AGENT_TILE && core == lastCore)) {
						os::agent::HardwareMap[tid][core].type = ResType::none;
						os::agent::HardwareMap[tid][core].flags = 0;
					} else {
						const dev::HWInfo::TileType tileType = hwinfo.getTileType(tid);
						const dev::HWInfo::CoreType coreType = hwinfo.getRemoteCoreType(tid, core);

						switch(tileType){
							case dev::HWInfo::TCPATile:
								switch(coreType){
									case dev::HWInfo::ComputeCore:
										// ignore all compute cores on TCPA tiles ...
										//printf("ComputeCore on TCPATile - core %d on tile %d\n", core, tid);
										// Hardcode all tiles to TCPA-Tiles.
										os::agent::HardwareMap[tid][core].type  = ResType::TCPA;
										os::agent::HardwareMap[tid][core].flags = 0;
										//if(core == TCPA_CORE_ID){
										//	os::agent::HardwareMap[tid][core].type  = ResType::TCPA;
										//	os::agent::HardwareMap[tid][core].flags = 0;
										//}else{
										//	os::agent::HardwareMap[tid][core].type  = ResType::RISC;
										//	os::agent::HardwareMap[tid][core].flags = 0;
										//}
										break;
									case dev::HWInfo::iCore:
										// ignore all iCores on TCPA tiles ...
										//printf("iCore on TCPATile - core %d on tile %d\n", core, tid);
										os::agent::HardwareMap[tid][core].type  = ResType::none;
										os::agent::HardwareMap[tid][core].flags = 0;
										//os::agent::HardwareMap[tid][core].type = ResType::iCore;
										//os::agent::HardwareMap[tid][core].flags = 0;
										break;
									default:
										// ignore all other core types on TCPA tiles ...
										//printf("Core is something else on - core %d on tile %d\n",core, tid);
										os::agent::HardwareMap[tid][core].type  = ResType::none;
										os::agent::HardwareMap[tid][core].flags = 0;
								}
								break;

							case dev::HWInfo::ComputeTile:
								switch(coreType){
									case dev::HWInfo::ComputeCore:
										// register compute core in HardwareMap ...
										//printf("ComputeCore on ComputeTile - core %d on tile %d\n", core, tid);
										os::agent::HardwareMap[tid][core].type = ResType::RISC;
										os::agent::HardwareMap[tid][core].flags = 0;
										break;
									case dev::HWInfo::iCore:
										// register iCore in HardwareMap ...
										//printf("iCore on ComputeTile - core %d on tile %d\n", core, tid);
										os::agent::HardwareMap[tid][core].type = ResType::iCore;
										os::agent::HardwareMap[tid][core].flags = 0;
										break;
									default:
										// ignore all other core types on compute tiles ...
										os::agent::HardwareMap[tid][core].type = ResType::none;
										os::agent::HardwareMap[tid][core].flags = 0;
								}
								break;

							case dev::HWInfo::IOTile:
							default:
								// ignore all cores on other tile types ...
								os::agent::HardwareMap[tid][core].type  = ResType::none;
								os::agent::HardwareMap[tid][core].flags = 0;
						}
					}
				}
			}
		}
		DBG(SUB_AGENT, "filling hardware map - done");

	os::agent::AgentSystem::agentCount = 0;
	os::agent::AgentSystem::nextAgent = 0;
	for (uint8_t i = 0; i < os::agent::MAX_AGENTS; ++i) {
	    os::agent::AgentSystem::instances[i] = NULL;
	}

		DBG(SUB_AGENT, "Init Agents: ");
		os::agent::AgentSystem::solver = new ActorConstraintSolver();
		os::agent::AgentSystem::idleAgent = new AgentInstance(true);
		os::agent::ResourceID resource;

		DBG(SUB_AGENT, "Tiles: %" PRIu32 "/%" PRIu32 ", maxres: %" PRIu32 "\n", hw::hal::Tile::getTileCount(), hw::hal::Tile::MAX_TILE_COUNT, MAX_RES_PER_TILE);

		for (resource.tileID = 0; resource.tileID < hw::hal::Tile::getTileCount(); ++resource.tileID) {
			for (resource.resourceID = 0; resource.resourceID < MAX_RES_PER_TILE; ++resource.resourceID) {
				if ( os::agent::HardwareMap[resource.tileID][resource.resourceID].type != none ) {;
					systemResources[resource.tileID][resource.resourceID].flags = 0;

					setFlag(resource, FLAG_AVAILABLE_FOR_BARGAINING);

					unsetOwner(resource, getOwner(resource));
					DBG(SUB_AGENT, "Idle agent %p gets resource (%d/%d)\n", os::agent::AgentSystem::idleAgent, resource.tileID, resource.resourceID);
					reinterpret_cast<AgentInstance*>(os::agent::AgentSystem::idleAgent)->gainResource(resource);
				}
			}
		}

		os::agent::AgentSystem::initialAgent = new AgentInstance();

		acl = new os::agent::AndConstraintList(NULL, false);

		//  exactly one RISC-Core on Tile 0 for the initial Agent!
	tc = new os::agent::TileConstraint(acl);
	tc->allowTile(0);
	acl->addConstraint(tc);

	peqc = new os::agent::PEQuantityConstraint(acl);
	peqc->setQuantity(1, 1, RISC);
	acl->addConstraint(peqc);

	//TileSharingConstraint allows sharing by default
	tsc = new os::agent::TileSharingConstraint(acl);
	acl->addConstraint(tsc);

	DBG(SUB_AGENT, "Created Constraints, now calling invade_regioncheck\n");
		/* first, invade to make a request slot active */
		int slot = initialAgent->invade_regioncheck(acl);
		/* second, transfer single resource forcefully */
		resource.tileID = resource.resourceID = 0;
		idleAgent->transferResource(resource, initialAgent);
		/* third, finish invasion forcefully */
		initialAgent->getSlot(slot)->stage = octoinvade;

		DBG(SUB_AGENT, "Done init Agents!\n");


	dev::HWInfo::Inst().syncTiles();
}
