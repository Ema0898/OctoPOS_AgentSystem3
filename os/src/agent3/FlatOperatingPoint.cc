#include "os/agent3/FlatOperatingPoint.h"
#include "os/agent3/OperatingPoint.h"
#include "lib/debug.h"

bool os::agent::FlatOperatingPoint::flatten(OperatingPoint *source, uint8_t number_of_ops)
{
    size_t offset = 0;

    for (uint8_t i = 0; i < number_of_ops; i++)
    {
        DBG(SUB_AGENT, "for loop \n");
        this->dataUsed += source[i].flatten(this->data, FLAT_OPERATING_POINT_SIZE, (offset + this->dataUsed));
    }

    return (this->dataUsed != 0);
}

void os::agent::FlatOperatingPoint::unflatten(OperatingPoint **target, uint8_t number_of_ops) const
{
    size_t consumed = 0;

    for (uint8_t i = 0; i < number_of_ops; i++)
    {
        consumed += this->unflattenMember((&((*target)[i])), consumed);
    }
}

size_t os::agent::FlatOperatingPoint::unflattenMember(OperatingPoint *operating_point, size_t offset) const
{
    if ((size_t)this->dataUsed <= offset)
    {
        DBG(SUB_AGENT, "UnflattenOutside returns NULL (end of data reached)\n");
        //*target = NULL;
        return 0;
    }

    // unflatten operating point
    OperatingPoint *temp_operating_point = (OperatingPoint *)((size_t) & (this->data[0]) + offset);
    *operating_point = *temp_operating_point;

    // unflatten list of cluster guarantees id
    uint8_t *temp_cluster_guarantee_id_list = (uint8_t *)((size_t) & (this->data[0]) + offset + sizeof(OperatingPoint));
    for (uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES; i++)
    {
        operating_point->cluster_guarantee_id_list[i] = temp_cluster_guarantee_id_list[i];
    }

    // unflatten list of needed cluster types
    uint8_t *temp_cluster_id_list = (uint8_t *)((size_t) & (this->data[0]) + offset + sizeof(OperatingPoint) + (sizeof(uint8_t) * OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES));
    for (uint8_t i = 0; i < OP_MAX_NUMBER_OF_CLUSTERS; i++)
    {
        operating_point->cluster_id_list[i] = temp_cluster_id_list[i];
    }

    size_t consumed = (sizeof(os::agent::OperatingPoint) + (sizeof(uint8_t) * OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES) + (sizeof(uint8_t) * OP_MAX_NUMBER_OF_CLUSTERS));

    return consumed;
}
