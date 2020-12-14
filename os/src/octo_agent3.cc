#include "octo_agent3.h"

#include "os/dev/HWInfo.h"
#include "os/agent3/AgentRPCClient.h"
#include "os/agent3/AgentSystem.h"
#include "os/agent3/Agent.h"
#include "os/agent3/AbstractConstraint.h"
#include "os/agent3/AgentConstraint.h"
#include "os/agent3/ActorConstraint.h"
#include "os/agent3/ProxyAgentOctoClaim.h"

agent_t agent_agent_create(void)
{
  // creates a new Agent
  return os::agent::Agent::createAgent();
}

void agent_agent_delete(agent_t ag, uint8_t force)
{
  // deletes a new Agent
  os::agent::Agent::deleteAgent(static_cast<os::agent::AgentInstance *>(ag), static_cast<bool>(force != 0));
}

agent_t agent_claim_get_agent(agentclaim_t claim)
{
  if (!claim)
    panic("AgentOctoClaim == NULL");

  return static_cast<agent_t>(static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getOwningAgent());
}

constraints_t agent_claim_get_constr(agentclaim_t claim)
{
  if (!claim)
    panic("AgentOctoClaim == NULL");

  return static_cast<constraints_t>(static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getConstraints());
}

agentclaim_t agent_claim_invade_parentclaim(agentclaim_t parentclaim, constraints_t constr)
{
  if (!constr)
  {
    panic("agent_claim_invade_parentclaim: Constraints == NULL");
  }

  if (parentclaim)
  {
    return agent_claim_invade(agent_claim_get_agent(parentclaim), constr);
  }
  else
  {
    return agent_claim_invade(NULL, constr);
  }
}

os::agent::AgentInstance *getAgent(agent_t parentagent)
{
  os::agent::AgentInstance *ag;

  if (parentagent)
  {
    ag = static_cast<os::agent::AgentInstance *>(parentagent);
  }
  else
  {
    ag = os::agent::Agent::createAgent();
  }

  DBG(SUB_AGENT, "Using Agent %p\n", ag);

  if (ag == NULL)
  {
    panic("Agent == NULL at agent_claim_invade");
  }

  return ag;
}

agentclaim_t agent_claim_invade_or_constraints(agent_t parentagent, uint8_t constr_count, constraints_t constr[])
{
  os::agent::AndConstraintList superConstr = new os::agent::AndConstraintList();

  os::agent::PEQuantityConstraint *peqc = new os::agent::PEQuantityConstraint(&superConstr);
  superConstr.addConstraint(peqc);

  os::agent::TileConstraint *tc = new os::agent::TileConstraint(&superConstr);
  superConstr.addConstraint(tc);

  const os::agent::AppClassConstraint *helpacc = static_cast<const os::agent::AppClassConstraint *>(
      static_cast<os::agent::AgentConstraint *>(constr[0])->searchConstraint(
          os::agent::ConstrType::APPCLASS));

  if (helpacc)
  {
    os::agent::AppClassConstraint *acc = new os::agent::AppClassConstraint(helpacc->getAppClass(), &superConstr);
    superConstr.addConstraint(acc);
  }
  else
  {
    /*
	 * well.. it's rather unclear what this situation means. We could try our luck, and iterate through all
	 * Constraints in search for one which holds an AppClassConstraint.
	 * If we just don't have one, that shouldn't be too terrible.. This way of ORing Constraints is stupid anyway..
	 */
  }

  // find the max_max and the min_min of the different types.
  // The DowneyConstraint is not supported by this function, so it will use default values for rating.
  for (uint8_t type = 0; type < os::agent::HWTypes; type++)
  {
    int min_min = 512;
    int max_max = 0;
    for (int i = 0; i < constr_count; i++)
    {
      int helper;
      helper = static_cast<os::agent::AgentConstraint *>(constr[i])->getMinOfType((os::agent::HWType)type);
      if (helper < min_min)
      {
        min_min = helper;
      }
      helper = static_cast<os::agent::AgentConstraint *>(constr[i])->getMaxOfType((os::agent::HWType)type);
      if (helper > max_max)
      {
        max_max = helper;
      }
    }
    DBG(SUB_AGENT, "Type %d min %d max %d\n", type, min_min, max_max);
    peqc->setQuantity(min_min, max_max, (os::agent::HWType)type);
  }

  tc->clearAll();
  for (int i = 0; i < constr_count; i++)
  {
    const os::agent::TileConstraint *temptc = static_cast<const os::agent::TileConstraint *>(
        static_cast<os::agent::AgentConstraint *>(constr[i])->searchConstraint(
            os::agent::ConstrType::TILE));

    if (temptc)
    {
      int max = (tc->numTiles() > temptc->numTiles()) ? temptc->numTiles() : tc->numTiles();
      for (int j = 0; j < max; ++j)
      {
        if (temptc->isTileAllowed(j))
        {
          tc->allowTile(j, false);
        }
      }
    }
    else
    {
      /*
			 * in one of the Constraints, tiles are not set. Per default, that means that ALL tiles are allowed.
			 * We thus allow all tiles in the superConstr and move on
			 */
      tc->setAll();
      break;
    }
  }

  /*
     * TODO? nobody seems to use it (try grepping the src-directory for it
     * superConstr.SpeedupProfile=static_cast<os::agent::Constraints*>(constr[0])->SpeedupProfile;
     */

  // now that we have the superConstr, we let the agent system see, what is possible..
  // this might be a very, very expensive call to the AS ;-)

  os::agent::AgentInstance *ag = getAgent(parentagent);
  os::agent::AgentClaim *claim = new os::agent::AgentClaim();

  if (claim == NULL)
  {
    panic("new os::agent:AgentClaim failed!");
  }

  (*claim) = os::agent::Agent::invade_agent_constraints(ag, &superConstr);

  // now choose the set of constraints that gives the best overall rating
  int best_rating = 0;
  int best_rated = 0xff;

  for (int i = 0; i < constr_count; i++)
  {
    int rating = static_cast<os::agent::AgentConstraint *>(constr[i])->rateClaim(*claim);
    DBG(SUB_AGENT, "Constr %d give rating %d\n", i, rating);
    if (rating > best_rating)
    {
      best_rating = rating;
      best_rated = i;
    }
  }

  DBG(SUB_AGENT, "Choosing Constraintset %d with rating %d\n", best_rated,
      best_rating);

  if (best_rated == 0xff)
  { // oopsi.. the obtained claim is not useful for anyone...
    bool stillAlive = true;

    if (!claim->isEmpty())
    {
      stillAlive = os::agent::Agent::pure_retreat(claim->getOwningAgent(), claim->getUcid());
    }
    else
    {
      stillAlive = os::agent::Agent::checkAlive(claim->getOwningAgent());
    }

    if (!stillAlive)
    {
      os::agent::Agent::deleteAgent(claim->getOwningAgent());
    }

    delete claim;

    return NULL; // there must be a better way to signal error...
  }

  // okay, now we know which constraints would give the best rating. so we somehow fake a reinvade with exactly these constraints

  if (!claim->isEmpty())
  {
    os::agent::Agent::pure_retreat(claim->getOwningAgent(), claim->getUcid());
  }

  // we can rely on the good old agent_claim_invade now
  agentclaim_t retVal = agent_claim_invade(ag, constr[best_rated]); // returns an AbstractAgentOctoClaim handle (which is castable to AgentOctoClaim* via asAOC())

  return retVal;
}

// creates and returns a new claim. if parentclaim == NULL, a new Agent gets created
agentclaim_t agent_claim_invade(agent_t parentagent, constraints_t constr)
{
  if (!constr)
  {
    panic("agent_claim_invade: Constraints == NULL");
  }

  os::agent::AgentInstance *ag = getAgent(parentagent);
  os::agent::AgentClaim *claim = new os::agent::AgentClaim();

  if (!claim)
  {
    panic("agent_claim_invade: new os::agent:AgentClaim failed!");
  }

  (*claim) = os::agent::Agent::invade_agent_constraints(ag, (static_cast<os::agent::AgentConstraint *>(constr))); // do the Bargaining

  /* Skip everything related to AgentOctoClaims and OctoPOS, if the invade operation was unsuccessful. */
  if (claim->isEmpty())
  {
    if (claim->getOwningAgent() && !os::agent::Agent::checkAlive(claim->getOwningAgent()))
    {
      os::agent::Agent::deleteAgent(claim->getOwningAgent());
    }

    delete claim;

    return (NULL);
  }

  if (claim->getOwningAgent() != ag)
  {
    DBG(SUB_AGENT, "AgentClaim Belongs to %p, expected %p\n", claim->getOwningAgent(), ag);
    panic("agent_claim_invade: Wrong claim owner!");
  }

  DBG(SUB_AGENT, "agent_claim_invade: Transforming AgentClaim to AgentOctoClaim!\n");

  os::agent::AgentOctoClaim *octoclaim = new os::agent::AgentOctoClaim(*claim, static_cast<os::agent::AgentConstraint *>(constr), NULL); // initialize the Configuration of OctoPOS and CiC

  if (!octoclaim)
  {
    panic("agent_claim_invade: new os::agent:AgentOctoClaim failed!");
  }

  if (octoclaim->getOwningAgent() != ag)
  {
    DBG(SUB_AGENT, "OctoAgentClaim Belongs to %p, expected %p\n", octoclaim->getOwningAgent(), ag);
    panic("agent_claim_invade: Wrong claim owner!");
  }

  os::agent::Agent::register_AgentOctoClaim(ag, claim->getUcid(), octoclaim, os::res::DispatchClaim::getOwnDispatchClaim());

  // we *could* return to the application and do stuff here.. but instead
  //F/DBG(SUB_AGENT,"agent_claim_invade: Waiting for remote configurations to finish!\n");

  octoclaim->invadeAgentClaim_finish(); // force the Remote-Configuration of OctoPOS and CiC to finish.

  os::agent::Agent::signalOctoPOSConfigDone(static_cast<os::agent::AgentOctoClaim *>(octoclaim)->getOwningAgent(), static_cast<os::agent::AgentOctoClaim *>(octoclaim)->getUcid());

  //F/DBG(SUB_AGENT,"agent_claim_invade: Done!\n");

  return (os::agent::AbstractAgentOctoClaim *)octoclaim;
}

agentclaim_t agent_claim_invade_with_name(agent_t parentagent, constraints_t constr, const char *agent_name)
{
  agentclaim_t octoclaim = agent_claim_invade(parentagent, constr);
  set_agent_name(octoclaim, agent_name);
  return (os::agent::AbstractAgentOctoClaim *)octoclaim;
}

agentclaim_t agent_claim_get_initial(claim_t octoclaim)
{
#ifndef cf_board_x64native
  if (os::dev::HWInfo::Inst().getTileCount(os::dev::HWInfo::IOTile) == 0)
  {
    panic("No IO tile! Required for agent system.");
  }
#endif

  printf("Agent 3.0 Initialized Successfully\n");

  os::agent::AgentClaim *claim = new os::agent::AgentClaim();

  (*claim) = os::agent::Agent::getInitialAgentClaim();

  DBG(SUB_AGENT, "Got Initial Agent: %p\n", claim->getOwningAgent());

  os::agent::AndConstraintList *constr = new os::agent::AndConstraintList();
  os::agent::PEQuantityConstraint *peqc = new os::agent::PEQuantityConstraint(constr);
  os::agent::TileConstraint *tc = new os::agent::TileConstraint(constr);
  constr->addConstraint(peqc);
  constr->addConstraint(tc);
  tc->allowTile(0);
  peqc->setQuantity(1, 1, (os::agent::HWType)0);

  os::agent::AgentOctoClaim *octoagentclaim = new os::agent::AgentOctoClaim(
      *claim, static_cast<os::agent::AgentConstraint *>(constr), NULL, false);

  return (os::agent::AbstractAgentOctoClaim *)octoagentclaim;
}

agentclaim_t agent_claim_get_initial_with_name(claim_t octoclaim, const char *agent_name)
{
  agentclaim_t octoagentclaim = agent_claim_get_initial(octoclaim);
  set_agent_name((os::agent::AbstractAgentOctoClaim *)octoagentclaim, agent_name);
  return (os::agent::AbstractAgentOctoClaim *)octoagentclaim;
}

int agent_claim_reinvade(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_reinvade AgentOctoClaim == NULL");

  os::agent::AbstractAgentOctoClaim *aaoc = static_cast<os::agent::AbstractAgentOctoClaim *>(claim);
  if (aaoc->asAOC() == NULL && (aaoc->asPAOC() != NULL))
  {
    // it was a PAOC actually, so cast to PAOC
    os::agent::ProxyAgentOctoClaim *paoc = aaoc->asPAOC(); // need to cast to ProxyAgentOctoClaim here, not to AbstractAgentOctoClaim, because we want to call the PAOC-only function reinvadeSameConstr
    return paoc->reinvadeSameConstr();                     // if we're not on objects' tile, makes RPC to objects' tile and calls Cface function agent_claim_reinvade there with original AOC. In both cases, we'll land in the following else part then:
  }
  else if (aaoc->asPAOC() == NULL && (aaoc->asAOC() != NULL))
  {
    // it was a AOC actually, so cast to AOC
    os::agent::AgentOctoClaim *aoc = aaoc->asAOC();
    if (!aoc->getConstraints())
    {
      panic("NO CONSTRAINTS!");
    }
    return agent_claim_reinvade_constraints(claim, aoc->getConstraints());
  }
  else
  {
    panic("Cannot cast claim to either AgentOctoClaim or ProxyAgentOctoClaim");
  }
}

int agent_claim_reinvade_constraints(agentclaim_t claim, constraints_t constr)
{
  if (!claim)
    panic("agent_claim_reinvade_constraints AgentOctoClaim == NULL");
  if (!constr)
    panic("agent_claim_reinvade_constraints Constraints == NULL");

  // AgentOctoClaim object we need to register later (register_AgentOctoClaim). Can't do this with an AbstractAgentOctoClaim object because register_AgentOctoClaim does not want Abstract AOCs.
  // Case claim is AOC: Need to work with regular AgentOctoClaim object, because of register_AgentOctoClaim.
  // Case claim is PAOC, we're working on objects' tile: Need to work with regular AgentOctoClaim object, because of register_AgentOctoClaim.
  // Case claim is PAOC, we're on different tile than objects' tile: Need to produce error message as soon as possible (when calling getOriginalAgentOctoClaim).
  // So in all cases we're good.
  os::agent::AgentOctoClaim *cl = NULL;

  os::agent::AbstractAgentOctoClaim *aaoc = static_cast<os::agent::AbstractAgentOctoClaim *>(claim);
  if (aaoc->asAOC() == NULL && (aaoc->asPAOC() != NULL))
  {
    // it was a PAOC actually, so cast to PAOC
    os::agent::ProxyAgentOctoClaim *paoc = aaoc->asPAOC();
    cl = paoc->getOriginalAgentOctoClaim(); // need to work with original AOC object here because of register_AgentOctoClaim. Produces error message if working on not objects' tile
  }
  else if (aaoc->asPAOC() == NULL && (aaoc->asAOC() != NULL))
  {
    // it was a AOC actually, so cast to AOC
    cl = aaoc->asAOC();
  }
  else
  {
    panic("Cannot cast claim to either AgentOctoClaim or ProxyAgentOctoClaim");
  }

  // Info: The following code has been commented out for a long time. Maybe remove it?
  // reinvades 'claim' with new constraints. returns 0 if reinvade resulted in no change
  //if(!cl->isEmpty()) {

  // check if we are within the agentclaim.
  /*
     * This seems to be a supported use case, though.
     * We don't really care whether it's a reinvade
     * within a claim or not.
     * Let's hope the retreat operations don't fail, fingers crossed.
     */
  /*
    int inType;
    for(inType=0; inType<os::agent::HWTypes; inType++) {
	if(cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType) &&
		cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType)->getTag().value == os::res::DispatchClaim::getOwnDispatchClaim().getTag().value ) {
	    panic ("agent_claim_reinvade_constraints: reinvading from within a claim is not supported.");
	}
    }
    */

  DBG(SUB_AGENT, "Retreating AgentClaim %p within reinvade_constraints:\n",
      claim);
  cl->getAgentClaim()->print();

  uint32_t old_ucid = cl->getUcid();

  DBG(SUB_AGENT, "Invade AgentClaim %p within reinvade_constraints...\n",
      claim);

  os::agent::AgentClaim *newClaim = new os::agent::AgentClaim;
  (*newClaim) = os::agent::Agent::reinvade_agent_constraints(cl->getOwningAgent(), (static_cast<os::agent::AgentConstraint *>(constr)),
                                                             old_ucid);

  if (newClaim->isEmpty())
  {
    bool keep = true;

    if (old_ucid)
    {
      keep = os::agent::Agent::pure_retreat(cl->getOwningAgent(), old_ucid);
    }

    if (!keep)
    {
      os::agent::Agent::deleteAgent(newClaim->getOwningAgent());
    }

    cl->retreat();

    /* Delete the new AgentClaim object ... */
    delete newClaim;

    /* ... and the old AgentClaim object ... */
    delete cl->getAgentClaim();

    /* ... and the original, passed AgentOctoClaim object. */
    delete cl;

    return (-1);
  }

  DBG(SUB_AGENT, "Adapting AgentClaim %p to:\n", newClaim);
  newClaim->print();

  int retVal = cl->adaptToAgentClaim_prepare(*newClaim);
  retVal = cl->adaptToAgentClaim_finish(*newClaim, retVal);

  os::agent::Agent::register_AgentOctoClaim(newClaim->getOwningAgent(), newClaim->getUcid(), cl, os::res::DispatchClaim::getOwnDispatchClaim());

  os::agent::Agent::signalOctoPOSConfigDone(cl->getOwningAgent(), cl->getUcid());

  // take care: adaptToAgentClaim deletes the CURRENT AgentClaim which is represented in the AgentOctoClaim and sets newClaim as currentClaim
  cl->setConstraints(static_cast<os::agent::AgentConstraint *>(constr));

  if (old_ucid)
  {
    os::agent::Agent::pure_retreat(cl->getOwningAgent(), old_ucid);
  }

  // Calls reinvade handler iff the Constraints' reinvade handler is non-NULL.
  reinvade_handler_t reinvadeHandler = agent_constr_get_reinvade_handler(constr);
  if (reinvadeHandler != NULL)
  {
    DBG(SUB_AGENT, "Calling non-NULL reinvade handler within reinvade_constraints...\n");
    reinvadeHandler(); // calls the reinvade handler
  }

  return retVal;
}

void agent_claim_retreat(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_retreat AgentOctoClaim == NULL");

  // AgentOctoClaim object we need later for functions like getProxyClaim. Can't work with AbstractAgentOctoClaim because retreating its own claim from another tile does not make sense and it makes objects to delete only more complicated when working on objects' tile.
  // Case claim is AOC: Work with regular AgentOctoClaim object;
  // Case claim is PAOC, we're working on objects' tile: Need to work with regular AgentOctoClaim object.
  // Case claim is PAOC, we're on different tile than objects' tile: Need to produce error message, because one tries to retreat its claim from within.
  // We cannot RPC to a Proxy AOC's object_tile to retreat from that claim because this would skip the check for retreating one's claim from within (different getTag().value of that DispatchClaim).
  // So in all cases we're good.
  os::agent::AgentOctoClaim *cl = NULL;

  os::agent::AbstractAgentOctoClaim *aaoc = static_cast<os::agent::AbstractAgentOctoClaim *>(claim);
  if (aaoc->asAOC() == NULL && (aaoc->asPAOC() != NULL))
  {
    // it was a PAOC actually, so cast to PAOC
    os::agent::ProxyAgentOctoClaim *paoc = aaoc->asPAOC();
    cl = paoc->getOriginalAgentOctoClaim(); // need to work with original AOC object here because of register_AgentOctoClaim. Produces error message if working on not objects' tile
  }
  else if (aaoc->asPAOC() == NULL && (aaoc->asAOC() != NULL))
  {
    // it was a AOC actually, so cast to AOC
    cl = aaoc->asAOC();
  }
  else
  {
    panic("Cannot cast claim to either AgentOctoClaim or ProxyAgentOctoClaim");
  }

  DBG(SUB_AGENT, "agent_claim_retreat %p\n", claim);

  /* check if we are within the agentclaim. */
  for (int inType = 0; inType < os::agent::HWTypes; inType++)
  {
    if (cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType) &&
        cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType)->getTag().value == os::res::DispatchClaim::getOwnDispatchClaim().getTag().value)
    {

      DBG(SUB_AGENT, "Type %d, Tile %d, Tag %d/%d\n", inType,
          os::res::DispatchClaim::getOwnDispatchClaim().getTID(),
          cl->getProxyClaim(os::res::DispatchClaim::getOwnDispatchClaim().getTID(), (os::agent::HWType)inType)->getTag().value,
          os::res::DispatchClaim::getOwnDispatchClaim().getTag().value);

      panic("Application wants to retreat claim from within.");
    }
  }

  if (cl->getOwningAgent() == NULL)
  {
    panic("Retreating Claim which doesn't belong to any Agent.");
  }

  /* adapt OctoPOS configurations */
  cl->retreat();

  /* give away all resources */
  bool keep = os::agent::Agent::pure_retreat(cl->getOwningAgent(), cl->getUcid());

  if (!keep)
  {
    os::agent::Agent::deleteAgent(cl->getOwningAgent());
  }

  /* free local stuff */
  // TODO this seems to be buggy
  //delete cl->getAgentClaim();  // dangerous with the current way of retreating?
  //delete cl;  // dangerous with the current way of retreating?
  //delete static_cast<os::agent::ProxyAgentOctoClaim*>(claim); // dangerous with the current way of retreating?
}

void *agent_claim_get_proxyclaim_tile_type(agentclaim_t claim, tile_id_t tileID, res_type_t type)
{
  if (!claim)
    panic("agent_claim_get_proxyclaim_tile_type AgentOctoClaim == NULL");

  // returns proxyclaim required by infect for type type in tile tileID
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getProxyClaim(tileID, (ResType)type);
}

int agent_claim_get_pecount(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_get_pecount AgentOctoClaim == NULL");

  // returns number of resources in 'claim'
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getResourceCount();
}

int agent_claim_get_operatingpoint_index(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_get_operatingpoint_index AgentOctoClaim == NULL");

  // returns the index of operating point satisfied
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getOperatingPointIndex();
}

int agent_claim_get_pecount_type(agentclaim_t claim, res_type_t type)
{
  if (!claim)
    panic("agent_claim_get_pecount AgentOctoClaim == NULL");

  // returns number of resources in 'claim'
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getQuantity(os::agent::NOTILE, (ResType)type);
}

int agent_claim_get_pecount_tile(agentclaim_t claim, tile_id_t tileID)
{
  if (!claim)
    panic("agent_claim_get_pecount_tile AgentOctoClaim == NULL");

  // returns number of resources in tile tileID 'claim'
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getQuantity(tileID, TYPE_ALL);
}

int agent_claim_get_pecount_tile_type(agentclaim_t claim, tile_id_t tileID, res_type_t type)
{
  if (!claim)
    panic("agent_claim_get_pecount_tile_type AgentOctoClaim == NULL");

  // returns number of resources in tile tileID 'claim'
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getQuantity(tileID, (ResType)type);
}

int agent_claim_get_size(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_get_size AgentOctoClaim == NULL");

  // returns number of resources in 'claim'
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getResourceCount();
}

void agent_claim_print(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_print AgentOctoClaim == NULL");

  static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->print();
}

// returns number of tiles in 'claim'
int agent_claim_get_tilecount(agentclaim_t claim)
{
  if (!claim)
    panic("agent_claim_get_tilecount AgentOctoClaim == NULL");

  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getTileCount();
}

// get iterative TileID
int agent_claim_get_tileid_iterative(agentclaim_t claim, int iterator)
{
  if (!claim)
    panic("agent_claim_get_tileid_iterative AgentOctoClaim == NULL");

  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getTileID(iterator);
}

// Functions to interact with Constraints
constraints_t agent_constr_create(void)
{
  // creates Constraints struct (all ANDed) and initializes to default values
  os::agent::AndConstraintList *constr = new os::agent::AndConstraintList();
  return constr;
}

int agent_get_downey_sigma(constraints_t constr)
{
  if (!constr)
  {
    panic("agent_get_downey_sigma: Constraints == NULL");
  }

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  DowneySpeedupConstraint *dsc = static_cast<DowneySpeedupConstraint *>(lconstr->searchConstraint(ConstrType::DOWNEYSPEEDUP));
  if (!dsc)
  {
    panic("No DowneySpeedupConstraint found.");
  }

  return dsc->getSigma();
}

void agent_constr_delete(constraints_t constr)
{
  delete static_cast<os::agent::AgentConstraint *>(constr);
  constr = NULL;
}

void agent_constr_overwrite(constraints_t constrTarget, constraints_t additionalConstraints)
{
  panic("Unimplemented!");
  // TODO: copies two constraint-sets
}

void agent_constr_set_quantity(constraints_t constr, unsigned int minPEs, unsigned int maxPEs, res_type_t type)
{
  if (!constr)
    panic("agent_constr_set_quantity Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  PEQuantityConstraint *peqc = static_cast<PEQuantityConstraint *>(lconstr->searchConstraint(ConstrType::PEQUANTITY));
  if (!peqc)
  {
    peqc = new PEQuantityConstraint(lconstr);
    lconstr->addConstraint(peqc);
  }
  peqc->setQuantity(minPEs, maxPEs, (HWType)type);
}

void agent_constr_set_downey_speedup_parameter(constraints_t constr, int A, int sigma)
{
  if (!constr)
    panic("agent_constr_set_quantity Constraints == NULL");
  if (A <= 0)
    panic("A too small");
  if (A > SHRT_MAX)
    panic("A bigger than SHRT_MAX");
  if (sigma <= 0)
    panic("sigma too small");
  if (sigma > SHRT_MAX)
    panic("sigma bigger than SHRT_MAX");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  DowneySpeedupConstraint *dsc = static_cast<DowneySpeedupConstraint *>(lconstr->searchConstraint(ConstrType::DOWNEYSPEEDUP));
  if (!dsc)
  {
    dsc = new DowneySpeedupConstraint(A, sigma, lconstr);
    lconstr->addConstraint(dsc);
  }
  else
  {
    dsc->setA(A);
    dsc->setSigma(sigma);
  }
}

void agent_constr_set_tile(constraints_t constr, tile_id_t TileID)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  TileConstraint *tc = static_cast<TileConstraint *>(lconstr->searchConstraint(ConstrType::TILE));
  if (!tc)
  {
    tc = new TileConstraint(lconstr);
    lconstr->addConstraint(tc);
  }
  tc->allowTile(TileID);
}

void agent_constr_set_tile_bitmap(constraints_t constr, uint32_t bitmap)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  TileConstraint *tc = static_cast<TileConstraint *>(lconstr->searchConstraint(ConstrType::TILE));
  if (!tc)
  {
    tc = new TileConstraint(lconstr);
    lconstr->addConstraint(tc);
  }
  tc->setBitmap(bitmap);
}

void agent_constr_set_notontile(constraints_t constr, tile_id_t TileID)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  TileConstraint *tc = static_cast<TileConstraint *>(lconstr->searchConstraint(ConstrType::TILE));
  if (!tc)
  {
    tc = new TileConstraint(lconstr);
    lconstr->addConstraint(tc);
  }
  tc->disallowTile(TileID);
}

void agent_constr_set_appclass(constraints_t constr, int AppClass)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  AppClassConstraint *acc = static_cast<AppClassConstraint *>(lconstr->searchConstraint(ConstrType::APPCLASS));
  if (!acc)
  {
    acc = new AppClassConstraint(AppClass, lconstr);
    lconstr->addConstraint(acc);
  }
  else
  {
    acc->setAppClass(AppClass);
  }
}

void agent_constr_set_appnumber(constraints_t constr, int appNumber)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  AppNumberConstraint *anc = static_cast<AppNumberConstraint *>(lconstr->searchConstraint(ConstrType::APPNUMBER));
  if (!anc)
  {
    anc = new AppNumberConstraint(appNumber, lconstr);
    lconstr->addConstraint(anc);
  }
  else
  {
    anc->setAppNumber(appNumber);
  }
}

void agent_constr_set_stickyclaim(constraints_t constr, bool sticky)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  StickyConstraint *sc = static_cast<StickyConstraint *>(lconstr->searchConstraint(ConstrType::STICKY));
  if (!sc)
  {
    sc = new StickyConstraint(lconstr);
    lconstr->addConstraint(sc);
  }
  sc->setStickyness(sticky);
}

void agent_constr_set_vipg(constraints_t constr, uint8_t vipgEnable)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  ViPGConstraint *vipgc = static_cast<ViPGConstraint *>(lconstr->searchConstraint(ConstrType::VIPG));
  if (!vipgc)
  {
    vipgc = new ViPGConstraint(lconstr);
    lconstr->addConstraint(vipgc);
  }
  vipgc->setViPGEnabled(vipgEnable != 0);
}

void agent_constr_set_tile_shareable(constraints_t constr, uint8_t is_tile_shareable)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  TileSharingConstraint *tsc = static_cast<TileSharingConstraint *>(lconstr->searchConstraint(ConstrType::TILESHARING));
  if (!tsc)
  {
    tsc = new TileSharingConstraint(lconstr);
    lconstr->addConstraint(tsc);
  }
  tsc->setShareable(is_tile_shareable != 0);
}

void agent_constr_set_malleable(constraints_t constr, bool malleable, resize_handler_t resize_handler, resize_env_t resize_env)
{
  if (!constr)
  {
    panic("agent_constr_set_malleable: Constraints == NULL");
  }

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  MalleabilityConstraint *mc = static_cast<MalleabilityConstraint *>(lconstr->searchConstraint(ConstrType::MALLEABILITY));
  if (!mc)
  {
    mc = new MalleabilityConstraint(lconstr);
    lconstr->addConstraint(mc);
  }
  if (malleable)
  {
    mc->setMalleable();
    mc->setResizeHandler(resize_handler);
    mc->setResizeEnvPtr(resize_env);
  }
  else
  {
    mc->setMalleable(false);
  }
}

void agent_constr_set_local_memory(constraints_t constr, int min, int max)
{
  if (!constr)
    panic("Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  LocalMemoryConstraint *lmc = static_cast<LocalMemoryConstraint *>(lconstr->searchConstraint(ConstrType::LOCALMEMORY));

  if (!lmc)
  {
    lmc = new LocalMemoryConstraint(lconstr, min, max);
    lconstr->addConstraint(lmc);
  }
  else
  {
    lmc->setMin(min);
    lmc->setMax(max);
  }
}

void agent_constr_set_time_limit(constraints_t constr, int timeLimit)
{
  if (!constr)
    panic("agent_constr_set_quantity Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  SearchLimitConstraint *slc = static_cast<SearchLimitConstraint *>(lconstr->searchConstraint(ConstrType::SEARCHLIMIT));
  if (!slc)
  {
    slc = new SearchLimitConstraint(lconstr);
    lconstr->addConstraint(slc);
  }
  slc->setTimeLimit(timeLimit);
}

void agent_constr_set_rating_limit(constraints_t constr, int ratingLimit)
{
  if (!constr)
    panic("agent_constr_set_quantity Constraints == NULL");

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  SearchLimitConstraint *slc = static_cast<SearchLimitConstraint *>(lconstr->searchConstraint(ConstrType::SEARCHLIMIT));
  if (!slc)
  {
    slc = new SearchLimitConstraint(lconstr);
    lconstr->addConstraint(slc);
  }
  slc->setRatingLimit(ratingLimit);
}

/* Testing functions only! Do not use! */
void agent_stresstest_agentclaim()
{
  os::agent::AgentConstraint *constr = new os::agent::AndConstraintList();
  if (!constr)
  {
    panic("Could not allocate new os::agent::Constraints!");
  }

  for (;;)
  {
    os::agent::AgentClaim *claim = new os::agent::AgentClaim();

    if (!claim)
    {
      panic("Could not allocate new os::agent::AgentClaim!");
    }

    delete (claim);
  }

  delete (constr);
}

void agent_stresstest_agentoctoclaim()
{
  os::agent::AgentConstraint *constr = new os::agent::AndConstraintList();
  if (!constr)
  {
    panic("Could not allocate new os::agent::Constraints!");
  }

  os::agent::AgentClaim *claim = new os::agent::AgentClaim();
  if (!claim)
  {
    panic("Could not allocate new os::agent::Claim!");
  }

  for (;;)
  {
    os::agent::AgentOctoClaim *octoclaim = new os::agent::AgentOctoClaim(*claim, constr, NULL);

    if (!octoclaim)
    {
      panic("Could not allocate new os::agent::AgentOctoClaim!");
    }

    delete (octoclaim);
  }

  delete (constr);
  delete (claim);
}

void agent_constr_set_reinvade_handler(constraints_t constr, reinvade_handler_t reinvade_handler)
{
  if (!constr)
  {
    panic("agent_constr_set_reinvade_handler: Constraints == NULL");
  }

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  ReinvadeHandlerConstraint *rc = static_cast<ReinvadeHandlerConstraint *>(lconstr->searchConstraint(ConstrType::REINVADE));
  if (!rc)
  {
    rc = new ReinvadeHandlerConstraint(lconstr);
    rc->setReinvadeHandler(reinvade_handler);
    lconstr->addConstraint(rc);
  }
  else
  {
    rc->setReinvadeHandler(reinvade_handler);
  }
}

reinvade_handler_t agent_constr_get_reinvade_handler(constraints_t constr)
{
  if (!constr)
  {
    panic("agent_constr_get_reinvade_handler: Constraints == NULL");
  }

  using namespace os::agent;

  ConstraintList *lconstr = static_cast<ConstraintList *>(constr);

  ReinvadeHandlerConstraint *rc = static_cast<ReinvadeHandlerConstraint *>(lconstr->searchConstraint(ConstrType::REINVADE));
  if (!rc)
  {
    return NULL;
  }
  else
  {
    return rc->getReinvadeHandler();
  }
}

constraints_t agent_actor_constraint_create(uint8_t number_of_clusters, uint8_t number_of_cluster_guarantees, uint8_t number_of_operating_points)
{
  os::agent::ActorConstraint *actorConstraint = new os::agent::ActorConstraint(number_of_clusters, number_of_cluster_guarantees, number_of_operating_points);

  return actorConstraint;
}

uint8_t agent_actor_constraint_add_cluster(constraints_t actorConstraint, res_type_t type)
{
  if (!actorConstraint)
  {
    panic("agent_actor_constraint_add_cluster: ActorConstraint* == NULL");
  }
  return static_cast<os::agent::ActorConstraint *>(actorConstraint)->add_cluster(type + 1);
}

uint8_t agent_actor_constraint_add_cluster_guarantee(constraints_t actorConstraint, uint8_t firstClusterType, uint8_t secondClusterType, uint8_t hopDistance, uint8_t serviceLevel)
{
  if (!actorConstraint)
  {
    panic("agent_actor_constraint_add_cluster_guarantee: ActorConstraint* == NULL");
  }
  return static_cast<os::agent::ActorConstraint *>(actorConstraint)->add_cluster_guarantee(firstClusterType, secondClusterType, hopDistance, serviceLevel);
}

uint8_t agent_actor_constraint_add_operating_point(constraints_t actorConstraint, uint8_t number_of_cluster_guarantees, uint8_t number_of_clusters)
{
  if (!actorConstraint)
  {
    panic("agent_actor_constraint_add_operating_point: ActorConstraint* == NULL");
  }
  return static_cast<os::agent::ActorConstraint *>(actorConstraint)->add_operating_point(number_of_cluster_guarantees, number_of_clusters);
}

void agent_actor_constraint_add_cluster_to_operating_point(constraints_t actorConstraint, uint8_t op_id, uint8_t c_id)
{
  if (!actorConstraint)
  {
    panic("agent_actor_constraint_add_cluster_guarantee_to_operating_point: ActorConstraint* == NULL");
  }
  static_cast<os::agent::ActorConstraint *>(actorConstraint)->add_cluster_to_operating_point(op_id, c_id);
}

void agent_actor_constraint_add_cluster_guarantee_to_operating_point(constraints_t actorConstraint, uint8_t op_id, uint8_t cg_id)
{
  if (!actorConstraint)
  {
    panic("agent_actor_constraint_add_cluster_guarantee_to_operating_point: ActorConstraint* == NULL");
  }
  static_cast<os::agent::ActorConstraint *>(actorConstraint)->add_cluster_guarantee_to_operating_point(op_id, cg_id);
}

// creates and returns a new claim. if parentclaim == NULL, a new Agent gets created
agentclaim_t agent_claim_actor_constraint_invade(agent_t parentagent, constraints_t actorConstraint)
{
  if (!actorConstraint)
  {
    panic("agent_claim_actor_constraint_invade: OperatingPoint* == NULL");
  }

  os::agent::AgentInstance *ag = getAgent(parentagent);
  os::agent::AgentClaim *claim = new os::agent::AgentClaim();

  if (!claim)
  {
    panic("agent_claim_actor_constraint_invade: new os::agent:AgentClaim failed!");
  }
  // do the Bargaining
  (*claim) = os::agent::Agent::invade_actor_constraints(ag, (static_cast<os::agent::ActorConstraint *>(actorConstraint)));

  /* Skip everything related to AgentOctoClaims and OctoPOS, if the invade operation was unsuccessful. */
  if (claim->isEmpty())
  {

    if (claim->getOwningAgent() && !os::agent::Agent::checkAlive(claim->getOwningAgent()))
    {
      os::agent::Agent::deleteAgent(claim->getOwningAgent());
    }

    delete claim;

    return (NULL);
  }

  if (claim->getOwningAgent() != ag)
  {
    DBG(SUB_AGENT, "AgentClaim Belongs to %p, expected %p\n", claim->getOwningAgent(), ag);
    panic("agent_claim_actor_constraint_invade: Wrong claim owner!");
  }

  DBG(SUB_AGENT, "agent_claim_invade: Transforming AgentClaim to AgentOctoClaim!\n");

  // initialize the Configuration of OctoPOS and CiC
  os::agent::AgentOctoClaim *octoclaim = new os::agent::AgentOctoClaim(*claim, NULL, static_cast<os::agent::ActorConstraint *>(actorConstraint));

  if (!octoclaim)
  {
    panic("agent_claim_actor_constraint_invade: new os::agent:AgentOctoClaim failed!");
  }

  if (octoclaim->getOwningAgent() != ag)
  {
    DBG(SUB_AGENT, "OctoAgentClaim Belongs to %p, expected %p\n", octoclaim->getOwningAgent(), ag);
    panic("agent_claim_actor_constraint_invade: Wrong claim owner!");
  }

  os::agent::Agent::register_AgentOctoClaim(ag, claim->getUcid(), octoclaim, os::res::DispatchClaim::getOwnDispatchClaim());

  // we *could* return to the application and do stuff here.. but instead
  //F/DBG(SUB_AGENT,"agent_claim_invade: Waiting for remote configurations to finish!\n");

  octoclaim->invadeAgentClaim_finish(); // force the Remote-Configuration of OctoPOS and CiC to finish.

  os::agent::Agent::signalOctoPOSConfigDone(static_cast<os::agent::AgentOctoClaim *>(octoclaim)->getOwningAgent(), static_cast<os::agent::AgentOctoClaim *>(octoclaim)->getUcid());

  //F/DBG(SUB_AGENT,"agent_claim_invade: Done!\n");

  return (os::agent::AbstractAgentOctoClaim *)octoclaim;
}

agentclaim_t agent_proxy_get_proxyagentoctoclaim(int objects_tile, uint32_t octo_ucid)
{
  if (objects_tile < 0)
  {
    panic("agent_proxy_get_proxyagentoctoclaim: objects_tile < 0");
  }

  os::agent::ProxyAgentOctoClaim *proxy_aoc = new os::agent::ProxyAgentOctoClaim(objects_tile, octo_ucid);
  DBG(SUB_AGENT, "agent_proxy_get_proxyagentoctoclaim: Got ProxyAgentOctoClaim with objects_tile %i and octo_ucid %" PRIu32 "\n", proxy_aoc->getObjectsTile(), proxy_aoc->getUcid());

  return (os::agent::AbstractAgentOctoClaim *)proxy_aoc; // needs to be casted to AAOC like all other functions here that return an agentclaim_t. AAOC is what every C interface function that has a agentclaim_t expects to cast to.
}

void agent_proxy_delete_proxyagentoctoclaim(agentclaim_t proxy_agentoctoclaim)
{
  os::agent::AbstractAgentOctoClaim *aaoc = static_cast<os::agent::AbstractAgentOctoClaim *>(proxy_agentoctoclaim);
  if (aaoc->asAOC() == NULL && (aaoc->asPAOC() != NULL))
  {
    // it was a PAOC actually, so delete it
    delete aaoc->asPAOC();
    proxy_agentoctoclaim = NULL;
  }
  else
  {
    panic("agent_proxy_delete_proxyagentoctoclaim: proxy_agentoctoclaim is not castable to ProxyAgentOctoClaim. Can only work with ProxyAgentOctoClaims here.");
  }
}

int agent_proxy_get_objectstile(agentclaim_t proxy_agentoctoclaim)
{
  if (!proxy_agentoctoclaim)
  {
    panic("agent_proxy_get_objectstile: proxy_agentoctoclaim == NULL");
  }

  os::agent::ProxyAgentOctoClaim *paoc = NULL;
  os::agent::AbstractAgentOctoClaim *aaoc = static_cast<os::agent::AbstractAgentOctoClaim *>(proxy_agentoctoclaim);
  if (aaoc->asAOC() == NULL && (aaoc->asPAOC() != NULL))
  {
    // it was a PAOC actually, so cast to PAOC
    paoc = aaoc->asPAOC();
  }
  else
  {
    panic("agent_proxy_get_objectstile: proxy_agentoctoclaim is not castable to ProxyAgentOctoClaim. Can only work with ProxyAgentOctoClaims here.");
  }

  int objects_tile = paoc->getObjectsTile();
  DBG(SUB_AGENT, "agent_proxy_get_objectstile: ProxyAgentOctoClaim's objects_tile is %i\n", objects_tile);

  return objects_tile;
}

uint32_t agent_claim_get_ucid(agentclaim_t claim)
{
  if (!claim)
  {
    panic("agent_claim_get_ucid: claim == NULL");
  }

  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->getUcid();
}

bool agent_claim_isempty(agentclaim_t claim)
{
  if (!claim)
  {
    panic("agent_claim_isempty: claim == NULL");
  }
  return static_cast<os::agent::AbstractAgentOctoClaim *>(claim)->isEmpty();
}

void set_agent_name(agentclaim_t claim, const char *agent_name)
{
  os::agent::AgentInstance *ag = static_cast<os::agent::AgentInstance *>(agent_claim_get_agent(claim));
  os::agent::Agent::setAgentName(ag, agent_name);
}

int get_agent_name(agentclaim_t claim, char buffer[], size_t size)
{
  os::agent::AgentInstance *ag = static_cast<os::agent::AgentInstance *>(agent_claim_get_agent(claim));
  if (os::agent::Agent::getAgentName(ag, buffer, size) == 0)
    return 0;
  else
    return -1;
}
