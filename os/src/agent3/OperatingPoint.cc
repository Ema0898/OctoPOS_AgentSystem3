#include "os/agent3/OperatingPoint.h"
#include "lib/debug.h"
#include "os/agent3/AgentMemory.h"
#include "lib/kassert.h"

os::agent::OperatingPoint::OperatingPoint()
{
    number_of_cluster_guarantees = 1;
    number_of_clusters = 1;
    current_cluster_guarantee_index = 0;
    current_cluster_index = 0;
    //    cluster_id_list = new uint8_t[OP_MAX_NUMBER_OF_CLUSTERS];
    //    cluster_guarantee_id_list = new uint8_t[OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES];
    //    for(uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES; i++)
    //    {
    //        cluster_guarantee_id_list[i] = op.cluster_guarantee_id_list[i];
    //    }
    //    for(uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTERS; i++)
    //    {
    //        cluster_id_list[i] = op.cluster_id_list[i];
    //    }
}

os::agent::OperatingPoint::OperatingPoint(uint8_t number_of_cluster_guarantees, uint8_t number_of_clusters)
{
    if (number_of_clusters > OP_MAX_NUMBER_OF_CLUSTERS || number_of_cluster_guarantees > OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES)
    {
        panic("OperatingPoint(...): one of the parameter is bigger than the specified maxim size\n");
        kassert(number_of_clusters <= OP_MAX_NUMBER_OF_CLUSTERS);
        kassert(number_of_cluster_guarantees <= OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES);
    }
    this->number_of_cluster_guarantees = number_of_cluster_guarantees;
    this->number_of_clusters = number_of_clusters;
    current_cluster_guarantee_index = 0;
    current_cluster_index = 0;
    //    cluster_id_list = new uint8_t[OP_MAX_NUMBER_OF_CLUSTERS];
    //    cluster_guarantee_id_list = new uint8_t[OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES];
    //    for(uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES; i++)
    //    {
    //        cluster_guarantee_id_list[i] = 100;
    //    }
    //    for(uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTERS; i++)
    //    {
    //        cluster_id_list[i] = 100;
    //    }
}

os::agent::OperatingPoint::OperatingPoint(const OperatingPoint &op)
{
    number_of_cluster_guarantees = op.number_of_cluster_guarantees;
    number_of_clusters = op.number_of_clusters;
    current_cluster_guarantee_index = op.current_cluster_guarantee_index;
    current_cluster_index = op.current_cluster_index;
    for (uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES; i++)
    {
        cluster_guarantee_id_list[i] = op.cluster_guarantee_id_list[i];
    }
    for (uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTERS; i++)
    {
        cluster_id_list[i] = op.cluster_id_list[i];
    }
}

os::agent::OperatingPoint::~OperatingPoint()
{
    //    delete cluster_guarantee_id_list;
    //    delete cluster_id_list;
}

void os::agent::OperatingPoint::add_cluster_guarantee_id(uint8_t cg_id)
{
    if (current_cluster_guarantee_index > number_of_cluster_guarantees - 1)
        panic("add_cluster_guarantee_id(...) : there are too many element in the list\n");
    kassert(current_cluster_guarantee_index < number_of_cluster_guarantees);
    // TODO : verify if first and second cluster types are needed ????
    cluster_guarantee_id_list[current_cluster_guarantee_index] = cg_id;
    current_cluster_guarantee_index++;
}

void os::agent::OperatingPoint::add_cluster_id(uint8_t c_id)
{
    if (current_cluster_index > number_of_clusters - 1)
        panic("add_needed_cluster_type(...) : there are too many element in the list\n");
    kassert(current_cluster_index < number_of_clusters);

    for (uint8_t i = 0; i < current_cluster_index; i++)
    {
        if (cluster_id_list[i] == c_id)
            panic("add_needed_cluster_type(...) : the same resource type cannot be added a second time\n");
        kassert(cluster_id_list[i] != c_id);
    }

    cluster_id_list[current_cluster_index] = c_id;
    current_cluster_index++;
}

uint8_t *os::agent::OperatingPoint::get_cluster_guarantee_id_list()
{
    return cluster_guarantee_id_list;
}

uint8_t *os::agent::OperatingPoint::get_cluster_id_list()
{
    return cluster_id_list;
}

uint8_t os::agent::OperatingPoint::get_number_of_cluster_guarantees()
{
    return number_of_cluster_guarantees;
}

uint8_t os::agent::OperatingPoint::get_number_of_clusters()
{
    return number_of_clusters;
}

uint8_t os::agent::OperatingPoint::get_current_cluster_guarantee_index()
{
    return current_cluster_guarantee_index;
}

uint8_t os::agent::OperatingPoint::get_current_cluster_index()
{
    return current_cluster_index;
}

void *os::agent::OperatingPoint::operator new(size_t s)
{
    DBG(SUB_AGENT, "CREATE OP\n");
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void *os::agent::OperatingPoint::operator new[](size_t s)
{
    DBG(SUB_AGENT, "CREATE[] OP\n");
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::OperatingPoint::operator delete(void *c, size_t s)
{
    DBG(SUB_AGENT, "DELETE OP\n");
    os::agent::AgentMemory::agent_mem_free(c);
}

void os::agent::OperatingPoint::operator delete[](void *c, size_t s)
{
    DBG(SUB_AGENT, "DELETE[] OP\n");
    os::agent::AgentMemory::agent_mem_free(c);
}
size_t os::agent::OperatingPoint::flatten(char *buf, size_t max, size_t offset)
{
    size_t needed = (sizeof(os::agent::OperatingPoint) + (sizeof(uint8_t) * OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES) + (sizeof(uint8_t) * OP_MAX_NUMBER_OF_CLUSTERS));

    DBG(SUB_AGENT, "max = %" PRIuPTR ", needed = %" PRIuPTR " \n", max, needed);
    if (max < needed)
    {
        DBG(SUB_AGENT, ":OperatingPoint::flatten ==> maximal buffer < needed buffer \n");
        return 0;
    }

    os::agent::OperatingPoint *operating_point = (os::agent::OperatingPoint *)(buf + offset);
    uint8_t *cluster_guarantee_id_list = (uint8_t *)(buf + offset + (sizeof(os::agent::OperatingPoint)));
    uint8_t *cluster_id_list = (uint8_t *)(buf + offset + (sizeof(os::agent::OperatingPoint)) + (sizeof(uint8_t) * OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES));

    // Flatten Operating point
    *operating_point = *this;

    // Flatten list of cluster guarantees id
    for (uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES; i++)
    {
        cluster_guarantee_id_list[i] = this->cluster_guarantee_id_list[i];
    }

    // Flatten list of needed cluster types
    for (uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTERS; i++)
    {
        cluster_id_list[i] = this->cluster_id_list[i];
    }
    DBG(SUB_AGENT, "return needed = %" PRIuPTR "\n", needed);
    return needed;
}
