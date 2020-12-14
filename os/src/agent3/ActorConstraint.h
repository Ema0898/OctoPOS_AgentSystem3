#ifndef ACTORCONSTRAINT_H
#define ACTORCONSTRAINT_H

#include <stdint.h>
#include <stddef.h>
#include "os/agent3/AbstractConstraint.h"
#include "os/agent3/Cluster.h"
#include "os/agent3/ClusterGuarantee.h"
#include "os/agent3/OperatingPoint.h"
#include "os/agent3/AgentRPCHeader.h"
#include "lib/adt/Bitset.h"

namespace os
{
	namespace agent
	{

		class ActorConstraint : public AbstractConstraint
		{

#define AC_MAX_NUMBER_OF_CLUSTERS 15
#define AC_MAX_NUMBER_OF_CLUSTER_GUARANTEES 15
#define AC_MAX_NUMBER_OF_OPERATING_POINTS 6

		public:
			ActorConstraint();
			ActorConstraint(uint8_t number_of_clusters, uint8_t number_of_cluster_guarantees, uint8_t number_of_operating_points);
			ActorConstraint(const ActorConstraint &ac);
			~ActorConstraint();

			uint8_t add_cluster(uint8_t type);
			uint8_t add_cluster_guarantee(uint8_t first_cluster_id, uint8_t second_cluster_id, uint8_t hopDistance, uint8_t serviceLevel);
			uint8_t add_operating_point(uint8_t number_of_cluster_guarantees, uint8_t number_of_needed_cluster_types);
			void add_cluster_guarantee_to_operating_point(uint8_t op_id, uint8_t cg_id);
			void add_cluster_to_operating_point(uint8_t op_id, uint8_t c_id);

			Cluster *get_cluster_list() const;
			ClusterGuarantee *get_cluster_guarantee_list() const;
			OperatingPoint *get_operating_point_list() const;

			uint8_t get_current_cluster_index() const;
			uint8_t get_current_cluster_guarantee_index() const;
			uint8_t get_current_operating_point_index() const;

			void set_cluster_list(Cluster *cl_list);
			void set_cluster_guarantee_list(ClusterGuarantee *cg_list);
			void set_operating_point_list(OperatingPoint *op_list);

			void *operator new(size_t s);
			void operator delete(void *c, size_t s);

			bool is_tile_usable(const TileID tileID) const;
			bool is_resource_usable(const ResourceID &resource) const;

			virtual bool isAgentConstraint() const;

			//    bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const;

		private:
			lib::adt::Bitset<16> tiles;

			uint8_t number_of_clusters;
			uint8_t number_of_cluster_guarantees;
			uint8_t number_of_operating_points;

			Cluster *cluster_list;
			ClusterGuarantee *cluster_guarantee_list;
			OperatingPoint *operating_point_list;

			uint8_t current_cluster_index;
			uint8_t current_cluster_guarantee_index;
			uint8_t current_operating_point_index;
		};
	} // namespace agent
} // namespace os
#endif // ACTORCONSTRAINT_H
