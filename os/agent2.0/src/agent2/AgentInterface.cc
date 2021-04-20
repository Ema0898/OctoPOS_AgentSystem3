#include "AgentInterface.h"
#include "AgentRPC.h"
#include "os/rpc/RPCStub.h"
#include "CommInterface.h"
#include "config.h"

#ifdef cf_agent2_metrics_custom
#include "AgentMetrics.h"
#endif

#ifdef cf_gui_enabled
#include "os/dev/HWInfo.h"
#include "MetricSender.h"
#include "MetricNewAgent.h"
#include "MetricSystemArchitecture.h"

extern AgentTileManager *LocalAgentTileManager;
static bool initMetric = false;
#endif

using namespace os::agent2;

#ifdef cf_gui_enabled
static inline void _init_metric()
{
	for (TID tid = 0; tid < os::dev::HWInfo::Inst().getTileCount(); tid++)
	{
		CI<Agent> ci = LocalAgentTileManager->get_idle_agent(tid);
		AgentID aid = ci.get_instance_identifier();
		InitMetricMANRPC_DMA::FType fut;
		InitMetricMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid);
		fut.force();
	}
}
#endif

CI<Agent> *AgentInterface::create_agent(TID tid)
{
	Agent *a;
	CI<Agent> *ci = new CI<Agent>;

	if (tid == hw::hal::Tile::getTileID())
	{
		a = new Agent;
		//CI<Agent> new_agent_ci = a->get_CI();
		ci->set_TileID(a->get_CI()->get_TileID());
		ci->set_instance_identifier(a->get_CI()->get_instance_identifier());

#ifdef cf_agent2_metrics_custom
		AgentID aid = ci->get_instance_identifier();
		AgentMetrics::new_agent(aid);
#endif

#ifdef cf_gui_enabled
		if (!initMetric)
		{
			_init_metric();
			initMetric = true;
		}
		MetricNewAgent m(a);
		MetricSender::measureMetric(m);
#endif
	}
	else
	{
		CreateAgentMANRPC_DMA::FType fut;
		CreateAgentMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL);
		fut.force();
		CIReturnEnvelope ret = fut.getData();

		ci->set_TileID(ret.tid);
		ci->set_instance_identifier(ret.aid);
	}
	return ci;
}

void AgentInterface::_delete_agent(CI<Agent> *agent_ci, bool force)
{
	AgentID aid = agent_ci->get_instance_identifier();

#ifdef cf_agent2_metrics_custom
	AgentMetrics::delete_agent(aid);
#endif

	TID tid = agent_ci->get_TileID();
	DeleteAgentMANRPC_DMA::FType fut;
	DeleteAgentMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid, force);
	fut.force();
}

void AgentInterface::delete_agent(CI<Agent> *agent_ci, bool force)
{
	return comminterface::call_no_postfix<void (*)(CI<Agent> *, bool), void>(_delete_agent, agent_ci, force);
}

void AgentInterface::_set_name(CI<Agent> *agent_ci, const char *name)
{
	size_t len = strlen(name);
	SerializationBuffer buf;
	if (len >= buf.size)
	{
		len = buf.size;
	}
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	SerializableAgent::serialize_element<char>(buf, *name, len);
	SetNameMANRPC_DMA::FType fut;
	SetNameMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid, buf);
	fut.force();
}

void AgentInterface::set_name(CI<Agent> *agent_ci, const char *name)
{
	comminterface::call<void (*)(CI<Agent> *, const char *), void>(_set_name, agent_ci, name);
}

TID AgentInterface::_ping_agent(CI<Agent> *agent_ci, int data, bool print)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	PingAgentMANRPC_DMA::FType fut;
	PingAgentMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid, data, print);
	fut.force();
	return fut.getData();
}

TID AgentInterface::ping_agent(CI<Agent> *agent_ci, int data, bool print)
{
	return comminterface::call<TID (*)(CI<Agent> *, int, bool), TID>(_ping_agent, agent_ci, data, print);
}

CI<AgentClaim> *AgentInterface::_invade_agent(CI<Agent> *agent_ci, SerializationBuffer buf)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	InvadeMANRPC_DMA::FType fut;
	InvadeMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, buf, aid);
	fut.force();
	CI<AgentClaim> *ci = new CI<AgentClaim>;
	*ci = fut.getData();

#ifdef cf_agent2_metrics_custom
	ClaimID claim_id = ci->get_instance_identifier();
	AgentMetrics::invade_agent(aid, claim_id);
#endif

	return ci;
}

CI<AgentClaim> *AgentInterface::invade_agent(CI<Agent> *agent_ci, SerializationBuffer buf)
{
	return comminterface::call<CI<AgentClaim> *(*)(CI<Agent> *, SerializationBuffer), CI<AgentClaim> *>(_invade_agent, agent_ci, buf);
}

void AgentInterface::_print_claim(CI<Agent> *agent_ci, ClaimID claim_id)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	PrintClaimMANRPC_DMA::FType fut;
	PrintClaimMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid, claim_id);
	fut.force();
}

void AgentInterface::print_claim(CI<Agent> *agent_ci, ClaimID claim_id)
{
	return comminterface::call<void (*)(CI<Agent> *, ClaimID), void>(_print_claim, agent_ci, claim_id);
}

os::res::ProxyClaim *AgentInterface::get_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type)
{
	return comminterface::call<os::res::ProxyClaim *(*)(CI<Agent> *, ClaimID, TID, ResourceType), os::res::ProxyClaim *>(_get_proxy_claim, agent_ci, claim_id, proxy_tid, res_type);
}

os::res::ProxyClaim *AgentInterface::_get_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID target_tid = agent_ci->get_TileID();
	os::res::ProxyClaim *new_proxy_claim = new os::res::ProxyClaim(0, 0);
	GetProxyClaimMANRPC_DMA::FType fut;
	GetProxyClaimMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(target_tid), &fut, NULL, aid, claim_id, proxy_tid, res_type);
	fut.force();
	SerializationBuffer buf = fut.getData();
	if (buf.index == 0)
	{
		return NULL;
	}
	buf.reset();
	SerializableAgent::deserialize_element<os::res::ProxyClaim>(buf, *new_proxy_claim);
	if (new_proxy_claim->getDispatchInfo().getTID() == UNKNOWN_TID)
	{
		new_proxy_claim = NULL;
	}
	return new_proxy_claim;
}

void AgentInterface::retreat_claim(CI<Agent> *agent_ci, ClaimID claim_id)
{
	return comminterface::call<void (*)(CI<Agent> *, ClaimID), void>(_retreat_claim, agent_ci, claim_id);
}

void AgentInterface::_retreat_claim(CI<Agent> *agent_ci, ClaimID claim_id)
{
	AgentID aid = agent_ci->get_instance_identifier();

#ifdef cf_agent2_metrics_custom
	AgentMetrics::retreat_agent(aid, claim_id);
#endif

	TID tid = agent_ci->get_TileID();
	RetreatClaimMANRPC_DMA::FType fut;
	RetreatClaimMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid, claim_id);
	fut.force();
}

void AgentInterface::return_resources(CI<Agent> *agent_ci, ResourcePool *pool)
{
	return comminterface::call<void (*)(CI<Agent> *, ResourcePool *), void>(_return_resources, agent_ci, pool);
}

void AgentInterface::_return_resources(CI<Agent> *agent_ci, ResourcePool *pool)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	SerializationBuffer buf;
	pool->serialize(buf);

	ReturnResourcesMANRPC_DMA::FType fut;
	ReturnResourcesMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, buf, aid);
	fut.force();
}

void AgentInterface::free_reserved_resources(CI<Agent> *agent_ci, ResourcePool *pool)
{
	return comminterface::call<void (*)(CI<Agent> *, ResourcePool *), void>(_free_reserved_resources, agent_ci, pool);
}

void AgentInterface::_free_reserved_resources(CI<Agent> *agent_ci, ResourcePool *pool)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	SerializationBuffer buf;
	pool->serialize(buf);
	FreeReservedResourcesMANRPC_DMA::FType fut;
	FreeReservedResourcesMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, buf, aid);
	fut.force();
}

pair<int, uint32_t> AgentInterface::reinvade_constraint(CI<Agent> *agent_ci, ClaimID claim_id, Constraint *constr)
{
	return comminterface::call<pair<int, uint32_t> (*)(CI<Agent> *, ClaimID, Constraint *), pair<int, uint32_t>>(_reinvade_constraint, agent_ci, claim_id, constr);
}

pair<int, uint32_t> AgentInterface::_reinvade_constraint(CI<Agent> *agent_ci, ClaimID claim_id, Constraint *constr)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID tid = agent_ci->get_TileID();
	SerializationBuffer buf;
	if (constr != NULL)
	{
		constr->serialize(buf);
	}
	ReinvadeMANRPC_DMA::FType fut;
	ReinvadeMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, buf, aid, claim_id);
	fut.force();
	pair<int, uint32_t> ret_val = fut.getData();
	return ret_val;
}

int AgentInterface::get_pecount(CI<Agent> *agent_ci, ClaimID claim_id, ResourceType res_type, TID tid)
{
	return comminterface::call<int (*)(CI<Agent> *, ClaimID, ResourceType, TID), int>(_get_pecount, agent_ci, claim_id, res_type, tid);
}

int AgentInterface::_get_pecount(CI<Agent> *agent_ci, ClaimID claim_id, ResourceType res_type, TID tid)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID target_tid = agent_ci->get_TileID();
	GetPECountMANRPC_DMA::FType fut;
	GetPECountMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(target_tid), &fut, NULL, aid, claim_id, res_type, tid);
	fut.force();
	return fut.getData();
}

int AgentInterface::get_tile_count(CI<Agent> *agent_ci, ClaimID claim_id)
{
	return comminterface::call<int (*)(CI<Agent> *, ClaimID), int>(_get_tile_count, agent_ci, claim_id);
}

int AgentInterface::_get_tile_count(CI<Agent> *agent_ci, ClaimID claim_id)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID target_tid = agent_ci->get_TileID();
	GetTileCountMANRPC_DMA::FType fut;
	GetTileCountMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(target_tid), &fut, NULL, aid, claim_id);
	fut.force();
	return fut.getData();
}

void AgentInterface::print_proxy_claims(CI<Agent> *agent_ci)
{
	return comminterface::call<void (*)(CI<Agent> *), void>(_print_proxy_claims, agent_ci);
}

void AgentInterface::_print_proxy_claims(CI<Agent> *agent_ci)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID target_tid = agent_ci->get_TileID();
	PrintProxyClaimsMANRPC_DMA::FType fut;
	PrintProxyClaimsMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(target_tid), &fut, NULL, aid);
	fut.force();
}

void AgentInterface::release_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type)
{
	return comminterface::call<void (*)(CI<Agent> *, ClaimID, TID, ResourceType), void>(_release_proxy_claim, agent_ci, claim_id, proxy_tid, res_type);
}

void AgentInterface::_release_proxy_claim(CI<Agent> *agent_ci, ClaimID claim_id, TID proxy_tid, ResourceType res_type)
{
	AgentID aid = agent_ci->get_instance_identifier();
	TID target_tid = agent_ci->get_TileID();
	ReleaseProxyClaimMANRPC_DMA::FType fut;
	ReleaseProxyClaimMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(target_tid), &fut, NULL, aid, claim_id, proxy_tid, res_type);
	fut.force();
}

#ifdef cf_agent2_metrics_custom

void AgentInterface::enable_metrics_interface()
{
	AgentMetrics::enable_metrics();
}

void AgentInterface::metrics_timer_init_interface()
{
	AgentMetrics::metrics_timer_init();
}

uint64_t AgentInterface::metrics_timer_start_interface()
{
	return AgentMetrics::metrics_timer_start();
}

uint64_t AgentInterface::metrics_timer_stop_interface()
{
	return AgentMetrics::metrics_timer_stop();
}

void AgentInterface::general_metrics_interface(uint8_t options)
{
	AgentMetrics::general_metrics(options);
}

void AgentInterface::cluster_metrics_interface()
{
	AgentMetrics::cluster_metrics();
}

void AgentInterface::timer_metrics_interface()
{
	AgentMetrics::timer_metrics();
}

void AgentInterface::claim_metrics_interface(CI<Agent> *agent_ci, ClaimID claim_id)
{
	AgentID aid = agent_ci->get_instance_identifier();
	Agent *ag = LocalAgentTileManager->get_agent(aid);

	AgentMetrics::claim_metrics(*ag, claim_id);
}

void AgentInterface::all_metrics_interface(CI<Agent> *agent_ci, ClaimID claim_id, uint8_t options)
{
	AgentID aid = agent_ci->get_instance_identifier();
	Agent *ag = LocalAgentTileManager->get_agent(aid);

	AgentMetrics::print_metrics(*ag, claim_id, options);
}

#endif

// bool AgentInterface::_migrate_agent(CI<Agent> *agent_ci, TID dest){
// 	AgentID aid = agent_ci->get_instance_identifier();
// 	TID tid = agent_ci->get_TileID();
// 	RemoteMigrateMANRPC_DMA::FType fut;
// 	RemoteMigrateMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(tid), &fut, NULL, aid, dest);
// 	fut.force();
// 	return fut.getData();
// }

// bool AgentInterface::migrate_agent(CI<Agent> *agent_ci, TID dest){
// 	return comminterface::call_no_postfix<bool(*)(CI<Agent> *agent_ci, TID), bool>(_migrate_agent, agent_ci, dest);
// }