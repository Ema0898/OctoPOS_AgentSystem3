#ifndef OPERATINGPOINT_H
#define OPERATINGPOINT_H

#include <stdint.h>
#include <stddef.h>

namespace os {
namespace agent{

class OperatingPoint
{
    #define OP_MAX_NUMBER_OF_CLUSTERS 6
    #define OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES 5

    public:
	friend class FlatOperatingPoint;

	OperatingPoint();
	OperatingPoint(uint8_t number_of_cluster_guarantees, uint8_t number_of_clusters);
	OperatingPoint(const OperatingPoint& op);
	~OperatingPoint();

	void* operator new(size_t s);
	void* operator new[](size_t s);
	void operator delete(void *c, size_t s);
	void operator delete[](void *c, size_t s);

	size_t flatten(char *buf, size_t max, size_t offset);

	void add_cluster_id(uint8_t c_id);
	uint8_t get_number_of_clusters();
	uint8_t* get_cluster_id_list();
	uint8_t get_current_cluster_index();

	void add_cluster_guarantee_id(uint8_t cg_id);
	uint8_t get_number_of_cluster_guarantees();
	uint8_t* get_cluster_guarantee_id_list();
	uint8_t get_current_cluster_guarantee_index();

	OperatingPoint& operator=(const OperatingPoint& rhs) = default;

    private:
	uint8_t number_of_clusters;
	uint8_t current_cluster_index;
	uint8_t cluster_id_list[OP_MAX_NUMBER_OF_CLUSTERS];
//        uint8_t *cluster_id_list;

	uint8_t number_of_cluster_guarantees;
	uint8_t current_cluster_guarantee_index;
	uint8_t cluster_guarantee_id_list[OP_MAX_NUMBER_OF_CLUSTER_GUARANTEES];
//        uint8_t *cluster_guarantee_id_list;
};

}/* agent */
}/* os */
#endif // OPERATINGPOINT_H
