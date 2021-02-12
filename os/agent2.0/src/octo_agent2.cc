#include "octo_agent.h"
#include "octo_agent2.h"

#include "os/res/ProxyClaim.h"

#include "os/agent2/ActorConstraint.h"
#include "os/agent2/AgentClaim.h"
#include "os/agent2/AgentConstraint.h"
#include "os/agent2/AgentInterface.h"
#include "os/agent2/AgentRPC.h"
#include "os/agent2/AgentSerialization.h"
//#include "os/agent2/AgentMetrics.h"

using namespace os::agent2;

extern AgentTileManager *LocalAgentTileManager;

constraints_t agent_claim_get_constr(agentclaim_t claim)
{
	// TODO: Implement me
	printf("agent_claim_get_constr is not implemented yet\n");
	return NULL;
}

agent_t agent_claim_get_agent(agentclaim_t claim)
{
	// TODO: Implement me
	printf("agent_claim_get_agent is not implemented yet\n");
	return NULL;
}

agentclaim_t agent_claim_invade_with_name(agent_t parentagent, constraints_t constr, const char *agent_name)
{
	// TODO: Implement me
	printf("agent_claim_invade_with_name is not implemented yet\n");
	return NULL;
}

int agent_agent_get_name(agentclaim_t claim, char buffer[], size_t size)
{
	// TODO: Implement me
	printf("agent_agent_get_name is not implemented yet\n");
	return 0;
}

agentclaim_t agent_claim_invade_or_constraints(agent_t parentagent, uint8_t constr_count, constraints_t constr[])
{
	// TODO: Implement me
	printf("agent_claim_invade_or_constraints is not implemented yet\n");
	return NULL;
}

agentclaim_t agent_claim_invade_parentclaim(agentclaim_t parentclaim, constraints_t constr)
{
	// TODO: Implement me
	printf("agent_claim_invade_parentclaim is not implemented yet\n");
	return NULL;
}

agentclaim_t agent_claim_get_initial_with_name(claim_t octoclaim, const char *agent_name)
{
	// TODO: Implement me
	printf("agent_claim_get_initial_with_name is not implemented yet\n");
	return NULL;
}

int agent_claim_get_operatingpoint_index(agentclaim_t claim)
{
	// TODO: Implement me
	printf("agent_claim_get_operatingpoint_index  is not implemented yet\n");
	return 0;
}

int agent_claim_get_tileid_iterative(agentclaim_t claim, int iterator)
{
	// TODO: Implement me
	printf("agent_claim_get_tileid_iterative is not implemented yet\n");
	return 0;
}

void agent_constr_overwrite(constraints_t constrTarget, constraints_t additionalConstraints)
{
	// TODO: Implement me
	printf("agent_constr_overwrite is not implemented yet\n");
}

void agent_constr_set_notontile(constraints_t constr, tile_id_t TileID)
{
	// TODO: Implement me
	printf("agent_constr_set_notontile is not implemented yet\n");
}

void agent_constr_set_tile_bitmap(constraints_t constr, uint32_t bitmap)
{
	// TODO: Implement me
	printf("agent_constr_set_tile_bitmap is not implemented yet\n");
}

void agent_constr_set_stickyclaim(constraints_t constr, bool sticky)
{
	// TODO: Implement me
	printf("agent_constr_set_stickyclaim is not implemented yet\n");
}

void agent_constr_set_vipg(constraints_t constr, uint8_t vipgEnable)
{
	// TODO: Implement me
	printf("agent_constr_set_vipg is not implemented yet\n");
}

void agent_constr_set_appclass(constraints_t constr, int AppClass)
{
	// TODO: Implement me
	printf("agent_constr_set_appclass is not implemented yet\n");
}

void agent_constr_set_local_memory(constraints_t constr, int min, int max)
{
	// TODO: Implement me
	printf("agent_constr_set_local_memory is not implemented yet\n");
}

void agent_stresstest_agentoctoclaim(void)
{
	// TODO: Implement me
	printf("agent_stresstest_agentoctoclaim is not implemented yet\n");
}

agentclaim_t agent_proxy_get_proxyagentoctoclaim(int objects_tile, uint32_t octo_ucid)
{
	// TODO: Implement me
	printf("agent_proxy_get_proxyagentoctoclaim is not implemented yet\n");
	return NULL;
}

void agent_proxy_delete_proxyagentoctoclaim(agentclaim_t proxy_agentoctoclaim)
{
	// TODO: Implement me
	printf("agent_proxy_delete_proxyagentoctoclaim is not implemented yet\n");
}

int agent_proxy_get_objectstile(agentclaim_t proxy_agentoctoclaim)
{
	// TODO: Implement me
	printf("agent_proxy_get_objectstile is not implemented yet\n");
	return 0;
}

uint32_t agent_claim_get_ucid(agentclaim_t claim)
{
	// TODO: Implement me
	printf("agent_claim_get_ucid is not implemented yet\n");
	return 0;
}

bool agent_claim_isempty(agentclaim_t claim)
{
	// TODO: Implement me
	printf("agent_claim_isempty is not implemented yet\n");
	return true;
}

agentclaim_t agent_claim_actor_constraint_invade(agent_t agent, constraints_t constr)
{
	printf("WARNING: Deprecated function agent_claim_actor_constraint_invade. Use agent_claim_invade instead.\n");
	return agent_claim_invade(agent, constr);
}

int agent_claim_actor_constraint_reinvade(agentclaim_t claim, constraints_t constr)
{
	printf("WARNING: Deprecated function agent_claim_actor_constraint_reinvade. Use agent_claim_reinvade_constraints instead.\n");
	return agent_claim_reinvade_constraints(claim, constr);
}

constraints_t agent_actor_constraint_create(uint8_t number_of_clusters,
											uint8_t number_of_cluster_guarantees,
											uint8_t number_of_operating_points)
{
	printf("WARNING: Deprecated function agent_actor_constraint_create. Use agent_actor_constr_create instead.\n");
	return agent_actor_constr_create();
}

void agent_actor_constraint_delete(constraints_t constr)
{
	printf("WARNING: Deprecated function agent_actor_constraint_delete. Use agent_constr_delete instead.\n");
	agent_constr_delete(constr);
}

uint8_t agent_actor_constraint_add_operating_point(constraints_t constr,
												   uint8_t number_of_cluster_guarantees,
												   uint8_t number_of_clusters)
{
	printf("WARNING: Deprecated function agent_actor_constraint_add_operating_point. Use agent_actor_constr_add_operating_point instead.\n");
	return agent_actor_constr_add_operating_point(constr);
}

uint8_t agent_actor_constraint_add_cluster(constraints_t constr, res_type_t type)
{
	printf("WARNING: Deprecated function agent_actor_constraint_add_cluster. Use agent_actor_constr_add_cluster instead.\n");
	return agent_actor_constr_add_cluster(constr, type);
}

uint8_t agent_actor_constraint_add_cluster_guarantee(constraints_t constr, uint8_t c1_id, uint8_t c2_id, uint8_t hops, uint8_t serviceLevel)
{
	printf("WARNING: Deprecated function agent_actor_constraint_add_cluster_guarantee. Use agent_actor_constr_add_cluster_guarantee instead.\n");
	return agent_actor_constr_add_cluster_guarantee(constr, c1_id, c2_id, hops, serviceLevel);
}

void agent_actor_constraint_add_cluster_to_operating_point(constraints_t constr, uint8_t op_id, uint8_t c_id)
{
	printf("WARNING: Deprecated function agent_actor_constraint_add_cluster_to_operating_point. Use agent_actor_constr_add_cluster_to_operating_point instead.\n");
	agent_actor_constr_add_cluster_to_operating_point(constr, op_id, c_id);
}

void agent_actor_constraint_add_cluster_guarantee_to_operating_point(constraints_t constr, uint8_t op_id, uint8_t cg_id)
{
	printf("WARNING: Deprecated function agent_actor_constraint_add_cluster_guarantee_to_operating_point. Use agent_actor_constr_add_cluster_guarantee_to_operating_point instead.\n");
	agent_actor_constr_add_cluster_guarantee_to_operating_point(constr, op_id, cg_id);
}

int agent_get_downey_sigma(constraints_t constr)
{
	if (!constr)
		panic("agent_get_downey_sigma: Constraints == NULL");
	AgentConstraint *Constrainte = NULL;
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
		panic("This Constrainte is not a AgentConstraint");
	else
		Constrainte = static_cast<AgentConstraint *>(constr);
	return Constrainte->dsu.get_Sigma();
}

void agent_constr_set_downey_speedup_parameter(constraints_t constr, double average, double sigma)
{
	if (!constr)
		panic("agent_constr_set_quantity Constraints == NULL");
	if (average <= 0)
		panic("Average too small");
	if (average > SHRT_MAX)
		panic("Average bigger than SHRT_MAX");
	if (sigma <= 0)
		panic("sigma too small");
	if (sigma > SHRT_MAX)
		panic("sigma bigger than SHRT_MAX");

	AgentConstraint *Constrainte = NULL;
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
		panic("This Constrainte is not a AgentConstraint");
	else
		Constrainte = static_cast<AgentConstraint *>(constr);

	Constrainte->dsu.set_Average(static_cast<unsigned int>(average * 100));
	Constrainte->dsu.set_Sigma(static_cast<unsigned int>(sigma * 100));
	Constrainte->dsu.set_Downey(true);
}

void agent_constr_set_appnumber(constraints_t constr, int AppNr)
{
	if (!constr)
		panic("Constraints == NULL");
	AgentConstraint *Constrainte = NULL;
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
		panic("This Constrainte is not a AgentConstraint");
	else
		Constrainte = static_cast<AgentConstraint *>(constr);
	Constrainte->appnum.setAppNumber(AppNr);
}

void agent_constr_set_tile_shareable(constraints_t constr, uint8_t is_tile_shareable)
{
	if (!constr)
		panic("Constraints == NULL");
	AgentConstraint *Constrainte = NULL;
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
		panic("This Constrainte is not a AgentConstraint");
	else
		Constrainte = static_cast<AgentConstraint *>(constr);
	Constrainte->tishar.setShareable(is_tile_shareable != 0);
}

void agent_constr_set_malleable(constraints_t constr, bool malleable, resize_handler_t resize_handler, resize_env_t resize_env)
{
	if (!constr)
		panic("agent_constr_set_malleable: Constraints == NULL");

	AgentConstraint *Constrainte = NULL;
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
		panic("This Constrainte is not a AgentConstraint");
	else
		Constrainte = static_cast<AgentConstraint *>(constr);

	if (malleable)
	{
		Constrainte->flexible.set_Malleable(true);
		Constrainte->flexible.set_Resize_Handler(resize_handler);
		Constrainte->flexible.set_Resize_Env_Ptr(resize_env);
	}
	else
	{
		Constrainte->flexible.set_Malleable(false);
	}
}

agent_t agent_agent_create()
{
	return agent_agent_create_on_tile(hw::hal::Tile::getTileID());
}

agent_t agent_agent_create_on_tile(tile_id_t tid)
{
	CI<Agent> *ci = AgentInterface::create_agent(static_cast<TID>(tid));
	return static_cast<agent_t>(ci);
}

void agent_agent_delete(agent_t agent, uint8_t force)
{
	AgentInterface::delete_agent(static_cast<CI<Agent> *>(agent));
}

void agent_agent_ping(agent_t agent, int data, int print)
{
	CI<Agent> *agent_ci = static_cast<CI<Agent> *>(agent);
	AgentInterface::ping_agent(agent_ci, data, static_cast<bool>(print));
}

void agent_agent_set_name(agent_t agent, const char *name)
{
	AgentInterface::set_name(static_cast<CI<Agent> *>(agent), name);
}

constraints_t agent_actor_constr_create()
{
	ActorConstraint *ac = new ActorConstraint();
	return ac;
}

uint8_t agent_actor_constr_add_cluster(constraints_t constr, res_type_t type)
{
	if (!constr)
	{
		panic("agent_actor_constr_add_cluster: ActorConstraint* == NULL");
	}

	return static_cast<ActorConstraint *>(constr)->addCluster(static_cast<ResourceType>(type));
}

uint8_t agent_actor_constr_add_cluster_guarantee(constraints_t constr, uint8_t c1_id, uint8_t c2_id,
												 uint8_t hops, uint8_t serviceLevel)
{
	if (!constr)
	{
		panic("agent_actor_constr_add_cluster_guarantee: ActorConstraint* == NULL");
	}

	return static_cast<ActorConstraint *>(constr)->addClusterGuarantee(c1_id, c2_id, hops, serviceLevel);
}

uint8_t agent_actor_constr_add_operating_point(constraints_t constr)
{
	if (!constr)
	{
		panic("agent_actor_constr_add_oper_point: ActorConstraint* == NULL");
	}

	return static_cast<ActorConstraint *>(constr)->addOperatingPoint();
}

void agent_actor_constr_add_cluster_to_operating_point(constraints_t constr, uint8_t op_id, uint8_t c_id)
{
	if (!constr)
	{
		panic("agent_actor_constr_add_cluster_guarantee_to_operating_point: ActorConstraint* == NULL");
	}

	static_cast<ActorConstraint *>(constr)->addClusterToOperatingPoint(op_id, c_id);
}

void agent_actor_constr_add_cluster_guarantee_to_operating_point(constraints_t constr,
																 uint8_t op_id,
																 uint8_t cg_id)
{
	if (!constr)
	{
		panic("agent_actor_constr_add_cluster_guarantee_to_operating_point: ActorConstraint* == NULL");
	}

	static_cast<ActorConstraint *>(constr)->addClusterGuaranteeToOperatingPoint(op_id, cg_id);
}

void agent_actor_constr_print_clusters(constraints_t constr)
{
	if (!constr)
	{
		panic("agent_actor_constr_print_clusters: ActorConstraint* == NULL");
	}

	static_cast<ActorConstraint *>(constr)->printClusters();
}

void agent_actor_constr_print_cluster_guarantees(constraints_t constr)
{
	if (!constr)
	{
		panic("agent_actor_constr_print_cluster_guarantees: ActorConstraint* == NULL");
	}

	static_cast<ActorConstraint *>(constr)->printClusterGuarantees();
}

void agent_actor_constr_print_operating_points(constraints_t constr)
{
	if (!constr)
	{
		panic("agent_actor_constr_print_operating_points: ActorConstraint* == NULL");
	}

	static_cast<ActorConstraint *>(constr)->printOperatingPoints();
}

agentclaim_t agent_claim_invade(agent_t agent, constraints_t constr)
{
	if (agent == NULL)
	{
		agent = agent_agent_create();
	}
	SerializationBuffer buf;
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
	{
		static_cast<ActorConstraint *>(constr)->serialize(buf);
	}
	else
	{
		static_cast<AgentConstraint *>(constr)->serialize(buf);
	}
	CI<AgentClaim> *ci = AgentInterface::invade_agent(static_cast<CI<Agent> *>(agent), buf);
	if (ci->get_instance_identifier() == INVALID_AGENTCLAIM)
	{
		delete (ci);
		return NULL;
	}

	return static_cast<agentclaim_t>(ci);
}

void agent_claim_retreat(agentclaim_t claim)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID claim_id = ci->get_instance_identifier();
	AgentInterface::retreat_claim(cia, claim_id);
}

void agent_claim_print(agentclaim_t claim)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID cic = ci->get_instance_identifier();
	AgentInterface::print_claim(cia, cic);
}

constraints_t agent_constr_create(void)
{
	AgentConstraint *constr = new AgentConstraint;
	return static_cast<constraints_t>(constr);
}

void agent_constr_delete(constraints_t constr)
{
	if (static_cast<Constraint *>(constr)->getType() == ConstraintType::Actor)
	{
		delete (static_cast<ActorConstraint *>(constr));
	}
	else
	{
		delete (static_cast<AgentConstraint *>(constr));
	}
}

void agent_constr_set_quantity(constraints_t constr, unsigned int min, unsigned int max, res_type_t type)
{
	AgentConstraint *Constr = static_cast<AgentConstraint *>(constr);
	Constr->set_PE_quantity(static_cast<ResourceType>(type), min, max);
	Constr->set_type_res(static_cast<ResourceType>(type));
}

void agent_print_system_resources(void)
{
	printf("========== System Resources ==========\n");
	for (unsigned int i = 0; i < hw::hal::Tile::getTileCount(); i++)
	{
		PrintSystemResourcesMANRPC_DMA::FType fut;
		PrintSystemResourcesMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(i), &fut, NULL, 0);
		fut.force();
	}
	printf("========================================\n");
}

proxy_claim_t get_proxy_test()
{
	TestProxySendMANRPC_DMA::FType fut;
	TestProxySendMANRPC_DMA::stub(os::res::SystemClaim::Inst().getDispatchClaim(1), &fut, NULL);
	fut.force();
	os::res::ProxyClaim *proxy = new os::res::ProxyClaim(0, 0);
	SerializationBuffer buf = fut.getData();
	buf.reset();
	SerializableAgent::deserialize_element<os::res::ProxyClaim>(buf, *proxy);
	return static_cast<proxy_claim_t>(proxy);
}

proxy_claim_t agent_claim_get_proxyclaim_tile_type(agentclaim_t claim, tile_id_t tid, res_type_t type)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID cic = ci->get_instance_identifier();
	os::res::ProxyClaim *new_proxy_claim = AgentInterface::get_proxy_claim(cia, cic, tid, static_cast<ResourceType>(type));
	return static_cast<proxy_claim_t>(new_proxy_claim);
}

int agent_claim_reinvade(agentclaim_t claim)
{
	return agent_claim_reinvade_constraints(claim, NULL);
}

int agent_claim_reinvade_constraints(agentclaim_t claim, constraints_t constr)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID claim_id = ci->get_instance_identifier();
	pair<int, uint32_t> result = AgentInterface::reinvade_constraint(cia, claim_id, static_cast<Constraint *>(constr));
	int ret_val = result.first;
	uint32_t reinvade_handler_id = result.second;

	if (reinvade_handler_id != INVALID_CONSTRAINT)
	{
		reinvade_handler_ptr reinvade_handler = LocalAgentTileManager->get_reinvade_handler(reinvade_handler_id);
		if (reinvade_handler)
		{
			reinvade_handler();
		}
	}

	return ret_val;
}

void agent_constr_set_reinvade_handler(constraints_t constr, reinvade_handler_t reinvade_handler)
{
	LocalAgentTileManager->set_reinvade_handler(static_cast<Constraint *>(constr)->get_ucid(), reinvade_handler);
}

reinvade_handler_t agent_constr_get_reinvade_handler(constraints_t constr)
{
	uint32_t ucid = static_cast<Constraint *>(constr)->get_ucid();
	return static_cast<reinvade_handler_t>(LocalAgentTileManager->get_reinvade_handler(ucid));
}

int agent_claim_get_pecount(agentclaim_t claim)
{
	return agent_claim_get_pecount_type(claim, static_cast<res_type_t>(ResourceType::TYPE_ALL));
}

int agent_claim_get_pecount_type(agentclaim_t claim, res_type_t type)
{
	return agent_claim_get_pecount_tile_type(claim, UNKNOWN_TID, static_cast<res_type_t>(type));
}

int agent_claim_get_pecount_tile_type(agentclaim_t claim, tile_id_t tile, res_type_t type)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID claim_id = ci->get_instance_identifier();

	return AgentInterface::get_pecount(cia, claim_id, static_cast<ResourceType>(type), static_cast<TID>(tile));
}

int agent_claim_get_pecount_tile(agentclaim_t claim, tile_id_t tile)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID claim_id = ci->get_instance_identifier();

	return AgentInterface::get_pecount(cia, claim_id, ResourceType::TYPE_ALL, static_cast<TID>(tile));
}

int agent_claim_get_tilecount(agentclaim_t claim)
{
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID claim_id = ci->get_instance_identifier();

	return AgentInterface::get_tile_count(cia, claim_id);
}

void agent_print_proxy_claims(agent_t agent)
{
	CI<Agent> *agent_ci = static_cast<CI<Agent> *>(agent);
	AgentInterface::print_proxy_claims(agent_ci);
}

void agent_claim_release_proxyclaim_tile_type(agentclaim_t claim, tile_id_t tid, res_type_t type, proxy_claim_t proxy_claim)
{
	if (!proxy_claim)
	{
		return;
	}
	delete (static_cast<os::res::ProxyClaim *>(proxy_claim));
	CI<AgentClaim> *ci = static_cast<CI<AgentClaim> *>(claim);
	CI<Agent> *cia = ci->get_agent_ci();
	ClaimID cic = ci->get_instance_identifier();
	AgentInterface::release_proxy_claim(cia, cic, tid, static_cast<ResourceType>(type));
	proxy_claim = NULL;
}

void agent_constr_set_tile(constraints_t constr, tile_id_t tile)
{
	static_cast<AgentConstraint *>(constr)->set_tile(static_cast<TID>(tile));
}

agentclaim_t agent_claim_get_initial(claim_t octoclaim)
{
	constraints_t constr = agent_constr_create();
	agent_constr_set_quantity(constr, 1, 1, 0);
	agent_constr_set_tile(constr, hw::hal::Tile::getTileID());
	return agent_claim_invade(NULL, constr);
}

void print_metrics()
{
	AgentInterface::print_metrics_interface();
}