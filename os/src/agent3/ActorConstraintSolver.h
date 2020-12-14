#ifndef ACTORCONSTRAINTSOLVER_H
#define ACTORCONSTRAINTSOLVER_H

#include <utility>

#include "os/agent3/AgentSystem.h"
#include "os/agent3/AgentClaim.h"
#include "hw/dev/HWInfo.h"
#include "lib/debug.h"
#include "lib/kassert.h"
#include "Cluster.h"

namespace os
{
    namespace agent
    {
        class ActorConstraint;

        class ActorConstraintSolver
        {
#define NUMBER_OF_TILE_TYPES 3
#define TOTAL_NUMBER_OF_TILES 16
#define MAX_NUMBER_OF_GUARANTEES 8

            typedef os::agent::AgentSystem AS;

            typedef struct
            {
                uint8_t tile_id;
                bool reserved;

                TileInformation()
                {
                    tile_id = 0;
                    reserved = false;
                }
                void reserve()
                {
                    kassert(reserved == false);
                    reserved = true;
                    DBG(SUB_AGENT, "Tile %d is now RESERVED:\n", tile_id);
                }
                void unreserve()
                {
                    kassert(reserved == true);
                    reserved = false;
                    DBG(SUB_AGENT, "Tile %d is now UNRESERVED:\n", tile_id);
                }
            } TileInformation;

            typedef struct
            {
                uint8_t type_id;
                uint8_t tile_count;
                uint8_t reserved_count;
                TileInformation tiles[TOTAL_NUMBER_OF_TILES];

                TileTypeInformation()
                {
                    type_id = 0;
                    tile_count = 0;
                    reserved_count = 0;
                }

                void add_tile(uint8_t type, uint8_t tile)
                {
                    kassert(type >= TILE_RISC_ID && type <= TILE_TCPA_ID);
                    kassert(tile_count < TOTAL_NUMBER_OF_TILES);
                    kassert(type_id == type);
                    tiles[tile_count].tile_id = tile;
                    tile_count++;
                }

                void reserve_tile(uint8_t id)
                {
                    bool reserved = false;
                    kassert(reserved_count < tile_count);
                    for (uint8_t i = 0; i < tile_count; i++)
                    {
                        if (tiles[i].tile_id == id)
                        {
                            tiles[i].reserve();
                            reserved = true;
                            reserved_count++;
                            break;
                        }
                    }
                    kassert(reserved == true);
                    if (reserved)
                        DBG(SUB_AGENT, "Tile %d reserved", id);
                }
                void unreserve_tile(uint8_t id)
                {
                    bool unreserved = false;
                    kassert(reserved_count > 0);
                    for (uint8_t i = 0; i < tile_count; i++)
                    {
                        if (tiles[i].tile_id == id)
                        {
                            tiles[i].unreserve();
                            unreserved = true;
                            reserved_count--;
                            break;
                        }
                    }
                    kassert(unreserved == true);
                    if (unreserved)
                        DBG(SUB_AGENT, "Tile %d freed", id);
                }

            } TileTypeInformation;

            typedef struct
            {
                TileTypeInformation map[NUMBER_OF_TILE_TYPES];

                TileMap()
                {
                    for (uint8_t i = 0; i < NUMBER_OF_TILE_TYPES; i++)
                    {
                        /* map[i] = i+1*/
                        switch (i)
                        {
                        case 0:
                            map[i].type_id = TILE_RISC_ID;
                            break;
                        case 1:
                            map[i].type_id = TILE_ICORE_ID;
                            break;
                        case 2:
                            map[i].type_id = TILE_TCPA_ID;
                            break;
                        default:
                            panic("Tile map initialization failed!!!");
                        }
                    }
                }
            } TileMap;

            typedef struct
            {
                uint8_t tile_id;
                uint8_t is_tried;

                Trial()
                {
                    tile_id = TOTAL_NUMBER_OF_TILES;
                    is_tried = false;
                }

                void set_details(uint8_t id, bool tried)
                {
                    tile_id = id;
                    is_tried = tried;
                }
            } Trial;

            typedef struct
            {
                uint8_t local_cluster_id;
                uint8_t global_cluster_id;
                uint8_t tile_id;
                uint8_t tile_type;

                Trial next_candidates[TOTAL_NUMBER_OF_TILES];
                uint8_t trial_count;

                BacktrackingCandidate()
                {
                    tile_id = TOTAL_NUMBER_OF_TILES + 1;
                    tile_type = 0;
                    trial_count = 0;
                }

                void set_information(uint8_t id, uint8_t type)
                {
                    tile_id = id;
                    tile_type = type;
                }

                void add_next_candidate_trial(uint8_t id)
                {
                    for (uint8_t i = 0; i < trial_count; i++)
                    {
                        if (next_candidates[trial_count].tile_id == id && next_candidates[trial_count].is_tried)
                            panic("add_next_candidate_trial: Tile was already next candidate for this step");
                    }
                    next_candidates[trial_count].set_details(id, true);
                    trial_count++;
                }

                void sanitize_next_candidates(uint8_t predecessor, uint8_t last_step)
                {
                    DBG(SUB_AGENT, "Sanitize next candidate trials of tile %d in Step %d:\n", predecessor, last_step);
                    for (uint8_t i = 0; i < trial_count; i++)
                    {
                        DBG(SUB_AGENT, "==> Tile %d:\n", next_candidates[i].tile_id);
                        next_candidates[i].set_details(TOTAL_NUMBER_OF_TILES, false);
                    }
                    trial_count = 0;
                }
            } BacktrackingCandidate;

            typedef struct
            {
                BacktrackingCandidate path[TOTAL_NUMBER_OF_TILES];
                Trial first_step_candidate_trials[TOTAL_NUMBER_OF_TILES];
                uint8_t first_step_trial_count;
                uint8_t last_removed_tile_id;
                uint8_t last_removed_tile_type;
                uint8_t path_length;
                uint8_t step_count;

                TilePath(uint8_t length)
                {
                    path_length = length;
                    step_count = 0;
                    for (uint8_t i = 0; i < TOTAL_NUMBER_OF_TILES; i++)
                    {
                        first_step_candidate_trials[i].set_details(TOTAL_NUMBER_OF_TILES, false);
                    }
                    first_step_trial_count = 0;
                    last_removed_tile_id = TOTAL_NUMBER_OF_TILES;
                    last_removed_tile_type = 0;
                }

                void add_next_candidate_step_trial(uint8_t id, uint8_t type)
                {
                    kassert(step_count < path_length);
                    kassert(type >= TILE_RISC_ID && type <= TILE_TCPA_ID);
                    for (uint8_t i = 0; i < path[step_count - 1].trial_count; i++)
                    {
                        if (path[step_count - 1].next_candidates[i].tile_id == id && path[step_count - 1].next_candidates[i].is_tried == true)
                            panic("add_step_candidate: Tile was already candidate in this for this step");
                    }
                    DBG(SUB_AGENT, "add_step_candidate : tile %d trial on step %d\n", id, step_count);
                    path[step_count - 1].add_next_candidate_trial(id);
                }

                void add_step_candidate(uint8_t id, uint8_t type)
                {
                    bool is_already_tried = false;
                    DBG(SUB_AGENT, "add_step_candidate => Step %d: tile %d added\n", step_count, id);

                    if (step_count == 0)
                    {
                        for (uint8_t i = 0; i < first_step_trial_count; i++)
                        {
                            if (first_step_candidate_trials[i].tile_id == id)
                            {
                                is_already_tried = true;
                                break;
                            }
                        }
                        if (is_already_tried == false)
                        {
                            first_step_candidate_trials[first_step_trial_count].set_details(id, true);
                            first_step_trial_count++;
                            path[0].set_information(id, type);
                            DBG(SUB_AGENT, "add_step_candidate => Tile %d added\n", id);
                            step_count++;
                        }
                        else
                            panic("candidate already tried");
                    }

                    else
                    {
                        path[step_count].set_information(id, type);
                        DBG(SUB_AGENT, "add_step_candidate => Tile %d added\n", id);
                        step_count++;
                    }
                }

                void sanitize_step_trials()
                {
                    path[step_count - 1].sanitize_next_candidates(path[step_count - 1].tile_id, step_count);
                }

                void remove_candidate()
                {
                    kassert(step_count > 0);
                    step_count--;
                    last_removed_tile_id = path[step_count].tile_id;
                    last_removed_tile_type = path[step_count].tile_type;
                    path[step_count].set_information(TOTAL_NUMBER_OF_TILES, 0);
                    DBG(SUB_AGENT, "remove_candidate: Step %d => tile path of length %d after backtrack after removing tile %d as candidate for this step:\n", step_count, step_count, last_removed_tile_id);
                    for (uint8_t i = 0; i < step_count; i++)
                    {
                        DBG(SUB_AGENT, "==> %d\n", path[i].tile_id);
                    }
                }
            } TilePath;

            typedef struct
            {
                bool is_active;
                bool is_solved;
                uint8_t op_local_cg_id;
                uint8_t op_global_cg_id;
                TileID first_tile_id;
                TileID second_tile_id;
                AgentClaim first_tile_claim;
                AgentClaim second_tile_claim;

                ClusterGuaranteeSolution()
                {
                    is_active = false;
                    is_solved = false;
                    op_local_cg_id = 0;
                    op_global_cg_id = 0;
                    first_tile_id = 0;
                    second_tile_id = 0;
                }
            } ClusterGuaranteeSolution;

            typedef struct
            {
                ClusterGuaranteeSolution map[MAX_NUMBER_OF_GUARANTEES];
            } CGSolutionMap;

        public:
            ActorConstraintSolver();
            ~ActorConstraintSolver();
            std::pair<bool, os::agent::AgentClaim> solve(const ActorConstraint &constr,
                                                         const AgentClaim &availableResources);
            void *operator new(size_t size);
            void operator delete(void *c, size_t s);

        private:
            std::pair<bool, os::agent::AgentClaim> solve_operating_point(const ActorConstraint &actor_constraint,
                                                                         const AgentClaim &available_resources,
                                                                         const uint8_t &operating_point_id);

            bool backtrack(const ActorConstraint &actor_constraint,
                           AgentClaim &actual_satisfying_claim,
                           const uint8_t &operating_point_id,
                           const uint8_t &op_cluster_guarantee_count,
                           uint8_t &op_cluster_count,
                           TileMap &utilizable_tiles,
                           CGSolutionMap &op_solution);

            void prepare_backtracking(const ActorConstraint &actor_constraint,
                                      const uint8_t &operating_point_id,
                                      const uint8_t &op_cluster_guarantee_count,
                                      const uint8_t &op_cluster_count, uint8_t &risc_count, uint8_t &icore_count, uint8_t &tcpa_count,
                                      uint8_t &first_risc_id, uint8_t &first_icore_id, uint8_t &first_tcpa_id);
            uint8_t get_hop_count(TileID start_tile_id, TileID end_tile_id);
            uint8_t get_service_level(TileID start_tile_id, TileID end_tile_id);
            bool is_tile_of_type(TileID tileID, uint8_t type);
            bool are_all_tile_resources_on_claim(TileID tileID, AgentClaim availableResources);
        };
    } // namespace agent
} // namespace os
#endif // ACTORCONSTRAINTSOLVER_H
