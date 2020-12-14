#include "os/agent3/ClusterGuarantee.h"
#include "lib/debug.h"
#include "os/agent3/AgentMemory.h"

os::agent::ClusterGuarantee::ClusterGuarantee()
{
    hop_distance = 0;
    service_level = 0;
}

os::agent::ClusterGuarantee::ClusterGuarantee(Cluster first_cluster, Cluster second_cluster, uint8_t hop_distance, uint16_t service_level)
{
    this->first_cluster = first_cluster;
    this->second_cluster = second_cluster;
    this->hop_distance = hop_distance;
    this->service_level = service_level;
}

os::agent::ClusterGuarantee::ClusterGuarantee(const ClusterGuarantee &obj)
{
    this->first_cluster = obj.first_cluster;
    this->second_cluster = obj.second_cluster;
    this->hop_distance = obj.hop_distance;
    this->service_level = obj.service_level;
}

os::agent::ClusterGuarantee::~ClusterGuarantee()
{
}

os::agent::Cluster os::agent::ClusterGuarantee::get_first_cluster()
{
    return first_cluster;
}

os::agent::Cluster os::agent::ClusterGuarantee::get_second_cluster()
{
    return second_cluster;
}

void os::agent::ClusterGuarantee::setHopDistance(uint8_t hop_distance)
{
    this->hop_distance = hop_distance;
}

uint8_t os::agent::ClusterGuarantee::getHopDistance()
{
    return hop_distance;
}
void os::agent::ClusterGuarantee::setServiceLevel(uint16_t service_level)
{
    this->service_level = service_level;
}

uint16_t os::agent::ClusterGuarantee::getServiceLevel()
{
    return service_level;
}

void *os::agent::ClusterGuarantee::operator new(size_t s)
{
    DBG(SUB_AGENT, "CREATE CG\n");
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void *os::agent::ClusterGuarantee::operator new[](size_t s)
{
    DBG(SUB_AGENT, "CREATE[] CG\n");
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::ClusterGuarantee::operator delete(void *c, size_t s)
{
    DBG(SUB_AGENT, "DELETE CG\n");
    os::agent::AgentMemory::agent_mem_free(c);
}

void os::agent::ClusterGuarantee::operator delete[](void *c, size_t s)
{
    DBG(SUB_AGENT, "DELETE[] CG\n");
    os::agent::AgentMemory::agent_mem_free(c);
}

size_t os::agent::ClusterGuarantee::flatten(char *buf, size_t max, size_t offset)
{

    size_t needed = sizeof(os::agent::ClusterGuarantee);
    if (max < needed)
        return 0;

    os::agent::ClusterGuarantee *guarantee = (os::agent::ClusterGuarantee *)(buf + offset);
    *guarantee = *this;

    return needed;
}
