#ifndef OS_AGENT2_AGENTINTERFACE_H
#define OS_AGENT2_AGENTINTERFACE_H

#include "CI.h"
#include "Agent2.h"
#include "AgentClaim.h"
#include "AgentSerialization.h"
#include "Constraint.h"
#include "os/res/ProxyClaim.h"
#include "agent2_lib.h"

namespace os
{
	namespace agent2
	{

		class AgentInterface
		{
		public:
			static CI<Agent> *create_agent(TID tid);

			static void delete_agent(CI<Agent> *agent_ci, bool force = false);

			static CI<AgentClaim> *invade_agent(CI<Agent> *agent_ci, SerializationBuffer buf);
			//	static bool migrate_agent(CI<Agent> *agent_ci, TID dest);

			static void set_name(CI<Agent> *agent_ci, const char *name);

			static TID ping_agent(CI<Agent> *agent_ci, int data, bool print);

			static void print_claim(CI<Agent> *agent_ci, ClaimID claim_id);

			static os::res::ProxyClaim *get_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type);

			static void release_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type);

			static void retreat_claim(CI<Agent> *agent_ci, ClaimID claim_id);

			static void return_resources(CI<Agent> *agent_ci, ResourcePool *pool);

			static void free_reserved_resources(CI<Agent> *agent_ci, ResourcePool *pool);

			static pair<int, uint32_t> reinvade_constraint(CI<Agent> *agent_ci, ClaimID claim_id, Constraint *constr);

			static pair<int, uint32_t> reinvade(CI<Agent> *agent_ci, ClaimID claim_id);

			static int get_pecount(CI<Agent> *agent_ci, ClaimID claim_id, ResourceType res_type, TID tid);

			static int get_tile_count(CI<Agent> *agent_ci, ClaimID claim_id);

			static void print_proxy_claims(CI<Agent> *agent_ci);

			#ifdef cf_agent2_metrics_custom

			static void print_metrics_interface(uint8_t options);

			static void enable_metrics_interface();

			static uint64_t metrics_timer_start_interface();

			static uint64_t metrics_timer_stop_interface();

			static void metrics_timer_init_interface();

			#endif

		private:
			static void _print_claim(CI<Agent> *agent_ci, ClaimID claim_id);

			static CI<AgentClaim> *_invade_agent(CI<Agent> *agent_ci, SerializationBuffer buf);

			static void _delete_agent(CI<Agent> *agent_ci, bool force = false);

			static void _set_name(CI<Agent> *agent_ci, const char *name);

			static TID _ping_agent(CI<Agent> *agent_ci, int data, bool print);

			static os::res::ProxyClaim *_get_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type);

			static void _release_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type);

			static void _retreat_claim(CI<Agent> *agent_ci, ClaimID claim_id);

			static void _return_resources(CI<Agent> *agent_ci, ResourcePool *pool);

			static void _free_reserved_resources(CI<Agent> *agent_ci, ResourcePool *pool);

			static pair<int, uint32_t> _reinvade_constraint(CI<Agent> *agent_ci, ClaimID claim_id, Constraint *constr);

			static pair<int, uint32_t> _reinvade(CI<Agent> *agent_ci, ClaimID claim_id);

			static int _get_pecount(CI<Agent> *agent_ci, ClaimID claim_id, ResourceType res_type, TID tid);

			static int _get_tile_count(CI<Agent> *agent_ci, ClaimID claim_id);

			static void _print_proxy_claims(CI<Agent> *agent_ci);

			//	static bool _migrate_agent(CI<Agent> *agent_ci, TID dest);
		};

	} //agent2
} //os

#endif // OS_AGENT2_AGENTINTERFACE_H
