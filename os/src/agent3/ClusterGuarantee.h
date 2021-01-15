#ifndef CLUSTERGUARANTEE_H
#define CLUSTERGUARANTEE_H
#include "os/agent3/Cluster.h"

#include <stdint.h>
#include <stddef.h>

namespace os
{
	namespace agent
	{

		class ClusterGuarantee
		{
			friend class FlatClusterGuarantee;

		private:
			/* Specified characteristics to be satistified in a cluster pair */
			Cluster first_cluster;
			Cluster second_cluster;
			uint8_t hop_distance;
			uint16_t service_level;

		public:
			ClusterGuarantee();
			ClusterGuarantee(Cluster first_cluster, Cluster second_cluster, uint8_t hop_distance, uint16_t service_level);
			ClusterGuarantee(const ClusterGuarantee &obj);
			~ClusterGuarantee();

			size_t flatten(char *buffer, size_t maxSize, size_t offset);

			Cluster get_first_cluster();
			Cluster get_second_cluster();
			void setHopDistance(uint8_t hop_distance);
			uint8_t getHopDistance();
			void setServiceLevel(uint16_t service_level);
			uint16_t getServiceLevel();
			void *operator new(size_t s);
			void *operator new[](size_t s);
			void operator delete(void *c, size_t s);
			void operator delete[](void *c, size_t s);
			ClusterGuarantee &operator=(const ClusterGuarantee &rhs) = default;
		};

	} // namespace agent
} // namespace os
#endif // CLUSTERGUARANTEE_H
