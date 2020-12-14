#include "os/agent3/FlatCluster.h"
#include "os/agent3/Cluster.h"
#include "lib/debug.h"

bool os::agent::FlatCluster::flatten(Cluster *source, uint8_t number_of_clusters)
{

    size_t offset = 0;

    for (uint8_t i = 0; i < number_of_clusters; i++)
    {
        this->dataUsed += source[i].flatten(this->data, FLAT_CLUSTER_SIZE, offset + this->dataUsed);
    }

    return (this->dataUsed != 0);
}

void os::agent::FlatCluster::unflatten(Cluster **target, uint8_t number_of_clusters) const
{
    size_t consumed = 0;

    for (uint8_t i = 0; i < number_of_clusters; i++)
    {
        consumed += this->unflattenMember((&((*target)[i])), consumed);
    }
}

size_t os::agent::FlatCluster::unflattenMember(Cluster *op, size_t offset) const
{

    if ((size_t)this->dataUsed <= offset)
    {
        DBG(SUB_AGENT, "UnflattenOutside returns NULL (end of data reached)\n");
        //*target = NULL;
        return 0;
    }

    size_t consumed = sizeof(Cluster);
    Cluster *temp = (Cluster *)((size_t) & (this->data[0]) + offset);
    *op = *temp;

    return consumed;
}
