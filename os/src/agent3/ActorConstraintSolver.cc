#include "os/agent3/ActorConstraintSolver.h"
#include "os/agent3/ActorConstraint.h"

os::agent::ActorConstraintSolver::ActorConstraintSolver()
{
}

os::agent::ActorConstraintSolver::~ActorConstraintSolver()
{
}

std::pair<bool, os::agent::AgentClaim> os::agent::ActorConstraintSolver::solve(const ActorConstraint &constr, const AgentClaim &availableResources)
{
    DBG(SUB_AGENT, "SOLVE(...)\n");

    bool ret = false;
    AgentClaim actor_claim;

    DBG(SUB_AGENT, "Available Resources in claim\n");
    availableResources.print();

    for (uint8_t i = 0; i < constr.get_current_operating_point_index(); i++)
    {
        std::pair<bool, os::agent::AgentClaim> result = solve_operating_point(constr, availableResources, i);

        if (result.first)
        {
            ret = result.first;
            actor_claim.addClaim(result.second);
            DBG(SUB_AGENT, "Operating Point %d SOLVED! Claim:\n", i);
            actor_claim.setOperatingPointIndex(i);
            actor_claim.print();
            break;
        }
        else
            DBG(SUB_AGENT, "Operating Point %d UNSOLVED\n\n", i);
    }

    DBG(SUB_AGENT, "CONSTRAINT SOLVER(...) ==> END\n");
    return std::make_pair(ret, actor_claim);
}

std::pair<bool, os::agent::AgentClaim> os::agent::ActorConstraintSolver::solve_operating_point(const ActorConstraint &actor_constraint, const AgentClaim &available_resources, const uint8_t &operating_point_id)
{
    DBG(SUB_AGENT, "solve_operating_point #%d\n", operating_point_id);
    uint8_t op_cluster_guarantee_count = actor_constraint.get_operating_point_list()[operating_point_id].get_current_cluster_guarantee_index();
    uint8_t op_cluster_count = actor_constraint.get_operating_point_list()[operating_point_id].get_current_cluster_index();
    CGSolutionMap solution;
    TileMap utilizable_tiles;
    AgentClaim op_claim;

    /* Initialize the solution map */
    for (uint8_t op_cg_id = 0; op_cg_id < op_cluster_guarantee_count; op_cg_id++)
    {
        solution.map[op_cg_id].is_active = true;
        solution.map[op_cg_id].is_solved = false;
        solution.map[op_cg_id].op_local_cg_id = op_cg_id;
        solution.map[op_cg_id].op_global_cg_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_guarantee_id_list()[op_cg_id];
    }
    /* add utilizable tiles to the tile map, it's useful for backtracking */
    for (uint8_t i = 0; i < NUMBER_OF_TILE_TYPES; i++)
    {
        for (TileID tile_id = 0; tile_id < hw::hal::Tile::getTileCount(); tile_id++)
        {
            uint8_t tile_type_id = utilizable_tiles.map[i].type_id;
            if ((is_tile_of_type(tile_id, tile_type_id)) && (are_all_tile_resources_on_claim(tile_id, available_resources)))
                utilizable_tiles.map[i].add_tile(tile_type_id, tile_id);
        }
    }

    /* backtrack call for solving cluster guarantees with all needed and updated parameter */
    bool is_backtracking_successfull = backtrack(actor_constraint, op_claim, operating_point_id, op_cluster_guarantee_count, op_cluster_count, utilizable_tiles, solution);

    if (is_backtracking_successfull)
    {
        /* Display information about partial claim for each cluster guanrantee */
        for (uint8_t op_cg_id = 0; op_cg_id < op_cluster_guarantee_count; op_cg_id++)
        {
            DBG(SUB_AGENT, "CG[%d] = %d => Solution: Tile %d & Tile %d\n", solution.map[op_cg_id].op_local_cg_id, solution.map[op_cg_id].op_global_cg_id, solution.map[op_cg_id].first_tile_id, solution.map[op_cg_id].second_tile_id);
        }
    }
    return std::make_pair(is_backtracking_successfull, op_claim);
}

bool os::agent::ActorConstraintSolver::backtrack(const ActorConstraint &actor_constraint, AgentClaim &actual_satisfying_claim, const uint8_t &operating_point_id, const uint8_t &op_cluster_guarantee_count, uint8_t &op_cluster_count, TileMap &utilizable_tiles, CGSolutionMap &op_solution)
{
    TilePath solution_path(op_cluster_count);
    uint8_t needed_risc = 0, needed_icore = 0, needed_tcpa = 0, first_id_risc = 0, first_id_icore = 0, first_id_tcpa = 0;
    bool is_operating_point_solved = false;

    prepare_backtracking(actor_constraint, operating_point_id, op_cluster_guarantee_count, op_cluster_count, needed_risc, needed_icore, needed_tcpa, first_id_risc, first_id_icore, first_id_tcpa);

    if (op_cluster_count != (needed_risc + needed_icore + needed_tcpa))
    {
        DBG(SUB_AGENT, "op_cluster_count == %d  but NOT EQUAL to\n", op_cluster_count);
        DBG(SUB_AGENT, "needed_risc (%d) +  needed_icore (%d) + needed_tcpa (%d) == %d\n",
            needed_risc, needed_icore, needed_tcpa, (needed_risc + needed_icore + needed_tcpa));
        panic("cluster count isn't consistent");
    }

    /* initialize solving path */
    for (uint8_t i = 0; i < op_cluster_count; i++)
    {
        solution_path.path[i].local_cluster_id = i;
        solution_path.path[i].global_cluster_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_id_list()[i];
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        for (uint8_t j = 0; j < utilizable_tiles.map[i].tile_count; j++)
        {
            DBG(SUB_AGENT, "Type %d: Tile %d\n", utilizable_tiles.map[i].type_id, utilizable_tiles.map[i].tiles[j].tile_id);
        }
    }

    /* try solving only if there enough resources of each type avaible*/
    if (utilizable_tiles.map[TILE_RISC_ID - 1].tile_count >= needed_risc && utilizable_tiles.map[TILE_ICORE_ID - 1].tile_count >= needed_icore && utilizable_tiles.map[TILE_TCPA_ID - 1].tile_count >= needed_tcpa)
    {
        /* searching for needed clusters; step is the current cluster id we are searching for */
        while ((solution_path.step_count < op_cluster_count) && (!is_operating_point_solved))
        {
            /* actual needed cluster */
            uint8_t global_cluster_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_id_list()[solution_path.step_count];
            Cluster needed_cluster(actor_constraint.get_cluster_list()[global_cluster_id]);
            bool is_cluster_satisfied = false;

            DBG(SUB_AGENT, "\n\n\nSearch Step %d => actual path length is %d:\n", solution_path.step_count, solution_path.step_count);
            for (uint8_t i = 0; i < solution_path.step_count; i++)
            {
                DBG(SUB_AGENT, "==> %d\n", solution_path.path[i].tile_id);
            }

            if (utilizable_tiles.map[(needed_cluster.get_type() - 1)].tile_count - utilizable_tiles.map[(needed_cluster.get_type() - 1)].reserved_count == 0)
            {
                panic("resources are inconsistent");
            }
            else
            {
                /* Tiles statistic for the needed type */
                DBG(SUB_AGENT, "Needed Type: %d => %d of %d available tiles are unreserved\n",
                    utilizable_tiles.map[(needed_cluster.get_type() - 1)].type_id,
                    (utilizable_tiles.map[(needed_cluster.get_type() - 1)].tile_count - utilizable_tiles.map[(needed_cluster.get_type() - 1)].reserved_count),
                    utilizable_tiles.map[(needed_cluster.get_type() - 1)].tile_count);
            }

            /* search the next candidate in the map for utilizable and unreserved tiles of the same type (risc, icore or tcpa)*/
            for (uint8_t tile_map_index = 0; tile_map_index < utilizable_tiles.map[(needed_cluster.get_type() - 1)].tile_count; tile_map_index++)
            {

                /* If tile is reserved */
                if (utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].reserved)
                {
                    DBG(SUB_AGENT, "=> %d. element: Tile %d is RESERVED\n",
                        tile_map_index,
                        utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);
                }
                /* If tile is unreserved */
                else
                {
                    DBG(SUB_AGENT, "=> %d. element: Tile %d is FREE\n",
                        tile_map_index,
                        utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);

                    /* Verify if this tile hasn't been tried as next candidate for this step before */
                    bool is_candidate_tried = false;

                    if (solution_path.step_count == 0)
                    {
                        DBG(SUB_AGENT, "===> Already %d tiles tried as candidates on this step %d: \n",
                            solution_path.step_count,
                            solution_path.first_step_trial_count);

                        /* Verify if tile wasn't already tried as next candidate for this path combination */
                        for (uint8_t trial_id = 0; trial_id < solution_path.first_step_trial_count; trial_id++)
                        {
                            if ((solution_path.first_step_candidate_trials[trial_id].is_tried) &&
                                (solution_path.first_step_candidate_trials[trial_id].tile_id ==
                                 utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id))
                            {
                                DBG(SUB_AGENT, "==> Tile %d already tried as candidate on step %d\n",
                                    utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                    solution_path.step_count);
                                is_candidate_tried = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        /* Verify if tile wasn't already tried as next candidate for this path combination */
                        for (uint8_t trial_id = 0; trial_id < solution_path.path[solution_path.step_count - 1].trial_count; trial_id++)
                        {
                            if ((solution_path.path[solution_path.step_count - 1].next_candidates[trial_id].is_tried) &&
                                (solution_path.path[solution_path.step_count - 1].next_candidates[trial_id].tile_id ==
                                 utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id))
                            {
                                DBG(SUB_AGENT, "===> Tile %d already tried as next candidate of %d for step %d\n",
                                    utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                    solution_path.path[(solution_path.step_count - 1)].tile_id,
                                    solution_path.step_count);
                                is_candidate_tried = true;
                                break;
                            }
                        }
                    }

                    if (is_candidate_tried == false)
                    {
                        DBG(SUB_AGENT, "===> Tile %d is UNTRIED as next candidate of %d for step %d\n",
                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                            solution_path.path[(solution_path.step_count - 1)].tile_id,
                            solution_path.step_count);

                        if (solution_path.step_count == 0)
                        {
                            /* For the first cluster we just add the resource */
                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].reserve_tile(utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);
                            solution_path.add_step_candidate(
                                utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                utilizable_tiles.map[(needed_cluster.get_type() - 1)].type_id);
                            is_cluster_satisfied = true;
                            break;
                        }
                        else
                        {
                            /* mark the actual tile id as already tried as next candidate
                             * mark tile as tried next candidate for the step before */
                            solution_path.add_next_candidate_step_trial(
                                utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                utilizable_tiles.map[(needed_cluster.get_type() - 1)].type_id);

                            /* Looking for the guarantee which contains the needed cluster id */
                            uint8_t search_cg_step = 0;
                            while (search_cg_step < op_cluster_guarantee_count)
                            {
                                uint8_t global_cg_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_guarantee_id_list()[search_cg_step];
                                uint8_t last_global_cg_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_guarantee_id_list()[search_cg_step - 1];

                                Cluster first_cluster(actor_constraint.get_cluster_guarantee_list()[global_cg_id].get_first_cluster());
                                Cluster second_cluster(actor_constraint.get_cluster_guarantee_list()[global_cg_id].get_second_cluster());
                                Cluster second_cluster_last_cg(actor_constraint.get_cluster_guarantee_list()[last_global_cg_id].get_second_cluster());

                                if (first_cluster.get_id() == needed_cluster.get_id())
                                {
                                    DBG(SUB_AGENT, "\n\nGUARANTEE: FIRST CLUSTER =>  BEGIN\n");

                                    /* If the second cluster id of the precedent guarantee is different then we search for a new cluster
                                     * elsewhere the solution of the second cluter of precedent guarantee is the same as the solution for this first cluster */
                                    if ((second_cluster_last_cg.get_id() != needed_cluster.get_id()))
                                    {
                                        DBG(SUB_AGENT, "Tile %d is UNRESERVED and UNTRIED for this step %d. Add to solution path\n",
                                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                            solution_path.step_count);
                                        /* add the available tile to the solution path and reserve the tile on map */
                                        utilizable_tiles.map[(needed_cluster.get_type() - 1)].reserve_tile(utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);
                                        solution_path.add_step_candidate(
                                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                            needed_cluster.get_type());
                                        is_cluster_satisfied = true;
                                    }
                                    else if (second_cluster_last_cg.get_id() == needed_cluster.get_id())
                                    {
                                        panic("SEARCHING NEW FIRST CLUSTER GUARANTEE ID WHICH IS THE SAME AS SECOND CLUSTER GUARANEE BEFORE\n");
                                    }
                                    else
                                    {
                                        panic("GUARANTEE: FIRST CLUSTER => case undefined");
                                    }
                                    DBG(SUB_AGENT, "\nGUARANTEE: FIRST CLUSTER =>  END\n");
                                    break;
                                }
                                else if (second_cluster.get_id() == needed_cluster.get_id())
                                {
                                    DBG(SUB_AGENT, "\n\nGUARANTEE: SECOND CLUSTER\n");
                                    uint8_t hop_distance = actor_constraint.get_cluster_guarantee_list()[global_cg_id].getHopDistance();
                                    uint8_t service_level = actor_constraint.get_cluster_guarantee_list()[global_cg_id].getServiceLevel();

                                    /* VERIFY if the guaranteee is satisfied with this new cluster  */
                                    if ((get_service_level(solution_path.path[solution_path.step_count - 1].tile_id, utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id) > (service_level - 1)) && (get_hop_count(solution_path.path[solution_path.step_count - 1].tile_id, utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id) < (hop_distance + 1)))
                                    {
                                        DBG(SUB_AGENT, "=========> Well done: HD and SL are satisfied. RESERVE tile %d\n",
                                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);

                                        /* add the available tile to the solution path and reserve the tile on map */
                                        utilizable_tiles.map[(needed_cluster.get_type() - 1)].reserve_tile(utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);
                                        solution_path.add_step_candidate(
                                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                            needed_cluster.get_type());
                                        is_cluster_satisfied = true;

                                        /* if the last cluster, member of the last guarantee is satisfied */
                                        if (solution_path.step_count == op_cluster_count)
                                        {
                                            DBG(SUB_AGENT, "LAST CLUSTER SATISFIED\n");
                                            is_operating_point_solved = true;
                                        }
                                    }
                                    else
                                    {
                                        /* tile is unreserved and doesn't satisfy the guarantee */
                                        DBG(SUB_AGENT, "==========> Tile %d was UNSUCCESSFULLY TRIED as next candidate of %d for step %d. Either SL or HD unsatisfied\n",
                                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id,
                                            solution_path.path[(solution_path.step_count - 1)].tile_id,
                                            solution_path.step_count);
                                    }
                                    DBG(SUB_AGENT, "\nGUARANTEE: SECOND CLUSTER => END\n");
                                    break;
                                }
                                else
                                {
                                    search_cg_step++;
                                    continue;
                                }

                                /* search until the cluster is satisfied */
                                if (is_cluster_satisfied)
                                {
                                    DBG(SUB_AGENT, "=====> Step %d & guarantee %d => CLUSTER SATISFIED.\nPath:\n", solution_path.step_count, search_cg_step);
                                    for (uint8_t i = 0; i < solution_path.step_count; i++)
                                    {
                                        DBG(SUB_AGENT, "=====> %d\n", solution_path.path[i].tile_id);
                                    }
                                    break;
                                }
                                else
                                {
                                    DBG(SUB_AGENT, "=====> Step %d & guarantee %d => CLUSTER UNSATISFIED\n", solution_path.step_count, search_cg_step);
                                    for (uint8_t i = 0; i < solution_path.step_count; i++)
                                    {
                                        DBG(SUB_AGENT, "=====> %d\n", solution_path.path[i].tile_id);
                                    }
                                    search_cg_step++;
                                }
                            }
                        }
                    }
                    else
                    {
                        DBG(SUB_AGENT, "===> %d. tile %d was TRIED as candidate on this step\n",
                            tile_map_index + 1,
                            utilizable_tiles.map[(needed_cluster.get_type() - 1)].tiles[tile_map_index].tile_id);
                        continue;
                    }
                }
                if (is_cluster_satisfied)
                    break;
            }

            if (is_cluster_satisfied && is_operating_point_solved)
            {
                /* the last cluster is satisfied */
                DBG(SUB_AGENT, "===> CLUSTER SATISFIED AND OPERATING POINT SOLVED.\n");
                break;
            }
            else if (is_cluster_satisfied && !is_operating_point_solved)
            {
                /* the current cluster is satisfied and there are other clusters to satisfy */
                DBG(SUB_AGENT, "Cluster satisfied & Operating point UNSOLVED. Step %d\n", solution_path.step_count);
            }
            /* the cluster isn't satisfied as with all the possible options in the current step
             * If the actual cluster cannot be satisfied in this step with available resources then we backtrack
             */
            else if (!is_cluster_satisfied)
            {
                DBG(SUB_AGENT, "Cluster UNSATISFIED. Step %d\n", solution_path.step_count);
                if ((solution_path.path[solution_path.step_count - 1].trial_count =
                         (utilizable_tiles.map[(needed_cluster.get_type() - 1)].tile_count -
                          utilizable_tiles.map[(needed_cluster.get_type() - 1)].reserved_count)))
                {
                    /*
                     * if we've tried all the available and unreserved resources as next candidates then we
                     * we mark all the tried tiles as untried
                     */
                    DBG(SUB_AGENT, "Step %d : maximal candidate trials (%d). Sanitize step candidates, backtrack to step %d and remove tile %d\n",
                        solution_path.step_count,
                        solution_path.path[solution_path.step_count - 1].trial_count,
                        solution_path.step_count - 1,
                        solution_path.path[solution_path.step_count - 1].tile_id);

                    /* sanitize trials, then unreserve the lastly added tile, remove it from the actual path and actualize the found resources of this type */
                    for (uint8_t i = 0; i < utilizable_tiles.map[((solution_path.path[solution_path.step_count - 1].tile_type) - 1)].tile_count; i++)
                    {
                        if (utilizable_tiles.map[((solution_path.path[solution_path.step_count - 1].tile_type) - 1)].tiles[i].tile_id ==
                            (solution_path.path[solution_path.step_count - 1].tile_id &&
                             utilizable_tiles.map[((solution_path.path[solution_path.step_count - 1].tile_type) - 1)].tiles[i].reserved))
                        {
                            DBG(SUB_AGENT, "Tile %d is the last added candidate\n",
                                solution_path.path[solution_path.step_count - 1].tile_id);
                            /* backtrack */
                            solution_path.sanitize_step_trials();
                            utilizable_tiles.map[((solution_path.path[solution_path.step_count - 1].tile_type) - 1)].unreserve_tile(solution_path.path[solution_path.step_count - 1].tile_id);
                            solution_path.remove_candidate();
                            break;
                        }
                        else
                        {
                            DBG(SUB_AGENT, "Tile %d is not the last added candidate\n", utilizable_tiles.map[((solution_path.path[solution_path.step_count - 1].tile_type) - 1)].tiles[i].tile_id);
                        }
                    }

                    /* backtrack only for the steps greather than 0.
                     * We should remember the number of trials for step 0, because it's the moment we consider backtracking as unsuccesfull */
                    while (solution_path.step_count > 0)
                    {
                        DBG(SUB_AGENT, "Step %d : Type %d is needed\n", solution_path.step_count, solution_path.last_removed_tile_type);
                        DBG(SUB_AGENT, "Availaible tiles of type %d: %d; reserved one: %d; already %d trials \n",
                            solution_path.last_removed_tile_type,
                            utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count,
                            utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].reserved_count,
                            solution_path.path[solution_path.step_count - 1].trial_count);

                        /* If all the available and unreserved candidates have been tried for the step before */
                        if (solution_path.path[solution_path.step_count - 1].trial_count ==
                            utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count -
                                utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].reserved_count)
                        {
                            DBG(SUB_AGENT, "Step %d, needed type %d, Maximal trials (%d) for step candidates => Try Sanitizing then Backtracking to step %d\n",
                                solution_path.step_count,
                                solution_path.last_removed_tile_type,
                                solution_path.path[solution_path.step_count - 1].trial_count,
                                solution_path.step_count - 1);
                            for (uint8_t i = 0; i < utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count; i++)
                            {
                                if (utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tiles[i].tile_id == solution_path.path[(solution_path.step_count - 1)].tile_id)
                                {
                                    DBG(SUB_AGENT, "Step %d , tile %d has to be removed from solution path, then we backtrack to step %d\n",
                                        solution_path.step_count,
                                        utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tiles[i].tile_id,
                                        solution_path.step_count - 1);

                                    DBG(SUB_AGENT, "Type %d: total = %d, reserved = %d\n",
                                        solution_path.path[(solution_path.step_count - 1)].tile_type,
                                        utilizable_tiles.map[(solution_path.path[(solution_path.step_count - 1)].tile_type - 1)].tile_count,
                                        utilizable_tiles.map[(solution_path.path[(solution_path.step_count - 1)].tile_type - 1)].reserved_count);

                                    if ((utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count -
                                         utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].reserved_count) ==
                                        solution_path.path[solution_path.step_count - 1].trial_count)
                                    {
                                        DBG(SUB_AGENT, "Step %d, needed type %d, Maximal step trials (%d) for step candidates => Sanitze and Backtrack to step %d\n",
                                            solution_path.step_count,
                                            solution_path.last_removed_tile_type,
                                            solution_path.path[solution_path.step_count - 1].trial_count,
                                            solution_path.step_count - 1);
                                        solution_path.sanitize_step_trials();
                                        utilizable_tiles.map[(solution_path.path[(solution_path.step_count - 1)].tile_type - 1)].unreserve_tile(solution_path.path[(solution_path.step_count - 1)].tile_id);
                                        solution_path.remove_candidate();
                                    }
                                    else
                                    {
                                        DBG(SUB_AGENT, "Step %d, There are %d untried possibilities\n",
                                            solution_path.step_count,
                                            (utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count -
                                             utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].reserved_count -
                                             solution_path.path[solution_path.step_count - 1].trial_count));
                                    }
                                    break;
                                }
                                else
                                {
                                    DBG(SUB_AGENT, "Step %d: tile %d is not the one to be removed. Tile %d should be removed\n",
                                        solution_path.step_count,
                                        utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tiles[i].tile_id,
                                        solution_path.path[solution_path.step_count - 1].tile_id);
                                }
                            }
                            continue;
                        }
                        else
                        {
                            DBG(SUB_AGENT, "There are still %d untried possibilities for step %d. Keep trying new candidates\n",
                                (utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count -
                                 utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].reserved_count -
                                 solution_path.path[solution_path.step_count - 1].trial_count),
                                solution_path.step_count);
                            break;
                        }
                    }
                    if (solution_path.step_count == 0)
                    {
                        DBG(SUB_AGENT, "First cluster type is %d\n", solution_path.last_removed_tile_type);
                        if (utilizable_tiles.map[(solution_path.last_removed_tile_type - 1)].tile_count == solution_path.first_step_trial_count)
                        {
                            /* all possible paths have been explored
                             * backtracking unsuccessful */
                            DBG(SUB_AGENT, "Step %d, maximal candidate trials %d, candidate %d was last possibility\n", solution_path.step_count,
                                solution_path.first_step_trial_count, solution_path.path[solution_path.step_count].tile_id);
                            break;
                        }
                        else
                        {
                            DBG(SUB_AGENT, "Step %d, just %d trials on this step. There other possibilities\n", solution_path.step_count, solution_path.first_step_trial_count);
                        }
                    }
                    else
                    {
                        DBG(SUB_AGENT, "Step %d : try other candidates for this step\n", solution_path.step_count);
                    }
                }
                else
                {
                    DBG(SUB_AGENT, "==> Step %d : %d next candidate trials\n", solution_path.step_count, solution_path.path[solution_path.step_count - 1].trial_count);
                }
            }
            else
            {
                panic("Search cluster step ==> This option is undefined");
            }
        }

        DBG(SUB_AGENT, "Path after research:\n");
        for (uint8_t i = 0; i < solution_path.step_count; i++)
        {
            DBG(SUB_AGENT, "==> %d\n", solution_path.path[i].tile_id);
        }
    }
    else
    {
        DBG(SUB_AGENT, "UNSOLVABLE: not enough resources\n");
    }

    if (is_operating_point_solved)
    {
        DBG(SUB_AGENT, "OP[%d] is solved \n", operating_point_id);
        actual_satisfying_claim.setOperatingPointIndex(operating_point_id);
        /* update the op claim */
        /* cluster guarante has a solution */
        for (uint8_t i = 0; i < solution_path.step_count; i++)
        {
            ResourceID satisfying_cluster;
            satisfying_cluster.tileID = solution_path.path[i].tile_id;

            for (uint8_t j = 0; j < os::agent::MAX_RES_PER_TILE; j++)
            {
                satisfying_cluster.resourceID = j;
                actual_satisfying_claim.add(satisfying_cluster);
            }

            //            for(uint8_t j = 0; j < op_cluster_guarantee_count; j++)
            //            {
            //                uint8_t global_cg_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_guarantee_id_list()[j];
            //                Cluster first_cluster(actor_constraint.get_cluster_guarantee_list()[global_cg_id].get_first_cluster());
            //                Cluster second_cluster(actor_constraint.get_cluster_guarantee_list()[global_cg_id].get_second_cluster());

            //                ResourceID first_cluster_satisfied_resource, second_cluster_satisfied_resource;
            //                first_cluster_satisfied_resource.tileID = solution_path.path[i].tile_id;
            //                second_cluster_satisfied_resource.tileID = solution_path.path[i].tile_id;

            //                op_solution.map[j].is_solved = true;

            //                if(first_cluster.get_id() == solution_path.path[i].global_cluster_id)
            //                {
            //                    op_solution.map[j].first_tile_id = first_cluster_satisfied_resource.tileID;
            //                    op_solution.map[j].first_tile_claim.add(first_cluster_satisfied_resource);

            //                    for(uint8_t k = 0; k < os::agent::MAX_RES_PER_TILE; k++)
            //                    {
            //                        first_cluster_satisfied_resource.resourceID = k;
            //                        op_solution.map[j].first_tile_claim.add(first_cluster_satisfied_resource);
            //                    }
            //                }
            //                if(second_cluster.get_id() == solution_path.path[i].global_cluster_id)
            //                {
            //                    op_solution.map[j].second_tile_id = second_cluster_satisfied_resource.tileID;
            //                    op_solution.map[j].second_tile_claim.add(second_cluster_satisfied_resource);

            //                    for(uint8_t k = 0; k < os::agent::MAX_RES_PER_TILE; k++)
            //                    {
            //                        second_cluster_satisfied_resource.resourceID = k;
            //                        op_solution.map[j].first_tile_claim.add(second_cluster_satisfied_resource);
            //                    }
            //                }
            //            }
        }
        DBG(SUB_AGENT, "Satisfying claim is\n");
        actual_satisfying_claim.print();
    }
    else
    {
        DBG(SUB_AGENT, "OP[%d] isn't solved \n", operating_point_id);
    }

    return is_operating_point_solved;
}

void os::agent::ActorConstraintSolver::prepare_backtracking(const ActorConstraint &actor_constraint, const uint8_t &operating_point_id, const uint8_t &op_cluster_guarantee_count, const uint8_t &op_cluster_count, uint8_t &risc_count, uint8_t &icore_count, uint8_t &tcpa_count, uint8_t &first_risc_id, uint8_t &first_icore_id, uint8_t &first_tcpa_id)
{
    /* count the needed tiles for every type */
    for (uint8_t cg_id = 0; cg_id < op_cluster_guarantee_count; cg_id++)
    {
        uint8_t global_cg_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_guarantee_id_list()[cg_id];
        Cluster first_cluster(actor_constraint.get_cluster_guarantee_list()[global_cg_id].get_first_cluster());
        Cluster second_cluster(actor_constraint.get_cluster_guarantee_list()[global_cg_id].get_second_cluster());

        if (cg_id == 0)
        {
            switch (first_cluster.get_type())
            {
            case TILE_RISC_ID:
                risc_count++;
                break;
            case TILE_ICORE_ID:
                icore_count++;
                break;
            case TILE_TCPA_ID:
                tcpa_count++;
                break;
            default:
                panic("first cluster type id of first guarantee invalid");
            }
        }
        else
        {
            uint8_t precedent_global_cg_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_guarantee_id_list()[cg_id - 1];
            Cluster second_cluster_precedent_guarantee(actor_constraint.get_cluster_guarantee_list()[precedent_global_cg_id].get_second_cluster());
            if (first_cluster.get_id() != second_cluster_precedent_guarantee.get_id())
            {
                switch (first_cluster.get_type())
                {
                case TILE_RISC_ID:
                    risc_count++;
                    break;
                case TILE_ICORE_ID:
                    icore_count++;
                    break;
                case TILE_TCPA_ID:
                    tcpa_count++;
                    break;
                default:
                    panic("cluster type id of guarantee is invalid");
                }
            }
        }
        switch (second_cluster.get_type())
        {
        case TILE_RISC_ID:
            risc_count++;
            break;
        case TILE_ICORE_ID:
            icore_count++;
            break;
        case TILE_TCPA_ID:
            tcpa_count++;
            break;
        default:
            panic("second cluster type id is invalid");
        }
    }
    /* find the first id of every cluster type under the cluster list */
    for (uint8_t c_id = 0; c_id < op_cluster_count; c_id++)
    {
        uint8_t global_c_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_id_list()[c_id];
        if (actor_constraint.get_cluster_list()[global_c_id].get_type() == TILE_RISC_ID)
        {
            first_risc_id = c_id;
            break;
        }
    }
    for (uint8_t c_id = 0; c_id < op_cluster_count; c_id++)
    {
        uint8_t global_c_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_id_list()[c_id];
        if (actor_constraint.get_cluster_list()[global_c_id].get_type() == TILE_ICORE_ID)
        {
            first_icore_id = c_id;
            break;
        }
    }
    for (uint8_t c_id = 0; c_id < op_cluster_count; c_id++)
    {
        uint8_t global_c_id = actor_constraint.get_operating_point_list()[operating_point_id].get_cluster_id_list()[c_id];
        if (actor_constraint.get_cluster_list()[global_c_id].get_type() == TILE_TCPA_ID)
        {
            first_tcpa_id = c_id;
            break;
        }
    }

    DBG(SUB_AGENT, "risc_count = %d\n", risc_count);
    DBG(SUB_AGENT, "icore_count = %d\n", icore_count);
    DBG(SUB_AGENT, "tcpa_count = %d\n", tcpa_count);
    DBG(SUB_AGENT, "first_risc_id = %d\n", first_risc_id);
    DBG(SUB_AGENT, "first_icore_id = %d\n", first_icore_id);
    DBG(SUB_AGENT, "first_tcpa_id = %d\n\n\n", first_tcpa_id);
}

bool os::agent::ActorConstraintSolver::is_tile_of_type(TileID tileID, uint8_t type)
{
    ResourceID resource;
    bool is_satisfied = false;
    resource.tileID = tileID;
    resource.resourceID = 0;
    if ((os::agent::HardwareMap[resource.tileID][resource.resourceID].type) == (type - 1))
        is_satisfied = true;
    else
        is_satisfied = false;

    if (is_satisfied)
        DBG(SUB_AGENT, "Tile %d & Type %d => true\n", tileID, type);
    else
        DBG(SUB_AGENT, "Tile %d & Type %d => false\n", tileID, type);
    return is_satisfied;
}

bool os::agent::ActorConstraintSolver::are_all_tile_resources_on_claim(TileID tileID, AgentClaim availableResources)
{
    bool is_satisfied = true;
    ResourceID resource;
    resource.tileID = tileID;

    for (uint8_t i = 0; i < os::agent::MAX_RES_PER_TILE; i++)
    {
        resource.resourceID = i;
        if (os::agent::HardwareMap[resource.tileID][resource.resourceID].type != none && !availableResources.contains(resource))
        {
            /* False if at least one resource is not available */
            is_satisfied = false;
            break;
        }
    }
    return is_satisfied;
}

uint8_t os::agent::ActorConstraintSolver::get_hop_count(TileID start_tile_id, TileID end_tile_id)
{
    uint8_t x_distance, y_distance;
    hw::dev::HWInfo &hwInfo = hw::dev::HWInfo::Inst();

    int8_t first_tile_x = (int8_t)hwInfo.getXCoord(start_tile_id);
    int8_t first_tile_y = (int8_t)hwInfo.getYCoord(start_tile_id);
    int8_t second_tile_x = (int8_t)hwInfo.getXCoord(end_tile_id);
    int8_t second_tile_y = (int8_t)hwInfo.getYCoord(end_tile_id);

    /* Manhattan metric */
    if ((first_tile_x - second_tile_x) < 0)
        x_distance = (uint8_t)(second_tile_x - first_tile_x);
    else
        x_distance = (uint8_t)(first_tile_x - second_tile_x);

    if ((first_tile_y - second_tile_y) < 0)
        y_distance = (uint8_t)(second_tile_y - first_tile_y);
    else
        y_distance = (uint8_t)(first_tile_y - second_tile_y);

    DBG(SUB_AGENT, "Tile %d (%d/%d) - Tile %d (%d/%d): hopcount = %d\n", start_tile_id, first_tile_x, first_tile_y, end_tile_id, second_tile_x, second_tile_y, (x_distance + y_distance));

    return (x_distance + y_distance);
}

uint8_t os::agent::ActorConstraintSolver::get_service_level(TileID start_tile_id, TileID end_tile_id)
{
    DBG(SUB_AGENT, "Tile %d - Tile %d: service_level = %d\n", start_tile_id, end_tile_id, 5);
    return 5;
}

void *os::agent::ActorConstraintSolver::operator new(size_t s)
{
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::ActorConstraintSolver::operator delete(void *c, size_t s)
{
    os::agent::AgentMemory::agent_mem_free(c);
}
