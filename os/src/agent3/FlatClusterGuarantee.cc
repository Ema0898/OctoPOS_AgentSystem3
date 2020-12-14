#include "os/agent3/FlatClusterGuarantee.h"
#include "os/agent3/ClusterGuarantee.h"
#include "lib/debug.h"

bool os::agent::FlatClusterGuarantee::flatten(ClusterGuarantee *source, uint8_t number_of_cgs)
{

    size_t offset = 0;

    for (uint8_t i = 0; i < number_of_cgs; i++)
    {
        this->dataUsed += source[i].flatten(this->data, FLAT_CLUSTER_GUARANTEE_SIZE, offset + this->dataUsed);
    }

    return (this->dataUsed != 0);
}

void os::agent::FlatClusterGuarantee::unflatten(ClusterGuarantee **target, uint8_t number_of_cgs) const
{
    size_t consumed = 0;

    for (uint8_t i = 0; i < number_of_cgs; i++)
    {
        consumed += this->unflattenMember((&((*target)[i])), consumed);
    }
}

size_t os::agent::FlatClusterGuarantee::unflattenMember(ClusterGuarantee *op, size_t offset) const
{

    if ((size_t)this->dataUsed <= offset)
    {
        DBG(SUB_AGENT, "UnflattenOutside returns NULL (end of data reached)\n");
        //*target = NULL;
        return 0;
    }

    size_t consumed = sizeof(ClusterGuarantee);
    ClusterGuarantee *temp = (ClusterGuarantee *)((size_t) & (this->data[0]) + offset);
    *op = *temp;

    return consumed;
}
