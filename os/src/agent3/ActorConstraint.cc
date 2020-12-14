#include "os/agent3/ActorConstraint.h"
#include "os/agent3/AgentConstraint.h"
#include "os/agent3/AgentSystem.h"
#include "lib/debug.h"
#include "lib/kassert.h"

os::agent::ActorConstraint::ActorConstraint()
{
    tiles.setAll();
    number_of_clusters = AC_MAX_NUMBER_OF_CLUSTERS;
    number_of_cluster_guarantees = AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES;
    number_of_clusters = AC_MAX_NUMBER_OF_OPERATING_POINTS;
    current_cluster_index = 0;
    current_cluster_guarantee_index = 0;
    current_operating_point_index = 0;
    cluster_list = new Cluster[AC_MAX_NUMBER_OF_CLUSTERS];
    cluster_guarantee_list = new ClusterGuarantee[AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES];
    operating_point_list = new OperatingPoint[AC_MAX_NUMBER_OF_OPERATING_POINTS];
}

os::agent::ActorConstraint::ActorConstraint(uint8_t number_of_clusters, uint8_t number_of_cluster_guarantees, uint8_t number_of_operating_points)
{
    if (number_of_clusters > AC_MAX_NUMBER_OF_CLUSTERS || number_of_cluster_guarantees > AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES || number_of_operating_points > AC_MAX_NUMBER_OF_OPERATING_POINTS)
        panic("ActorConstraint(...): one of the parameter is bigger than the specified maxim size\n");
    kassert(number_of_clusters < (AC_MAX_NUMBER_OF_CLUSTERS + 1) && number_of_cluster_guarantees < (AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES + 1) && number_of_operating_points < (AC_MAX_NUMBER_OF_OPERATING_POINTS + 1));
    tiles.setAll();
    this->number_of_clusters = number_of_clusters;
    this->number_of_cluster_guarantees = number_of_cluster_guarantees;
    this->number_of_operating_points = number_of_operating_points;
    current_cluster_index = 0;
    current_cluster_guarantee_index = 0;
    current_operating_point_index = 0;
    cluster_list = new Cluster[AC_MAX_NUMBER_OF_CLUSTERS];
    cluster_guarantee_list = new ClusterGuarantee[AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES];
    operating_point_list = new OperatingPoint[AC_MAX_NUMBER_OF_OPERATING_POINTS];
}

os::agent::ActorConstraint::ActorConstraint(const ActorConstraint &ac)
{
    tiles = ac.tiles;
    cluster_list = ac.cluster_list;
    cluster_guarantee_list = ac.cluster_guarantee_list;
    operating_point_list = ac.operating_point_list;

    number_of_clusters = ac.number_of_clusters;
    number_of_cluster_guarantees = ac.number_of_cluster_guarantees;
    number_of_operating_points = ac.number_of_operating_points;

    current_cluster_index = ac.current_cluster_index;
    current_cluster_guarantee_index = ac.current_cluster_guarantee_index;
    current_operating_point_index = ac.current_operating_point_index;
}

os::agent::ActorConstraint::~ActorConstraint()
{
    DBG(SUB_AGENT, "DELETE ACTOR CONSTRAINT\n");
    // Commented out as a quick (and dirty) fix to prevent a segfault during memfree
    // This should be fixed properly later as it creates a memory leak!
    //    delete[] operating_point_list;
    //    delete[] cluster_guarantee_list;
    //    delete[] cluster_list;
}

bool os::agent::ActorConstraint::isAgentConstraint() const
{
    DBG(SUB_AGENT, "It's ACTOR CONSTRAINT\n");
    return false;
}
uint8_t os::agent::ActorConstraint::add_cluster(uint8_t type)
{
    kassert(current_cluster_index < number_of_clusters);
    kassert(type >= TILE_RISC_ID && type <= TILE_TCPA_ID);
    Cluster cl(type, current_cluster_index);
    cluster_list[current_cluster_index] = cl;

    if (current_cluster_index >= AC_MAX_NUMBER_OF_CLUSTERS)
        panic("add_cluster: you cannot add more clusters than now");

    return current_cluster_index++;
}

uint8_t os::agent::ActorConstraint::add_cluster_guarantee(uint8_t first_cluster_id, uint8_t second_cluster_id, uint8_t hopDistance, uint8_t serviceLevel)
{
    kassert(current_cluster_guarantee_index < number_of_cluster_guarantees);
    kassert((first_cluster_id < current_cluster_index) && (second_cluster_id < current_cluster_index));
    ClusterGuarantee guarantee(cluster_list[first_cluster_id], cluster_list[second_cluster_id], hopDistance, serviceLevel);
    cluster_guarantee_list[current_cluster_guarantee_index] = guarantee;

    if (current_cluster_guarantee_index >= AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES)
        panic("add_cluster: you cannot add more cluster guarantees than now");

    return current_cluster_guarantee_index++;
}

uint8_t os::agent::ActorConstraint::add_operating_point(uint8_t number_of_cluster_guarantees, uint8_t number_of_needed_cluster_types)
{
    kassert(current_operating_point_index < number_of_operating_points);
    OperatingPoint operating_point(number_of_cluster_guarantees, number_of_needed_cluster_types);
    operating_point_list[current_operating_point_index] = operating_point;

    if (current_operating_point_index >= AC_MAX_NUMBER_OF_OPERATING_POINTS)
        panic("add_cluster: you cannot add more operating point than now");

    return current_operating_point_index++;
}

void os::agent::ActorConstraint::add_cluster_to_operating_point(uint8_t op_id, uint8_t c_id)
{
    kassert((op_id < current_operating_point_index) && (c_id < current_cluster_index));
    operating_point_list[op_id].add_cluster_id(c_id);
}

void os::agent::ActorConstraint::add_cluster_guarantee_to_operating_point(uint8_t op_id, uint8_t cg_id)
{
    kassert((op_id < current_operating_point_index) && (cg_id < current_cluster_guarantee_index));

    operating_point_list[op_id].add_cluster_guarantee_id(cg_id);
}

os::agent::Cluster *os::agent::ActorConstraint::get_cluster_list() const
{
    return cluster_list;
}

os::agent::ClusterGuarantee *os::agent::ActorConstraint::get_cluster_guarantee_list() const
{
    return cluster_guarantee_list;
}

os::agent::OperatingPoint *os::agent::ActorConstraint::get_operating_point_list() const
{
    return operating_point_list;
}

uint8_t os::agent::ActorConstraint::get_current_cluster_index() const
{
    return current_cluster_index;
}

uint8_t os::agent::ActorConstraint::get_current_cluster_guarantee_index() const
{
    return current_cluster_guarantee_index;
}

uint8_t os::agent::ActorConstraint::get_current_operating_point_index() const
{
    return current_operating_point_index;
}

void os::agent::ActorConstraint::set_cluster_list(Cluster *cl_list)
{
    cluster_list = cl_list;

    for (int i = 0; i < current_cluster_index; i++)
    {
        cluster_list[i] = cl_list[i];
    }
}

void os::agent::ActorConstraint::set_cluster_guarantee_list(os::agent::ClusterGuarantee *cg_list)
{
    cluster_guarantee_list = cg_list;
    for (int i = 0; i < current_cluster_guarantee_index; i++)
    {
        cluster_guarantee_list[i] = cg_list[i];
    }
}

void os::agent::ActorConstraint::set_operating_point_list(os::agent::OperatingPoint *op_list)
{
    operating_point_list = op_list;
    for (int i = 0; i < current_operating_point_index; i++)
    {
        operating_point_list[i] = op_list[i];
    }
}

bool os::agent::ActorConstraint::is_tile_usable(const os::agent::TileID tileID) const
{

    bool isTileUsable = false;

    /* Check if tile is eligible according to constraints. */
    if (tiles.get(tileID))
    {
        isTileUsable = true;
    }
    else
    {
        isTileUsable = false;
    }

    return (isTileUsable);
}

bool os::agent::ActorConstraint::is_resource_usable(const os::agent::ResourceID &resource) const
{

    bool isResourceUsable = false;

    for (int i = 0; i < current_cluster_guarantee_index; i++)
    {
        if (((cluster_guarantee_list[i].get_first_cluster().get_type() - 1) == AgentSystem::getHWType(resource)) ||
            ((cluster_guarantee_list[i].get_second_cluster().get_type() - 1) == AgentSystem::getHWType(resource)))
        {
            isResourceUsable = true;
            break;
        }
    }

    return (isResourceUsable);
}

void *os::agent::ActorConstraint::operator new(size_t s)
{
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::ActorConstraint::operator delete(void *c, size_t s)
{
    os::agent::AgentMemory::agent_mem_free(c);
}
