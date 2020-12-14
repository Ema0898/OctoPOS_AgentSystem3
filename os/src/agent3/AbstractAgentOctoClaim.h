#ifndef ABSTRACTAGENTOCTOCLAIM_H
#define ABSTRACTAGENTOCTOCLAIM_H

#include "os/agent3/AgentRPCHeader.h" // forward declaration of important agent classes
#include "octo_types.h"
#include "os/res/ProxyClaim.h"

namespace os
{
	namespace agent
	{
		/**
 * \class AbstractAgentOctoClaim
 *
 * \brief Serves as abstract class for ProxyAgentOctoClaim which allows the an iLet spawned from an AgentOctoClaim to access its AgentOctoClaim from a remote tile. The proxy uses RPCs to access the AgentOctoClaim.
 *
 * Similar to the proxy pattern of the Gang of Four, this class AbstractAgentOctoClaim is used to let ProxyAgentOctoClaim implement functions of AgentOctoClaim differently. So (currently all) public function prototypes of AgentOctoClaim were copied to AbstractAgentOctoClaim as purely virtual functions. Naturally AgentOctoClaim has already implemented the functions. ProxyAgentOctoClaim implements the functions mainly by using RPCs to the tile where the iLet's AgentOctoClaim is defined and to remotely execute AgentOctoClaim's functions there and return its value back (to the calling ProxyAgentOctoClaim's tile).
 *
 * Supposed the AgentOctoClaim is created by main_ilet on tile 0, and spawns an iLet on tile 1. The iLet cannot directly access its AgentOctoClaim on tile 1 due to different TileLocalMemory of the tiles. Using the C interface, an iLet can create a new ProxyAgentOctoClaim and use it to access its AgentOctoClaim on its tile. AbstractAgentOctoClaim is needed to cast an agentclaim_t to an *AbstractAgentOctoClaim and call the functions on that *AbstractAgentOctoClaim thereafter. Sometimes one needs to downcast from AbstractAgentOctoClaim to AgentOctoClaim/ProxyAgentOctoClaim, funcitons asPAOC and asAOC are used for that.
 */
		class AbstractAgentOctoClaim
		{
		public:
			/**
	 * \brief Class destructor
	 *
	 * Empty at the moment.
	 */
			virtual ~AbstractAgentOctoClaim(){};

			/**
	 * \brief Implements the new operator which is needed for creating new instances of this class.
	 *
	 * Uses class AbstractAgentOctoClaimAllocator to allocate new instances of this class on the MPSoC's Tile Local Memory (TLM).
	 *
	 * \param size Size in bytes of the requested memory block.
	 *
	 * \return New instance of this class.
	 */
			void *operator new(size_t s) throw();

			/**
	 * \brief Implements the delete operator which is needed for deleting instances of this class.
	 *
	 * Uses class AbstractAgentOctoClaimAllocator to delete instances of this class on the MPSoC's Tile Local Memory (TLM).
	 *
	 * \param p AbstractAgentOctoClaim handle
	 */
			void operator delete(void *p);

			/**
	 * \brief Returns whether the instance of this class is a ProxyAgentOctoClaim (returns its address) or not (returns NULL). Purely virtual function, needs to be implemented by the subclass.
	 *
	 * Allows for distinguishing between a ProxyAgentOctoClaim and an AgentOctoClaim to enable downcasting from an AbstractAgentOctoClaim in the C interface. Function is needed because iRTSS currently does not implement dynamic casting.
	 *
	 * \return Pointer to a ProxyAgentOctoClaim which is NULL (if the instance is not a ProxyAgentOctoClaim) or an actual address of the ProxyAgentOctoClaim (if the instance is indeed a ProxyAgentOctoClaim).
	 */
			virtual ProxyAgentOctoClaim *asPAOC() = 0;

			/**
	 * \brief Returns whether the instance of this class is a AgentOctoClaim (returns its address) or not (returns NULL). Purely virtual function, needs to be implemented by the subclass.
	 *
	 * Allows for distinguishing between a ProxyAgentOctoClaim and an AgentOctoClaim to enable downcasting from an AbstractAgentOctoClaim in the C interface. Function is needed because iRTSS currently does not implement dynamic casting.
	 *
	 * \return Pointer to a AgentOctoClaim which is NULL (if the instance is not a AgentOctoClaim) or an actual address of the AgentOctoClaim (if the instance is indeed a AgentOctoClaim).
	 */
			virtual AgentOctoClaim *asAOC() = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getResourceCount, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual uint8_t getResourceCount() const = 0;

			/**
	 * ANUJ: Fill the description of this virtual function.
	 */
			virtual uint8_t getOperatingPointIndex() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getUcid, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual uint32_t getUcid() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::print, for more information about this method.
	 */
			virtual void print() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::adaptToAgentClaim_finish, for more information about this method.
	 *
	 * \param newClaim See subclasses for more information about this method.
	 * \param numberCoresGained See subclasses for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual int adaptToAgentClaim_finish(AgentClaim &newClaim, int numberCoresGained) = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::adaptToAgentClaim_prepare, for more information about this method.
	 *
	 * \param newClaim See subclasses for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual int adaptToAgentClaim_prepare(AgentClaim &newClaim) = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getAgentClaim, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual AgentClaim *getAgentClaim() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getConstraints, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual AgentConstraint *getConstraints() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getOwningAgent, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual AgentInstance *getOwningAgent() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getProxyClaim, for more information about this method.
	 *
	 * \param tileID See subclasses for more information about this method.
	 * \param type See subclasses for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual os::res::ProxyClaim *getProxyClaim(TileID tileID, HWType type) const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getQuantity, for more information about this method.
	 *
	 * \param tileID See subclasses for more information about this method.
	 * \param type See subclasses for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual uint8_t getQuantity(TileID tileID, HWType type) const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getTileCount, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual uint8_t getTileCount() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::getTileID, for more information about this method.
	 *
	 * \param iterator See subclasses for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual TileID getTileID(uint8_t iterator) const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::invadeAgentClaim_finish, for more information about this method.
	 */
			virtual void invadeAgentClaim_finish() = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::invadeAgentClaim_prepare, for more information about this method.
	 */
			virtual void invadeAgentClaim_prepare() = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::isEmpty, for more information about this method.
	 *
	 * \return See subclasses for more information about this method.
	 */
			virtual bool isEmpty() const = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::retreat, for more information about this method.
	 */
			virtual void retreat() = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::setConstraints, for more information about this method.
	 *
	 * \param constr See subclasses for more information about this method.
	 */
			virtual void setConstraints(AgentConstraint *constr) = 0;

			/**
	 * \brief This function prototype was copied from AgentOctoClaim to AbstractAgentOctoClaim to implement the proxy pattern of ProxyAgentOctoClaim. Purely virtual function, needs to be implemented by the subclass.
	 *
	 * See subclasses, especially ProxyAgentOctoClaim::setOwningAgent,  for more information about this method.
	 *
	 * \param ag See subclasses for more information about this method.
	 */
			virtual void setOwningAgent(AgentInstance *ag) = 0;
		}; // class AbstractAgentOctoClaim

	} // namespace agent
} // namespace os

#endif // ABSTRACTAGENTOCTOCLAIM_H
