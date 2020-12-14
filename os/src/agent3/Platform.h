#ifndef OS_AGENT_PLATFORM_H
#define OS_AGENT_PLATFORM_H

#include "os/agent3/AgentRPCHeader.h"

namespace os
{
    namespace agent
    {

        struct TileResource_s
        {
            ResType type;
            uint8_t flags;
        }; // Should be in hw:: somewhere?!

        //TODO: remove – HardwareMap is deprecated !
        extern TileResource_s HardwareMap[hw::hal::Tile::MAX_TILE_COUNT][os::agent::MAX_RES_PER_TILE];

    } // namespace agent
} // namespace os

#endif
