#ifndef _OCTO_AGENT2_H_
#define _OCTO_AGENT2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "octo_types.h"
#include "octo_proxy_claim.h"
#include <stdbool.h>
    
#define METRICS_NEW 0x01
#define METRICS_DELETE 0x02
#define METRICS_INVADE 0x04
#define METRICS_RETREAT 0x08

/***** TODO: This functions are not implemented yet *****/
agent_t agent_claim_get_agent(agentclaim_t claim);
constraints_t agent_claim_get_constr(agentclaim_t claim);
agentclaim_t agent_claim_invade_with_name(agent_t agent,
                                          constraints_t constr,
                                          const char* agent_name);
int agent_agent_get_name(agentclaim_t claim, char buffer[], size_t size);
//int agent_get_downey_sigma(constraints_t constr);
agentclaim_t agent_claim_invade_or_constraints(agent_t agent,
                                               uint8_t constr_count,
                                               constraints_t constr[]);
agentclaim_t agent_claim_invade_parentclaim(agentclaim_t claim,
                                            constraints_t constr);
agentclaim_t agent_claim_get_initial_with_name(claim_t octoclaim,
                                               const char* agent_name);
int agent_claim_get_operatingpoint_index(agentclaim_t claim);
int agent_claim_get_tileid_iterative(agentclaim_t claim, int iterator);
void agent_constr_overwrite(constraints_t target,
                            constraints_t additional);
//void agent_constr_set_downey_speedup_curve(constraints_t constr, int a, int sigma);
void agent_constr_set_notontile(constraints_t constr, tile_id_t tid);
void agent_constr_set_tile_bitmap(constraints_t constr, uint32_t bitmap);
void agent_constr_set_stickyclaim(constraints_t constr, bool sticky);
void agent_constr_set_vipg(constraints_t constr, uint8_t vipg_enable);
void agent_constr_set_appclass(constraints_t constr, int app_class);
//void agent_constr_set_appnumber(constraints_t constr, int app_nr);
//void agent_constr_set_tile_shareable(constraints_t constr, uint8_t is_tile_shareable);
//void agent_constr_set_malleable(constraints_t constr, bool malleable, resize_handler_t resize_handler, resize_env_t resize_env);
void agent_constr_set_local_memory(constraints_t constr, int min, int max);
void agent_stresstest_agentoctoclaim(void);
agentclaim_t agent_proxy_get_proxyagentoctoclaim(int objects_tile,
                                                 uint32_t octo_ucid);
void agent_proxy_delete_proxyagentoctoclaim(agentclaim_t claim);
int agent_proxy_get_objectstile(agentclaim_t claim);
uint32_t agent_claim_get_ucid(agentclaim_t claim);
bool agent_claim_isempty(agentclaim_t claim);


/***** Deprecated Functions *****/
agentclaim_t __attribute__((deprecated))
agent_claim_actor_constraint_invade(agent_t agent, constraints_t constr);
int __attribute__((deprecated))
agent_claim_actor_constraint_reinvade(agentclaim_t claim, constraints_t constr);
constraints_t __attribute__((deprecated))
agent_actor_constraint_create(uint8_t cluster_size,
                              uint8_t cluster_guarantee_size,
                              uint8_t operating_point_size);
void __attribute__((deprecated))
agent_actor_constraint_delete(constraints_t constr);
uint8_t __attribute__((deprecated))
agent_actor_constraint_add_operating_point(constraints_t constr,
                                           uint8_t cluster_guarantee_size,
                                           uint8_t cluster_size);
uint8_t __attribute__((deprecated))
agent_actor_constraint_add_cluster(constraints_t constr, res_type_t type);
uint8_t __attribute__((deprecated))
agent_actor_constraint_add_cluster_guarantee(constraints_t constr,
                                             uint8_t c1_id,
                                             uint8_t c2_id,
                                             uint8_t hops,
                                             uint8_t service_level);
void __attribute__((deprecated))
agent_actor_constraint_add_cluster_to_operating_point(constraints_t constr,
                                                      uint8_t op_id,
                                                      uint8_t c_id);
void __attribute__((deprecated))
agent_actor_constraint_add_cluster_guarantee_to_operating_point(constraints_t constr,
                                                                uint8_t op_id,
                                                                uint8_t cg_id);


/**
 * \brief Creates a new Agent.
 *
 * This creates a new Agent on the current tile. Please note that new Agents are
 * automatically created when using agent_claim_invade without specifying an
 * agent. (Consequently, you should never have to use this function for now.)
 *
 * \return AgentInstance handle which is required for all subsequent
 *         communications with that agent.
 */
agent_t agent_agent_create();

/**
 * \brief Creates a new Agent.
 *
 * This creates a new Agent on the specified tile.
 *
 * \param tid Tile the agent is created on.
 *
 * \return AgentInstance handle which is required for all subsequent
 *         communications with that agent.
 */
agent_t agent_agent_create_on_tile(tile_id_t tid);

/**
 * \brief Deletes a previously created agent.
 *
 * \param agent AgentInstance handle
 * \param force Expects an empty agent when set to 0, forces deletion otherwise.
 */
void agent_agent_delete(agent_t agent, uint8_t force);

/**
 * \brief Set a visual name for the agent. This name is displayed in the gui.
 *
 * \param agent the agent
 * \param name the name
 */
void agent_agent_set_name(agent_t agent, const char* name);


/***** Invade Interface *****/

/**
 * \brief Invades new resources.
 *
 * This creates a new agent if no agent is specified. Otherwise, the specified
 * agent is used to perform the invade and will be responsible for managing the
 * invaded resources.
 *
 * A failed invade will lead to a NULL pointer being returned and the agent to
 * be deleted, if no other claims are active. Applications must check the return
 * value for validity.
 *
 * \param agent Agent handle of the agent that should perform the invade.
 *              NULL if a new Agent should be created.
 * \param constr Constraints handle, previously created with
 *               agent_constr_create();
 *
 * \return AgentClaim handle. NULL otherwise.
 */
agentclaim_t agent_claim_invade(agent_t agent, constraints_t constr);

/**
 * \brief Reinvades 'claim' with new constraints.
 *
 * Rebuilds a claim with new constraints. If the constraints could not be
 * fulfilled, the old claim is reconstructed.
 *
 * \param claim AgentClaim handle
 * \param constr Constraints handle
 *
 * \returns Absolute sum of changes (i.e., gained and lost resources).
 *          Returns  0 if reinvade resulted in no change to the claim.
 *          Returns -1 if the reinvade was unsuccessful.
 */
int agent_claim_reinvade_constraints(agentclaim_t claim, constraints_t constr);

/**
 * \brief Reinvades a claim with the current constraints.
 *
 * \note This function should never fail. The resources of the claim are reused
 *       and are a valid solution to the constraints by definition.
 *
 * \param claim AgentClaim handle
 *
 * \returns Absolute sum of changes (i.e., gained and lost resources).
 *          Returns 0 if reinvade resulted in no change to the claim.
 */
int agent_claim_reinvade(agentclaim_t claim);

/**
 * \brief Gets an OctoPOS ProxyClaim handle for resources on a specific tile of
 *        a specific type in a claim.
 *
 * \note This function needs to be used to get access to the OctoPOS ProxyClaim
 *       handles to perform actual infects on the previously invaded resources!
 *
 * \note The resource type can either be
 *       0: RISC,
 *       1: iCORE,
 *       2: TCPA or
 *       3: NONE
 *       Currently only RISC is usable.
 *
 * \note ProxyClaims that are acquired with this function are locked in order to
 *       prevent concurrent usage of the same resources. In order to make make
 *       the resources available again call
 *       agent_claim_release_proxyclaim_tile_type.
 *
 * \param claim AgentClaim handle
 * \param tid TileID
 * \param type Resource type (see note)
 *
 * \return OctoPOS ProxyClaim handle
 */
proxy_claim_t agent_claim_get_proxyclaim_tile_type(agentclaim_t claim,
                                                   tile_id_t tid,
                                                   res_type_t type);

/**
 * \brief Releases acquired ProxyClaims
 *
 * \param claim AgentClaim handle
 * \param tid TileID
 * \param type Resource type (see note)
 * \param proxy_claim ProxyClaim handle
 */
void agent_claim_release_proxyclaim_tile_type(agentclaim_t claim,
                                              tile_id_t tid,
                                              res_type_t type,
                                              proxy_claim_t proxy_claim);

/**
 * \brief Retreats all Resources in a claim.
 *
 * \param claim AgentClaim handle
 */
void agent_claim_retreat(agentclaim_t claim);

/**
 * \brief Gets the total number of resources in a claim.
 *
 * \param claim AgentClaim handle
 *
 * \returns Total number of resources in given claim.
 */
int agent_claim_get_pecount(agentclaim_t claim);

/**
 * \brief Gets the total number of resources of a specific type in a claim.
 *
 * \note The resource type can either be
 *       0: RISC,
 *       1: iCORE,
 *       2: TCPA or
 *       3: NONE
 *
 * \param claim AgentClaim handle
 * \param type Resource type (see note)
 *
 * \returns Total number of resources of specific type in given claim.
 */
int agent_claim_get_pecount_type(agentclaim_t claim, res_type_t type);

/**
 * \brief Gets the total number of resources on a specific tile of a specific
 *        type in a claim.
 *
 * \note The resource type can either be
 *       0: RISC,
 *       1: iCORE,
 *       2: TCPA or
 *       3: NONE.
 *
 * \param claim AgentClaim handle
 * \param tile tile id
 * \param type Resource type (see note)
 *
 * \returns Total number of resources on a specific tile of specific type im
 *          given claim.
 */
int agent_claim_get_pecount_tile_type(agentclaim_t claim,
                                      tile_id_t tile,
                                      res_type_t type);

/**
 * \brief Gets the total number of resources on a specific tile in a claim.
 *
 * \param claim AgentClaim handle
 * \param tile tile id
 *
 * \returns total number of resources on specific tile in given claim
 */
int agent_claim_get_pecount_tile(agentclaim_t claim, tile_id_t tile);

/**
 * \brief Gets the total number of tiles in a claim.
 *
 * \param claim AgentClaim handle
 *
 * \returns Total number of different tiles in given claim.
 */
int agent_claim_get_tilecount(agentclaim_t claim);

/**
 * \brief Gets an initial claim on the tile the application is started on
 *
 * \param octoclaim The OctoPOS-claim handle your application received in the
 *                  main-ilet.
 *
 * \return AgentClaim handle
 */
agentclaim_t agent_claim_get_initial(claim_t octoclaim);


/***** Constraints Interface *****/

/**
 * \brief Creates a new set of constraints.
 *
 * \note The constraints handle is only valid on the tile it has been created.
 *       (It is a local pointer!)
 *
 * \return Constraints handle
 */
constraints_t agent_constr_create(void);

/**
 * \brief Deletes a set of constraints.
 *
 * \note Constraints are owned by the application, not iRTSS. The application is
 *       responsible to delete all constraints it creates. Constraints must not
 *       be deleted as long as an agent claim uses it.
 *
 * \param constr Constraints handle that should be deleted.
 *
 */
void agent_constr_delete(constraints_t constr);

/**
 * \brief Sets the PE-Quantity Constraints.
 *
 * \note The resource type can either be
 *       0: RISC,
 *       1: iCORE,
 *       2: TCPA or
 *       3: NONE.
 *
 * \param constr Constraints handle for which the PE-Quantity constraints should
 *               be set.
 * \param min Minimum number of required PEs.
 * \param max Maximum number of useful PEs.
 * \param type PE type of which to set the PE-Quantity constraints. (See note.)
 */
void agent_constr_set_quantity(constraints_t constr,
                               unsigned int min,
                               unsigned int max,
                               res_type_t type);

/**
 * \brief Sets the 'this tile' constraint.
 *
 * Disallows all tiles except of TileID.
 *
 * \param constr Constraints handle for which this constraint should be set.
 * \param tile Resources MUST be located on tile
 */
void agent_constr_set_tile(constraints_t constr, tile_id_t tile);

/**
 * \brief Assigns a reinvade handler which gets called on every reinvade of the
 *        claim which is constrained by the Constraints handle constr.
 *
 * \param constr Constraints handle for which this reinvade handler should be
 *               set.
 * \param reinvade_handler A pointer to the function that will be called on
 *                         reinvades.
 *                         If reinvade_handler_t is NULL (default case), the
 *                         handler will not be called on reinvades.
 *                         Otherwise, the handler will be called on every
 *                         reinvade (until it is set NULL again).
 */
void agent_constr_set_reinvade_handler(constraints_t constr,
                                       reinvade_handler_t reinvade_handler);

/**
 * \brief Returns the current reinvade handler for the Constraints handle
 *        constr.
 *
 * \param constr Constraints handle whose reinvade handler should be returned.
 *
 * \return The reinvade handler for the Constraints constr.
*/
reinvade_handler_t agent_constr_get_reinvade_handler(constraints_t constr);

/**
 * \brief creates an actor constraint
 *
 * \return the created actor constraint
 */
constraints_t agent_actor_constr_create();

/**
 * \brief Add a new Cluster to the Actor Constraint.
 *
 * \param constr the actor constraint we want to add a new cluster to. The
 *               cluster get an ID which correspond to the index position
 *               he was added in the array cluster
 * \param type the type of the cluster which should be added to the constraint
 *
 * \return id of the created cluster
 */
uint8_t agent_actor_constr_add_cluster(constraints_t constr, res_type_t type);

/**
 * \brief Add a new Cluster guarantee to the Actor Constraint.
 *
 * \param constr the actor constraint we want to add a new cluster to. The
 *               cluster guarantee get an ID which correspond to the index
 *               position he was added in the array of cluster guarantees
 * \param c1_id The first of the two clusters we want a guarantee between, the
 *              cluster should belong to the cluster list of the actor
 *              constraint it's added to
 * \param c2_id The second of the two clusters we want a guarantee  between, the
 *              cluster should belong to the cluster list of the actor
 *              constraint it's added to
 * \param hops the maximal hop count we need between the first and the second
 *             clusters
 * \param service_level the minimum service level we need between the first and
 *                      the second clusters
 *
 * \return id of the created cluster guarantee
 */
uint8_t agent_actor_constr_add_cluster_guarantee(constraints_t constr,
                                                 uint8_t c1_id,
                                                 uint8_t c2_id,
                                                 uint8_t hops,
                                                 uint8_t service_level);

/**
 * \brief Add a new operating point to the Actor Constraint.
 *
 * \param constr the actor constraint we want to add a new operating point to.
 *               The cluster guarantee get an ID which correspond to the index
 *               position he was added in the array of cluster guarantees
 *
 * \return id of the created operating point
 */

uint8_t agent_actor_constr_add_operating_point(constraints_t constr);

/**
 * \brief Add a new operating point to the Actor Constraint.The operating point
 *        will get the specified cluster id added in his cluster list
 *
 * \param constr the actor constraint that contains the operating point we want
 *               to add clusters
 * \param op_id id of the operatin point in the actor constraint
 * \param c_id id of the cluster in the actor constraint
 *
 * \return void
 * */
void agent_actor_constr_add_cluster_to_operating_point(constraints_t constr,
                                                       uint8_t op_id,
                                                       uint8_t c_id);

/**
 * \brief Add a new operating point to the Actor Constraint.The operating point
 *        will get the specified cluster guarantee id added in his cluster list
 *
 * \param constr the actor constraint that contains the operating point we want
 *               to add cluster guarantee
 * \param op_id id of the operatin point in the actor constraint
 * \param cg_id id of the cluster guarantee in the actor constraint
 *
 * \return void
 * */
void agent_actor_constr_add_cluster_guarantee_to_operating_point(constraints_t constr,
                                                                 uint8_t op_id,
                                                                 uint8_t cg_id);

/**
* \brief Returns the sigma parameter of Downey's speedup curve model.
*
* This is the getter for the variance of parallelism parameter in Downey's speedup curve
* model, multiplied by 100.
* For more information, look up the setter function
* agent_constr_set_downey_speedup_curve(constraints_t constr, int A, int sigma);
*
* \param constr Constraints handle
* \return Constraints-internal sigma parameter of Downey's speedup curve model.
*/
int agent_get_downey_sigma(constraints_t constr);

/**
* \brief Sets the A and sigma parameters of Downey's speedup curve model.
*
* For a detailed description of the parameter semantics, read Downey's 1997 paper
* "A model for speedup of parallel programs".
*
* Intuitively, A is the upper bound of parallelism the application can exploit.
* Sigma is 0, for applications that scale linearly and goes to infinity for the rest.
* While sigma is a real number around 1.0 in the paper, we use 100 times the value
* as an integer. Thus, a value of 1.0 in the paper corresponds to a sigma value of 100 here.
*
* \param constr Constraints handle for which this constraint should be set.
* \param A      average parallelism
* \param sigma  variance of parallelism * 100
*/
void agent_constr_set_downey_speedup_parameter(constraints_t constr, double A, double sigma);

/**
* \brief Sets the 'tileSharing' constraint.
*
* \note Tile sharing is disabled by default.
*
* \param constr Constraints handle for which this constraint should be set.
* \param is_tile_shareable Turns the constraint off if 0, on otherwise.
*/
void agent_constr_set_tile_shareable(constraints_t constr, uint8_t is_tile_shareable);

/**
* \brief Sets the 'application number' constraint.
*
* \note This is primarily used for the Ethernet-State-Dump interface.
*
* \param constr Constraints handle for which this constraint should be set.
* \param AppNr Application Identifier
*/
void agent_constr_set_appnumber(constraints_t constr, int AppNr);

/**
* \brief Sets the 'malleable' constraint and assigns a resize handler, if applicable.
*
* \note Malleability is disabled by default.
*
* \param constr Constraints handle for which this constraint should be set.
* \param malleable Turns the constraint off if 0, on otherwise.
* \param resize_handler A pointer to the function that will be called on resizes.
*                       Must be non-NULL if malleable_boolean is not 0, otherwise
*                       it is ignored.
* \param resize_env A pointer to random data, application-specific. The agent
*                   system will not access this data. Is ignored if
*                   malleable_boolean is 0.
*/
void agent_constr_set_malleable(constraints_t constr, bool malleable, resize_handler_t resize_handler, resize_env_t resize_env);


/***** Debug Functions *****/
void agent_agent_ping(agent_t agent, int data, int print);
void agent_actor_constr_print_clusters(constraints_t constr);
void agent_actor_constr_print_cluster_guarantees(constraints_t constr);
void agent_actor_constr_print_operating_points(constraints_t constr);
void agent_claim_print(agentclaim_t claim);
void agent_print_proxy_claims(agent_t agent);
void agent_print_system_resources(void);
proxy_claim_t get_proxy_test();

#ifdef cf_agent2_metrics_custom
void print_metrics(uint8_t options);
void enable_metrics();
uint64_t metrics_timer_start();
uint64_t metrics_timer_stop();
void metrics_timer_init();
#endif

#ifdef __cplusplus
}
#endif
#endif /* _OCTO_AGENT2_H_ */
