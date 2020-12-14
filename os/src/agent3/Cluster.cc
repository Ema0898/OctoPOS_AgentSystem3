#include "os/agent3/Cluster.h"
#include "lib/debug.h"
#include "os/agent3/AgentMemory.h"
#include "lib/kassert.h"

os::agent::Cluster::Cluster()
{
    this->type = 0; // NO_TILE
}

os::agent::Cluster::Cluster(uint8_t type, uint8_t id)
{
    kassert(type >= TILE_RISC_ID && type <= TILE_TCPA_ID);
    this->type = type;
    this->id = id;
}

os::agent::Cluster::Cluster(const Cluster &obj)
{
    kassert(obj.type >= TILE_RISC_ID && obj.type <= TILE_TCPA_ID);
    type = obj.type;
    id = obj.id;
}

os::agent::Cluster::~Cluster()
{
}

uint8_t os::agent::Cluster::get_type()
{
    return type;
}

uint8_t os::agent::Cluster::get_id()
{
    return id;
}

void *os::agent::Cluster::operator new(size_t s)
{
    DBG(SUB_AGENT, "CREATE CLUSTER\n");
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void *os::agent::Cluster::operator new[](size_t s)
{
    DBG(SUB_AGENT, "CREATE[] CLUSTER\n");
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::Cluster::operator delete(void *c, size_t s)
{
    DBG(SUB_AGENT, "DELETE CLUSTER\n");
    os::agent::AgentMemory::agent_mem_free(c);
}

void os::agent::Cluster::operator delete[](void *c, size_t s)
{
    DBG(SUB_AGENT, "DELETE[] CLUSTER\n");
    os::agent::AgentMemory::agent_mem_free(c);
}

size_t os::agent::Cluster::flatten(char *buf, size_t max, size_t offset)
{

    size_t needed = sizeof(os::agent::Cluster);
    if (max < needed)
    {
        return 0;
    }

    os::agent::Cluster *guarantee = (os::agent::Cluster *)(buf + offset);
    *guarantee = *this;

    return needed;
}
