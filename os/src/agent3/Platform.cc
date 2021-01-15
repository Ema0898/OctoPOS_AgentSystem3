#include "os/agent3/Platform.h"
#include "hw/hal/Tile.h"

// $tileCount x $maxPEs
os::agent::TileResource_s os::agent::HardwareMap[hw::hal::Tile::MAX_TILE_COUNT][os::agent::MAX_RES_PER_TILE];
