#ifndef PROXYAGENTOCTOCLAIM_H
#define PROXYAGENTOCTOCLAIM_H

#include "os/agent3/AbstractAgentOctoClaim.h" // needed for inheritance
#include "os/agent3/Agent.h"									// needed for size_t etc., needed for creation/deletion of objects
#include "os/agent3/AgentRPCClient.h"					// needed for RPCs

namespace os
{
	namespace agent
	{
		/**
 * \class ProxyAgentOctoClaim
 *
 * \brief Provides a proxy to AgentOctoClaim which allows the AgentOctoClaim's spawned iLet to access its AgentOctoClaim from a remote tile. The proxy uses RPCs to access the AgentOctoClaim.
 *
 * The class ProxyAgentOctoClaim provides a proxy for an iLet to its AgentOctoClaim from any tile, in particular from a remote tile. A remote tile is a tile that is not the same tile where the AgentOctoClaim object is located.
 * For example: Supposed the AgentOctoClaim is created by main_ilet on tile 0, and spawns an iLet on tile 1. The iLet on tile 1 cannot directly (i.e. without using class ProxyAgentOctoClaim) access its AgentOctoClaim on tile 0 due to different Tile Local Memory (TLM) of the tiles. The memory address space of different tiles (and TLMs) is different.
 * \note Using the C interface, an iLet can create a new ProxyAgentOctoClaim and use it to access its AgentOctoClaim on its tile.
 * \note The class implements all of AbstractAgentOctoClaim's public purely virtual functions. These overridden functions are mainly the ones that AgentOctoClaim already implemented, with the exceptions of asPAOC() and asAOC() which were newly introduced to AgentOctoClaim and AbstractAgentOctoClaim to allow for downcasting from AbstractAgentOctoClaim* to its subclasses in the C interface. Also see file octo_types.h for more information.
 * \note This is how objects are located on the MPSoC: E.g. AgentOctoClaim A is created on tile 0, its Constraints object as well on tile 0; AgentInstance objects are on AgentSystem tile which is usually the last tile or the I/O tile; an iLet from A is spawned onto tile 1; the iLet creates a ProxyAgentOctoClaim object on tile 1, because it needs access to its AgentOctoClaim on tile 0. ProxyAgentOctoClaim will not copy or even deep copy the following objects onto tile 1: AgentClaim, AgentOctoClaim, AgentInstance, ProxyClaim. Those objects are left on their tiles. In short: ProxyAgentOctoClaim avoids inconsistencies by not further scattering an iLet's related objects onto different tiles than they already are.
 * \note The current implementation of this class assumes an ucid to be fixed during lifetime of the ProxyAgentOctoClaim instance. Reinvading changes the ucid of the proxied AgentOctoClaim. Therefore reinvading causes problems, see reinvadeSameConstr() for more information.
 */
		class ProxyAgentOctoClaim : public AbstractAgentOctoClaim
		{
		public:
			/**
	 * \brief Creates a new ProxyAgentOctoClaim.
	 *
	 * The constructor creates a new ProxyAgentOctoClaim. It takes two parameters, objects_tile and ucid and sets them as member variables. Then a DMA RPC is made to the os::agent::AgentSystem::AGENT_TILE using function Agent::proxyAOC_get_AOC_address with the constructor’s parameters. On the AGENT_TILE, the function looks for the AgentOctoClaim corresponding to the objects_tile and ucid. Once the AgentOctoClaim is found, its address is returned and stored in the originalAgentOctoClaim member variable of this ProxyAgentOctoClaim class.
	 *
	 * \param objects_tile The tile where the AgentOctoClaim is located at. -1 means invalid tile (not set yet).
	 * \param octo_ucid The unique id (ucid) of the AgentOctoClaim which we want to access.
	 */
			ProxyAgentOctoClaim(int objects_tile = -1, uint32_t octo_ucid = 0);

			/**
	 * \brief Class destructor; Sets class members to default values; Has no effect on the referenced AgentOctoClaim.
	 *
	 * Sets class members to default values. Has no effect on the referenced AgentOctoClaim. One may create and delete any number of ProxyAgentOctoClaims, it will have no effect on the AgentOctoClaim.
	 */
			~ProxyAgentOctoClaim();

			/**
	 * \brief Returns the objects_tile (tile where the proxy's AgentOctoClaim is located).
	 *
	 * \return Tile where the ProxyAgentOctoClaim's AgentOctoClaim is located (objects_tile).
	 */
			int getObjectsTile() const;

			/**
	 * \brief Reinvades with same Constraints by making an RPC to objects_tile, executes agent_claim_reinvade with the AgentOctoClaim object as parameter there and returns its value.
	 *
	 * Reinvades with the Constraints that are stored in the AgentOctoClaim via RPC to objects tile. This allows for reinvasion via C interface function agent_claim_reinvade from any tile. Reinvading causes a problem, because the ucid changes with a reinvade, even if the same Constraints are used. The reinvade itself is not forbidden with a ProxyAgentOctoClaim, but if any function is called on the same ProxyAgentOctoClaim after its reinvasion, function checkAgentOctoClaimAlive panics.
	 * \note Reinvasion results in a different ucid which is used for identification of the ProxyAgentOctoClaim’s AgentOctoClaim. In the current implementation, reinvasion is allowed from any tile using ProxyAgentOctoClaim’s function reinvadeSameConstr. This allows for changing resources from within the ProxyAgentOctoClaim’s iLet via C interface function agent_claim_reinvade. However after a reinvasion, if any of ProxyAgentOctoClaim's functions will be called again, an error is thrown by checkAgentOctoClaimAlive. It is possible that developers want to change the ProxyAgentOctoClaim class in one of the following ways:
	 * 1. Disallow reinvasion within a ProxyAgentOctoClaim completely. This can be implemented by throwing an error message in function reinvadeSameConstr. Function agent_claim_reinvade always leads to an error message when using a ProxyAgentOctoClaim then.
	 * 2. Allowing the ProxyAgentOctoClaim to adapt to the new AgentOctoClaim after a reinvade. This can be implemented via setting the new ucid and updating the other member variables of the ProxyAgentOctoClaim.
	 * \note What happens if the AgentOctoClaim is reinvaded with Constraints that result in not using the ProxyAgentOctoClaim’s tile anymore? Due to stickiness this case cannot occur. If not the whole tile is lost but only the core the iLet runs on, the ProxyAgentOctoClaim’s iLet is not terminated automatically in the current iRTSS. The same goes for retreating: Supposed the main_ilet runs on tile 0, creates an AgentOctoClaim and infects it to spawn an iLetB on tile 1. If iLetB performs a long calculation and the main_ilet does not wait for its signal and retreats iLetB’s AgentOctoClaim, iLetB still continues until completion. ILetB is not notified by the AgentSystem about the retreat operation. The ProxyAgentOctoClaim has no problem with neither the reinvasion nor the retreat scenario, because it checks for validity of the AgentOctoClaim and its ucid. The program terminates with an error message as soon as the iLet calls one of the ProxyAgentOctoClaim’s functions and checkAgentOctoClaimAlive() notices changes in the AgentOctoClaim.
	 *
	 * \return Absolute sum of resource changes after reinvading the AgentOctoClaim with same Constraints, e.g. losing 2, gaining 1 resource would return: 3.
	 */
			int reinvadeSameConstr();

			/**
	 * \brief Returns original AgentOctoClaim object if accessed on same tile as objects_tile, else panics.
	 *
	 * Returns original AgentOctoClaim object if this function is called on the same tile as objects_tile. If this function is called on a different tile than objects_tile, panics.
	 *
	 * \return Original AgentOctoClaim object if accessed on same tile as objects_tile, else panics.
	 */
			AgentOctoClaim *getOriginalAgentOctoClaim() const;

			/**
	 * \brief Returns a non-NULL pointer to this ProxyAgentOctoClaim instance.
	 *
	 * Returns a non-NULL pointer to this ProxyAgentOctoClaim instance. Allows for distinguishing between a ProxyAgentOctoClaim and an AgentOctoClaim to enable downcasting from an AbstractAgentOctoClaim in the C interface. Function is needed because iRTSS currently does not implement dynamic casting. Returns a non-NULL pointer, because this ProxyAgentOctoClaim is a ProxyAgentOctoClaim (and therefore can naturally be returned as one).
	 *
	 * \return Non-NULL pointer to this ProxyAgentOctoClaim instance.
	 */
			virtual ProxyAgentOctoClaim *asPAOC() { return this; }

			/**
	 * \brief Returns NULL.
	 *
	 * Allows for distinguishing between a ProxyAgentOctoClaim and an AgentOctoClaim in the C interface, needed for downcasting, because iRTSS currently does not implement dynamic casting.
	 * Returns NULL, because this ProxyAgentOctoClaim is not an AgentOctoClaim (and therefore cannot be returned as one).
	 *
	 * \return NULL.
	 */
			virtual AgentOctoClaim *asAOC() { return NULL; }

			/**
	 * \brief Panics if AgentOctoClaim has been changed.
	 *
	 * Panics with an error message if the AgentOctoClaim has changed. The stored objects tile and ucid variables are directly used (not their getter methods, because we want to call checkAgentOctoClaimAlive in the getters, too) to look for the AgentOctoClaim in the Agent tile; the same RPC function as in ProxyAgentOctoClaim’s constructor is used for that (Agent::proxyAOC_get_AOC_address). If the AgentOctoClaim cannot be found or is located at a different address than the stored originalAgentOctoClaim address, panics. Changes may happen if the AgentOctoClaim has been reinvaded or retreated. Function checkAgentOctoClaimAlive is called in the beginning of every function of this ProxyAgentOctoClaim class, except for: 1) The constructor, destructor and new/delete operators of ProxyAgentOctoClaim; 2) deleteDispatchClaimForRPC and getDispatchClaimForRPC functions.
	 * Reason why we need to check for AgentOctoClaim existance: One cannot detect AgentOctoClaim deletion (without manually signalling) in iRTSS, because iLets of retreated AgentOctoClaim's ProxyClaims are not stopped.
	 * If calling this checking function proves to be a serious performance bottleneck, and the programmer is sure he does not reinvade/retreat the AgentOctoClaim accidentally: One should provide a function (in the C interface, too) that sets and unsets checking subsequent ProxyAgentOctoClaim function calls for AgentOctoClaim validity. (That set/unset function then needs to be public because in C interface getUcid needs to check validity of ProxyAgentOctoClaim/AgentOctoClaim because the AgentOctoClaim may have been reinvaded, its ucid changed and ProxyAgentOctoClaim cannot notice it because of the stored ucid (which MUST be stored for the validity check itself)).
	 * Reinvading causes problems because of changing the AgentOctoClaim's ucid which leads to panicing in subsequent ProxyAgentOctoClaim function calls, because those function calls begin with a call of checkAgentOctoClaimAlive(). See reinvadeSameConstr() for more information about reinvasion with a ProxyAgentOctoClaim.
	 */
			void checkAgentOctoClaimAlive() const;

			// override base functions. Same order like in AgentOctoClaim in AgentClaim.h:

			/**
	 * \brief Implements the new operator which is needed for creating new instances of this class.
	 *
	 * Uses class ProxyAgentOctoClaimAllocator to allocate new instances of this class on the MPSoC's Tile Local Memory (TLM).
	 *
	 * \param size Size in bytes of the requested memory block.
	 *
	 * \return New instance of this class.
	 */
			void *operator new(size_t size) throw();

			/**
	 * \brief Implements the delete operator which is needed for deleting instances of this class.
	 *
	 * Uses class ProxyAgentOctoClaimAllocator to delete instances of this class on the MPSoC's Tile Local Memory (TLM).
	 *
	 * \param p ProxyAgentOctoClaim handle
	 */
			void operator delete(void *p);

			/**
	 * \brief Adapts to the AgentClaim that is passed as parameter.
	 *
	 * Adapts to the AgentClaim that is passed as parameter. If called from objects_tile, calls the function from its AgentOctoClaim. If called from a different tile, panics, because this involves an AgentClaim object on a tile different than objects_tile. Cannot do this via RPC, because this results in related objects being scattered on different tiles which leads to inconsistencies. See AgentOctoClaim for more information.
	 *
	 * \param newClaim AgentClaim to adapt to
	 * \param numberCoresGained number of cores gained
	 *
	 * \return Total sum of changes (i.e. losing 2, gaining 1 resource would return: 3)
	 */
			int adaptToAgentClaim_finish(AgentClaim &newClaim, int numberCoresGained);

			/**
	 * \brief Adapts to the AgentClaim that is passed as parameter.
	 *
	 * Adapts to the AgentClaim that is passed as parameter. If called from objects_tile, calls the function from its AgentOctoClaim. If called from a different tile, panics, because this involves an AgentClaim object on a tile different than objects_tile. Cannot do this via RPC, because this results in related objects being scattered on different tiles which leads to inconsistencies. See AgentOctoClaim for more information.
	 *
	 * \param newClaim AgentClaim to adapt to
	 *
	 * \return See AgentOctoClaim for more information.
	 */
			int adaptToAgentClaim_prepare(AgentClaim &newClaim);

			/**
	 * \brief Invasion phase function; Involves calling system functions in OctoPos and is necessary when preparing respectively finishing an invade operation.
	 *
	 * Invasion phase function. Involves calling system functions in OctoPos and is necessary when finishing an invade operation. In this class (ProxyAgentOctoClaim): If called from objects_tile, calls the function on its AgentOctoClaim. If called from a different tile, panics, because the AgentOctoClaim cannot be invaded from a different tile than objects_tile. The AgentOctoClaim has to be setup already in order to be able to be infected with an iLet and create and work with a ProxyAgentOctoClaim. So it is impossible that this invade-phase-function needs to be run from a distant tile within an iLet that runs and is associated with that AgentOctoClaim. Panics.
	 */
			void invadeAgentClaim_finish();

			/**
	 * \brief Invasion phase function; Involves calling system functions in OctoPos and is necessary when preparing respectively finishing an invade operation.
	 *
	 * Invasion phase function. Involves calling system functions in OctoPos and is necessary when preparing an invade operation. In this class (ProxyAgentOctoClaim): If called from objects_tile, calls the function on its AgentOctoClaim. If called from a different tile, panics, because the AgentOctoClaim cannot be invaded from a different tile than objects_tile. The AgentOctoClaim has to be setup already in order to be able to be infected with an iLet and create and work with a ProxyAgentOctoClaim. So it is impossible that this invade-phase-function needs to be run from a distant tile within an iLet that runs and is associated with that AgentOctoClaim. Panics.
	 */
			void invadeAgentClaim_prepare();

			/**
	 * \brief Retreats the ProxyAgentOctoClaim's AgentOctoClaim.
	 *
	 * Retreats the ProxyAgentOctoClaim's AgentOctoClaim. This releases the OctoPos invaded resources and informs the AgentInstance. In this class (ProxyAgentOctoClaim): If called from objects_tile, retreats the AgentOctoClaim using originalAgentOctoClaim. If called from a different tile, panics, because the ProxyAgentOctoClaim implementation is only usable for accessing its own AgentOctoClaim in an iLet. So retreating in this situation implicates retreating its own claim which is used in the iLet running on current tile which is not possible. Therefore no RPC is made to the objects tile; panics.
	 */
			void retreat();

			/**
	 * \brief Prints the AgentOctoClaim’s AgentClaim.
	 *
	 * Prints the AgentOctoClaim’s AgentClaim. Can be viewed only in debug mode of iRTSS. SUB_AGENT_ON needs to be set to 1 in the iRTSS configuration in order to activate debug messages. In this class (ProxyAgentOctoClaim): If called from objects_tile, prints the AgentClaim using originalAgentOctoClaim. If called from a different tile, makes a RPC to objects_tile and executes print on the AgentOctoClaim object there. Note: In the iRTSS x86 guest-mode visualization tool called Tiletail, the AgentClaim will be printed in the objects_tile and not ProxyAgentOctoClaim’s current tile.
	 */
			void print() const;

			/**
	 * \brief Returns the AgentOctoClaim’s ucid.
	 *
	 * Returns the AgentOctoClaim’s unique id called ucid. In this class (ProxyAgentOctoClaim): Returns the ProxyAgentOctoClaim’s ucid which corresponds to the AgentOctoClaim’s ucid. AgentOctoClaim’s ucid just returns its AgentClaim's ucid. So ProxyAgentOctoClaim, its AgentOctoClaim and its AgentClaim always return the same ucid.
	 *
	 * \return The AgentOctoClaim's ucid.
	 */
			uint32_t getUcid() const;

			/**
	 * \brief Returns the tile id at the specified iterator position.
	 *
	 * Returns the tile id at the specified iterator position. In this class (ProxyAgentOctoClaim): If called from objects tile, returns the AgentOctoClaim’s tile id using originalAgentOctoClaim. If called from a different tile, makes a RPC to objects_tile, executes getTileID on the AgentOctoClaim object there and returns its value.
	 *
	 * \note This function is described in class AgentOctoClaim as "this function doesn't make too much sense as it is..".
	 *
	 * \param iterator The specified iterator position
	 *
	 * \return The tile id at the specified iterator position.
	 */
			TileID getTileID(uint8_t iterator) const;

			/**
	 * \brief Returns the ProxyClaim of the specified tile and type.
	 *
	 * Returns the ProxyClaim of the specified tile and type. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s ProxyClaim using originalAgentOctoClaim. If called from a different tile, panics, because this leads to creating a ProxyClaim object on a different tile
than objects tile. Cannot do this via RPC because this results in related objects
being scattered on different tiles which leads to inconsistencies.
	 *
	 * \param tileID The tile id that is used to specify the ProxyClaim
	 * \param type The type (RISC, iCore, TCPA, ...) that is used to specify the ProxyClaim
	 *
	 * \return The ProxyClaim of the specified tile and type.
	 */
			os::res::ProxyClaim *getProxyClaim(TileID tileID, HWType type) const;

			/**
	 * \brief Returns the AgentInstance.
	 *
	 * Returns the AgentInstance. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s AgentInstance using originalAgentOctoClaim. If called from a different tile, panics, because this leads to creating an AgentInstance object on a different tile than objects_tile. Cannot do this via RPC because this results in related objects being scattered on different tiles which leads to inconsistencies.
	 *
	 * \return The AgentInstance
	 */
			AgentInstance *getOwningAgent() const;

			/**
	 * \brief Sets the AgentInstance.
	 *
	 * Sets the AgentInstance. In this class (ProxyAgentOctoClaim): If called from objects_tile, sets the AgentOctoClaim’s AgentInstance using originalAgentOctoClaim. If called from a different tile, panics, because this leads to setting an AgentInstance object on a different tile than objects_tile. Cannot do this via RPC because this results in related objects being scattered on different tiles which leads to inconsistencies.
	 *
	 * \param ag The AgentInstance handle which we want to set to this ProxyAgentOctoClaim
	 */
			void setOwningAgent(AgentInstance *ag);

			/**
	 * \brief Returns true if the AgentOctoClaim is empty.
	 *
	 * Returns whether the ProxyAgentOctoClaim's AgentOctoClaim contains any claimed resources (returns false) or not (returns true). In this class (ProxyAgentOctoClaim): If called from objects_tile, returns whether the AgentOctoClaim contains any claimed resources or not, using originalAgentOctoClaim. If called from a different tile, makes a RPC to objects_tile, executes isEmpty on the AgentOctoClaim object there and returns its value.
	 *
	 * \return True if the AgentOctoClaim does not, false if the proxied AgentOctoClaim does contain any resources.
	 */
			bool isEmpty() const;

			/**
	 * \brief Returns the number of all resources that are claimed by the AgentOctoClaim.
	 *
	 * Returns the number of all resources that are claimed by the AgentOctoClaim. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s number of resources using originalAgentOctoClaim. If called from a different tile, makes a RPC to objects_tile, executes getResourceCount on the AgentOctoClaim object there and returns its value.
	 *
	 * \return The number of all resources that are claimed by the AgentOctoClaim.
	 */
			uint8_t getResourceCount() const;

			/**
	 * ANUJ: Fill the description of this virtual function.
	 */
			uint8_t getOperatingPointIndex() const;

			/**
	 * \brief Returns the number of resources of the specified tile and type that are claimed by the ProxyAgentOctoClaim's AgentOctoClaim.
	 *
	 * Returns the number of resources of the specified tile and type that are claimed by the ProxyAgentOctoClaim's AgentOctoClaim. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s resources of the specified tile and type using originalAgentOctoClaim. If called from a different tile, makes a RPC to objects_tile, executes getQuantity on the AgentOctoClaim object there and returns its value.
	 *
	 * \param tileID The tile id
	 * \param type The type (RISC, iCore, TCPA, ...)
	 *
	 * \return The number of resources of the specified tile and type that are claimed by the ProxyAgentOctoClaim's AgentOctoClaim.
	 */
			uint8_t getQuantity(TileID tileID, HWType type) const;

			/**
	 * \brief Returns the AgentClaim.
	 *
	 * Returns the ProxyAgentOctoClaim's AgentOctoClaim's AgentClaim. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s AgentClaim using originalAgentOctoClaim. If called from a different tile, panics, because this leads to creating an AgentClaim object on a different tile than objects_tile. Cannot do this via RPC because this results in related objects being scattered on different tiles which leads to inconsistencies.
	 *
	 * \return The ProxyAgentOctoClaim's AgentOctoClaim's AgentClaim.
	 */
			AgentClaim *getAgentClaim() const;

			/**
	 * \brief Returns the AgentOctoClaim's Constraints.
	 *
	 * Returns the ProxyAgentOctoClaim's AgentOctoClaim's Constraints. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s AgentClaim using originalAgentOctoClaim. If called from a different tile, panics, because this leads to creating an Constraints object on a different tile than objects_tile. Cannot do this via RPC because this results in related objects being scattered on different tiles which leads to inconsistencies.
	 *
	 * \return The ProxyAgentOctoClaim's AgentOctoClaim's Constraints.
	 */
			AgentConstraint *getConstraints() const;

			/**
	 * \brief Sets the AgentOctoClaim's Constraints.
	 *
	 * Sets the ProxyAgentOctoClaim's AgentOctoClaim's Constraints. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s AgentClaim using originalAgentOctoClaim. If called from a different tile, panics, because this leads to associating an Constraints object on a different tile than objects_tile. Cannot do this via RPC because this results in related objects being scattered on different tiles which leads to inconsistencies.
	 *
	 * \param constr The Constraints to set
	 */
			void setConstraints(AgentConstraint *constr);

			/**
	 * \brief Returns the AgentOctoClaim's Constraints.
	 *
	 * Returns the number of tiles that are claimed by the ProxyAgentOctoClaim's AgentOctoClaim. In this class (ProxyAgentOctoClaim): If called from objects_tile, returns the AgentOctoClaim’s number of claimed tiles using originalAgentOctoClaim. If called from a different tile, makes a RPC to objects_tile, executes getTileCount on the AgentOctoClaim object there and returns its value.
	 *
	 * \return The AgentOctoClaim's Constraints.
	 */
			uint8_t getTileCount() const;

		private:
			/**
	 * \brief The tile where the original AgentOctoClaim is located at.
	 *
	 * The tile where the original AgentOctoClaim is located at.
	 */
			int objects_tile;

			/**
	 * \brief The ucid of the original AgentOctoClaim.
	 *
	 * The ucid of the original AgentOctoClaim. Needed to identify the correct AgentOctoClaim on objects_tile via RPC.
	 */
			uint32_t octo_ucid;

			/**
	 * \brief The address of the proxied AgentOctoClaim
	 *
	 * The address of the proxied AgentOctoClaim. Cannot be directly used if current tile is not same as the objects_tile; in this case RPCs to the objects_tile need to be made in order to access the AgentOctoClaim.
	 */
			uintptr_t originalAgentOctoClaim;

			/**
	 * \brief Returns a DispatchClaim which is needed for DMA RPC to the objects_tile. Remember to call deleteDispatchClaimForRPC after using this function to free resources again.
	 *
	 * Returns a DispatchClaim which is needed for DMA RPC to the objects_tile. Internally, does an invade with one processing element to objects_tile. During the invade, ProxyAgentOctoClaim's variale octoclaim_for_rpc is set. Remember to call deleteDispatchClaimForRPC after using this function to free resources again.
	 *
	 * \param objects_tile Tile id we want to make a DMA RPC to. Correct tile id values are >= 0.
	 *
	 * \return DispatchClaim which is needed for DMA RPC to the objects_tile.
	 */
			static os::res::DispatchClaim getDispatchClaimForRPC(int objects_tile = -1);

			/**
	 * \brief Frees all resources which were acquired by getDispatchClaimForRPC.
	 *
	 * Frees all resources which were acquired by getDispatchClaimForRPC.
	 */
			static void deleteDispatchClaimForRPC();

			/**
	 * \brief AgentOctoClaim object which is only temporarily used for RPCs
	 *
	 * AgentOctoClaim object which is only temporarily used for RPCs. The variable gets set by function getDispatchClaimForRPC. Gets set to NULL after each usage by method deleteDispatchClaimForRPC.
	 */
			static os::agent::AgentOctoClaim *octoclaim_for_rpc;
		}; // class ProxyAgentOctoClaim

	} // namespace agent
} // namespace os

#endif // PROXYAGENTOCTOCLAIM_H
