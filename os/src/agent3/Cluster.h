#ifndef CLUSTER_H
#define CLUSTER_H

#include <stdint.h>
#include <stddef.h>

namespace os {
namespace agent{
#define TILE_RISC_ID 1
#define TILE_ICORE_ID 2
#define TILE_TCPA_ID 3
class Cluster
{
    friend class FlatCluster;

    private:
	uint8_t type;
	uint8_t id;

    public:
	Cluster();
	Cluster(uint8_t type, uint8_t id);
	Cluster(const Cluster &obj);
	~Cluster();

	size_t flatten(char *buf, size_t max, size_t offset);

	uint8_t get_type();
	uint8_t get_id();

	void* operator new(size_t s);
	void* operator new[](size_t s);
	void operator delete(void *c, size_t s);
	void operator delete[](void *c, size_t s);

	Cluster& operator=(const Cluster& rhs) = default;
};

}/* agent */
}/* os */

#endif // CLUSTER_H
