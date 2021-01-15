#ifndef AGENT_CONSTRAINT_H
#define AGENT_CONSTRAINT_H

#include "lib/kassert.h"
#include "lib/debug.h"
#include "lib/adt/Bitset.h"

#include "os/agent3/AgentMemory.h"

#include "os/agent3/AgentClaim.h"

#include "os/agent3/AbstractConstraint.h"
#include "os/agent3/AgentRPCHeader.h"
//#include "os/agent/AgentRPCClient.h"

namespace os
{
  namespace agent
  {

    enum class ConstrType : int
    {
      NONE = 0,
      AND,
      PEQUANTITY,
      DOWNEYSPEEDUP,
      TILE,
      STICKY,
      MALLEABILITY,
      APPCLASS,
      APPNUMBER,
      TILESHARING,
      VIPG,
      REINVADE,
      LOCALMEMORY,
      SEARCHLIMIT,
      MALLEABLERATING
    };

    //class AgentClaim;

    class AgentConstraint : public AbstractConstraint
    {
      friend class FlatConstraints;

    public:
      AgentConstraint(const ConstrType type, AgentConstraint *parent = NULL)
          : cType(type), parent(parent)
      {
        this->setDefaults();
      }

      virtual ~AgentConstraint()
      {
      }

      virtual void siftPool(AgentClaim &pool) const
      {
        return;
      }

      virtual bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const
      {
        return true;
      }

      /*
     * Checks whether the given claim violates this AgentConstraint.
     *
     * For this function, "violates" is defined, so that the violation can not be repaired by adding
     * further resources.
     * For example: If a claim has not enough resources of a certain type, this function returns false.
     * If a claim has too much resources of a certain type, this function returns true.
     *
     * \param claimNotFulfills indicates if the claim was tested false by the fulfilledBy method. If this parameter is false,
     * 			it does NOT mean that fulfilledBy returned true but that there is no information about using that method.
     */
      virtual bool violatedBy(const AgentClaim &claim, const bool claimNotFulfills = false, const bool malleableContext = false) const
      {
        if (claimNotFulfills)
        {
          return true;
        }
        return !this->fulfilledBy(claim);
      }

      virtual ResourceRating determineMaxRating() const
      {
        return 100;
      }

      /*
     * \param claimFulfills indicates if the claim was tested true by the fulfilledBy method. If this parameter is false,
	 * 			it does NOT mean that fulfilledBy returned false but that there is no information about using that method.
	 */
      virtual ResourceRating rateClaim(const AgentClaim &claim, const bool claimFulfills = false) const
      {
        if (claimFulfills || fulfilledBy(claim))
        {
          return 100;
        }
        else
        {
          return 0;
        }
      }

      /**
     * Allocates resources that the AgentConstraint is responsible for and that the given claim contains.
     *
     * Examples: PEQuantityConstraint allocates PEs, LocalMemoryConstraint allocates memory, ...
     */
      virtual bool claimResources(const AgentClaim &claim, AgentInstance *agent, int slot) const
      {
        return true;
      }

      virtual bool isOptional() const;
      void setOptional(const bool optional);

      virtual AgentConstraint *searchConstraint(const ConstrType type)
      {
        if (type == this->getCType())
        {
          return this;
        }
        else
        {
          return NULL;
        }
      }

      virtual const AgentConstraint *searchConstraint(const ConstrType type) const
      {
        if (type == this->getCType())
        {
          return this;
        }
        else
        {
          return NULL;
        }
      }

      /*
     * writes a copy of itself into buffer, if it is not larger than 'maxSize'.
     * Returns the number of bytes copied.
     */
      virtual size_t flatten(char *buffer, size_t maxSize) const = 0;

      virtual const AgentConstraint *__searchConstraintInternally(const ConstrType type, const AgentConstraint *caller) const
      {
        if (type == this->getCType())
        {
          return this;
        }
        else if (this->parent != NULL && this->parent != caller)
        {
          // if our parent called as, we don't call him because we like our recursions to terminate
          return this->parent->searchConstraintInternally(type);
        }
        else
        {
          return NULL;
        }
      }

      virtual bool isAgentConstraint() const;

      void *operator new(size_t s)
      {
        return AgentMemory::agent_mem_allocate(s);
      }

      void operator delete(void *c, size_t s)
      {
        AgentMemory::agent_mem_free(c);
      }

    private:
      void setDefaults();

      bool optional;

      ConstrType cType;

      /*
     * this hard coding is currently not really nice. In the future, it would be nice to find
     * a solid design to use the functions and pointers below, which would allow to instantiate
     * classes by their names. These could (for example) be stored in an array, indexed by the
     * ConstraintType.
     *
    static template<typename T> Constraint *instantiateDefaultConstraint(Constraint *parent) {
	return new T(parent);
    }
    static Constraint*(*)(Constraint*) defaultConstraintConstructors[DUMMY_LAST_CONSTRAINT];
    */

      static const uint8_t NUM_IMPLICIT_CONSTRAINTS;

    protected:
      ConstrType getCType() const
      {
        return this->cType;
      }

      virtual const AgentConstraint *searchConstraintInternally(const ConstrType type) const
      {
        return this->__searchConstraintInternally(type, this);
      }

      const AgentConstraint *getParent() const
      {
        return this->parent;
      }

      AgentConstraint *parent;

    public:
      bool isSticky() const;
      bool isMalleable() const;
      resize_handler_t getResizeHandler() const;
      resize_env_t getResizeEnvPointer() const;
      bool isTileShareable() const;
      bool isViPGBlocking() const;
      bool isTileAllowed(const os::agent::TileID tileID) const;
      int getAppClass() const;
      int getAppNumber() const;
      uint16_t getDowneyA() const;
      uint16_t getDowneySigma() const;
      os::agent::ResourceRating downey_rate(uint16_t n, uint16_t A, uint16_t s) const;
      bool isResourceAllowed(const os::agent::ResourceID &res) const;
      bool canLoseResource(const os::agent::AgentClaim &claim, const os::agent::ResourceID &res) const;
      int getMinOfType(const os::agent::HWType type) const;
      int getMaxOfType(const os::agent::HWType type) const;
      bool violatesMaxPE(const os::agent::AgentClaim &claim, const os::agent::HWType type) const;
      os::agent::ResourceRating rateAdditionalResource(os::agent::AgentClaim &claim,
                                                       const os::agent::ResourceID &res) const;
      os::agent::ResourceRating rateLosingResource(os::agent::AgentClaim &claim,
                                                   const os::agent::ResourceID &res) const;

      AgentConstraint &operator=(const AgentConstraint &rhs) = default;
    };

    class ConstraintList : public AgentConstraint
    {
      friend class FlatConstraints;

    public:
      class Iterator
      {

      public:
        Iterator(const ConstraintList::Iterator &it)
        {
          this->parent = it.parent;
          this->currentIndex = it.currentIndex;
        }

        Iterator(const ConstraintList *parent)
            : parent(parent)
        {
          this->currentIndex = 0;
        };

        Iterator(const ConstraintList *parent, const uint8_t pos)
            : ConstraintList::Iterator(parent)
        {
          if (pos > parent->itemsUsed)
          {
            DBG(SUB_AGENT, "Illegal constraint list. Last item: %d, itemsUsed: %d\n", pos, parent->itemsUsed);
            // pos is behind ConstraintList::end()
            //TODO: do something, as this is illegal
          }

          this->currentIndex = pos;
        };

        ConstraintList::Iterator &operator=(const Iterator &it)
        {
          this->parent = it.parent;
          this->currentIndex = it.currentIndex;
          return *this;
        }

        /* pre-increment */
        ConstraintList::Iterator &operator++()
        {
          if (this->currentIndex < this->parent->itemsUsed)
          {
            this->currentIndex++;
          }
          return *this;
        }

        /* post-increment */
        ConstraintList::Iterator operator++(int)
        {
          ConstraintList::Iterator t(*this);
          if (this->currentIndex < this->parent->itemsUsed)
          {
            this->currentIndex++;
          }
          return t;
        }

        bool operator==(const Iterator &it)
        {
          if (this->parent == it.parent && this->currentIndex == it.currentIndex)
          {
            return true;
          }
          else
          {
            return false;
          }
        }

        bool operator!=(const Iterator &it)
        {
          if (this->parent != it.parent || this->currentIndex != it.currentIndex)
          {
            return true;
          }
          else
          {
            return false;
          }
        }

        //Note that this only makes sense when both iterators have the same parent
        bool operator<(const Iterator &it)
        {
          if (this->parent == it.parent && this->currentIndex < it.currentIndex)
          {
            return true;
          }
          else
          {
            return false;
          }
        }

        //Note that this only makes sense when both iterators have the same parent
        bool operator>(const Iterator &it)
        {
          if (this->parent == it.parent && this->currentIndex > it.currentIndex)
          {
            return true;
          }
          else
          {
            return false;
          }
        }

        //Note that this only makes sense when both iterators have the same parent
        bool operator<=(const Iterator &it)
        {
          if (*this < it || *this == it)
          {
            return true;
          }
          else
          {
            return false;
          }
        }

        //Note that this only makes sense when both iterators have the same parent
        bool operator>=(const Iterator &it)
        {
          if (*this > it || *this == it)
          {
            return true;
          }
          else
          {
            return false;
          }
        }

        AgentConstraint *operator*()
        {
          return parent->list[this->currentIndex];
        }

      private:
        const ConstraintList *parent;
        uint8_t currentIndex;
      };

      ConstraintList(const ConstrType type)
          : AgentConstraint(type)
      {
        this->constructList();
        this->setConstraintCache(ConstrType::NONE, NULL);
      }

      ConstraintList(const ConstrType type, AgentConstraint *parent)
          : AgentConstraint(type, parent)
      {
        this->constructList();
        this->setConstraintCache(ConstrType::NONE, NULL);
      }

      ~ConstraintList()
      {
        ConstraintList::Iterator current = this->begin();
        ConstraintList::Iterator end = this->end();

        for (; current != end; ++current)
        {
          delete (*current);
        }
      }

      uint8_t getSize() const
      {
        return this->itemsUsed;
      }

      bool addConstraint(AgentConstraint *c);

      void setImplicitConstraints();

      virtual bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const = 0;
      virtual bool violatedBy(const AgentClaim &claim, const bool claimNotFulfills = false, const bool malleableContext = false) const = 0;
      virtual size_t flatten(char *buffer, size_t maxSize) const = 0;

      virtual AgentConstraint *searchConstraint(const ConstrType type);
      virtual const AgentConstraint *searchConstraint(const ConstrType type) const;

      virtual const AgentConstraint *__searchConstraintInternally(const ConstrType type, const AgentConstraint *caller) const = 0;

      ConstraintList::Iterator begin() const;
      ConstraintList::Iterator end() const;

    private:
      void setConstraintCache(ConstrType type, AgentConstraint *c)
      {
        constraintCache.type = type;
        constraintCache.c = c;
      }

#define DEFAULT_SIZE 16
      void constructList()
      {
        this->itemsUsed = 0;
        this->arraySize = DEFAULT_SIZE;
        uint8_t i;
        for (i = 0; i < this->arraySize; i++)
        {
          this->list[i] = NULL;
        }
      }

      uint8_t arraySize;
      uint8_t itemsUsed;
      AgentConstraint *list[DEFAULT_SIZE];

      struct
      {
        ConstrType type;
        AgentConstraint *c;
      } constraintCache;

    protected:
      AgentConstraint **getList()
      {
        return &list[0];
      }

      void setItemsUsed(uint8_t itemsUsed)
      {
        this->itemsUsed = itemsUsed;
      }
    };

    class AndConstraintList : public ConstraintList
    {

    public:
      AndConstraintList(AgentConstraint *parent, bool addImplicits = true)
          : ConstraintList(ConstrType::AND, parent)
      {
        if (addImplicits)
        {
          setImplicitConstraints();
        }
      }

      AndConstraintList(bool addImplicits = true)
          : AndConstraintList(NULL, addImplicits)
      {
      }

      void siftPool(AgentClaim &pool) const;

      bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const;
      bool violatedBy(const AgentClaim &claim, const bool claimNotFulfills = false, const bool malleableContext = false) const;

      ResourceRating determineMaxRating() const;

      ResourceRating rateClaim(const AgentClaim &claim, const bool claimFulfills = false) const;

      bool claimResources(const AgentClaim &claim, AgentInstance *agent, int slot) const;

      size_t flatten(char *buffer, size_t maxSize) const;

      virtual const AgentConstraint *__searchConstraintInternally(const ConstrType type, const AgentConstraint *caller) const;
    };

    class PEQuantityConstraint : public AgentConstraint
    {

    public:
      PEQuantityConstraint(AgentConstraint *parent)
          : AgentConstraint(ConstrType::PEQUANTITY, parent)
      {
        for (size_t i = 0; i < HWTypes; ++i)
        {
          this->peConstr[i].min = 0;
          this->peConstr[i].max = 0;
        }
      }

      PEQuantityConstraint()
          : PEQuantityConstraint(NULL)
      {
      }

      PEQuantityConstraint(const PEQuantityConstraint &obj)
          : PEQuantityConstraint(obj.parent)
      {
        for (uint8_t i = 0; i < HWTypes; ++i)
        {
          this->setQuantity(obj.getMin((HWType)i), obj.getMax((HWType)i), (HWType)i);
        }
      }

      bool canLoseResource(const os::agent::AgentClaim &claim, const ResourceID &resource) const;
      bool isResourceUsable(const ResourceID &resource) const;

      void siftPool(AgentClaim &pool) const;

      bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const;
      bool violatedBy(const AgentClaim &claim, const bool claimNotFulfills = false, const bool malleableContext = false) const;

      ResourceRating determineMaxRating() const;

      ResourceRating rateClaim(const AgentClaim &claim, const bool claimFulfills = false) const;

      bool claimResources(const AgentClaim &claim, AgentInstance *agent, int slot) const;

      size_t flatten(char *buffer, size_t maxSize) const;

      void setQuantity(uint8_t min, uint8_t max, const HWType type)
      {
        if (type >= HWTypes)
        {
          panic("PEQuantityConstraint::setQuantity: I don't know the given hardware type\n");
        }
        this->peConstr[type].min = min;
        this->peConstr[type].max = max;
      }

      uint8_t getMin(const HWType type) const
      {
        return this->peConstr[type].min;
      }

      uint8_t getMax(const HWType type) const
      {
        return this->peConstr[type].max;
      }

      PEQuantityConstraint &operator=(const PEQuantityConstraint &rhs) = default;

    private:
      struct
      {
        uint8_t min;
        uint8_t max;
      } peConstr[HWTypes];

      static const uint8_t EMPTY_CLAIM_RATING_MULTIPLIER;
    };

    class DowneySpeedupConstraint : public AgentConstraint
    {

    public:
      DowneySpeedupConstraint(const int a, const int sigma, AgentConstraint *parent)
          : AgentConstraint(ConstrType::DOWNEYSPEEDUP, parent), a(a), sigma(sigma)
      {
      }

      DowneySpeedupConstraint(const int a, const int sigma)
          : DowneySpeedupConstraint(a, sigma, NULL)
      {
      }

      DowneySpeedupConstraint(const DowneySpeedupConstraint &obj)
          : DowneySpeedupConstraint(obj.getA(), obj.getSigma(), obj.parent)
      {
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      int getA() const
      {
        return this->a;
      }

      int getSigma() const
      {
        return this->sigma;
      }

      void setA(const int a)
      {
        if (a < 100)
        {
          this->a = 100;
        }
        else if (a < 20000)
        {
          this->a = a;
        }
        else
        {
          this->a = 20000;
        }
      }

      void setSigma(const int sigma)
      {
        if (sigma < 5000)
        {
          this->sigma = sigma;
        }
        else
        {
          this->sigma = 5000;
        }
      }

      const static uint16_t defaultA;
      const static uint16_t defaultSigma;

      DowneySpeedupConstraint &operator=(const DowneySpeedupConstraint &rhs) = default;

    private:
      DowneySpeedupConstraint()
          : AgentConstraint(ConstrType::DOWNEYSPEEDUP)
      {
      }

      uint16_t a;
      uint16_t sigma;
    };

    class TileConstraint : public AgentConstraint
    {

    public:
      TileConstraint(AgentConstraint *parent)
          : AgentConstraint(ConstrType::TILE, parent)
      {
        this->setAll();
      }

      TileConstraint()
          : TileConstraint(NULL)
      {
      }

      TileConstraint(const TileConstraint &obj)
          : TileConstraint(obj.parent)
      {
        //The Constructor from the Constructor-List set all bits
        for (uint8_t i = 0; i < obj.numTiles(); ++i)
        {
          if (!obj.isTileAllowed(i))
          {
            this->disallowTile(i, false);
          }
        }
      }

      void siftPool(AgentClaim &pool) const;

      bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const;

      size_t flatten(char *buffer, size_t maxSize) const;

      /*
     * @param only: indicates, whether only the given tile is to be allowed, or whether the given
     * tile is to be added to the list of allowed tiles.
     */
      void allowTile(const uint8_t tileId, const bool only = true)
      {
        if (only)
        {
          clearAll();
        }
        uint8_t arrayIdx = tileId / 32;
        if (arrayIdx > arraySize)
        {
          panic("Invalid tile id. Id higher than maximum in tile constraint!");
        }
        uint32_t bits = 1UL << (tileId % 32);
        tiles[arrayIdx] |= bits;
      }

      /*
     * @param only: indicates, whether only the given tile is disallowed, or whether the given
     * tile is to be added to the list of disallowed tiles.
     */
      void disallowTile(const uint8_t tileId, const bool only = true)
      {
        if (only)
        {
          setAll();
        }
        uint8_t arrayIdx = tileId / 32;
        if (arrayIdx > arraySize)
        {
          panic("Invalid tile id. Id higher than maximum in tile constraint!");
        }
        uint32_t bits = 1UL << (tileId % 32);
        tiles[arrayIdx] &= ~bits;
      }

      void clearAll()
      {
        uint32_t bits = 0;
        for (int i = 0; i < arraySize; i++)
        {
          tiles[i] = bits;
        }
      }

      void setAll()
      {
        uint32_t bits = ~0;
        for (int i = 0; i < arraySize; i++)
        {
          tiles[i] = bits;
        }
      }

      void setBitmap(const uint32_t bitmap)
      {
        tiles[0] |= bitmap;
      }

      bool isTileAllowed(const uint8_t tileId) const
      {
        uint8_t arrayIdx = tileId / 32;
        if (arrayIdx > arraySize)
        {
          panic("Invalid tile id. Id higher than maximum in tile constraint!");
        }
        uint32_t bits = 1UL << (tileId % 32);
        return tiles[arrayIdx] & bits;
      }

      bool isTileDisallowed(const uint8_t tileId) const
      {
        return !this->isTileAllowed(tileId);
      }

      int numTiles() const
      {
        return 32 * arraySize;
      }

      TileConstraint &operator=(const TileConstraint &rhs) = default;

    private:
      /*
     * A set index marks the respective tile as allowed
     * 4x unit32_t is enough for systems with 128 tiles. For bigger systems the number has to be adjusted.
     */
      static const uint8_t arraySize = 4;
      uint32_t tiles[arraySize];
    };

    class StickyConstraint : public AgentConstraint
    {

    public:
      StickyConstraint(AgentConstraint *parent)
          : AgentConstraint(ConstrType::STICKY, parent), sticky(true)
      {
      }

      StickyConstraint()
          : StickyConstraint((const AgentConstraint *)NULL)
      {
      }

      StickyConstraint(const bool sticky, AgentConstraint *parent)
          : AgentConstraint(ConstrType::STICKY, parent), sticky(sticky)
      {
      }

      StickyConstraint(const bool sticky)
          : StickyConstraint(sticky, NULL)
      {
      }

      StickyConstraint(const StickyConstraint &obj)
          : StickyConstraint(obj.isSticky(), obj.parent)
      {
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      void setStickyness(const bool sticky)
      {
        this->sticky = sticky;
      }

      bool isSticky() const
      {
        return this->sticky;
      }

      StickyConstraint &operator=(const StickyConstraint &rhs) = default;

    private:
      bool sticky;
    };

    class MalleabilityConstraint : public AgentConstraint
    {

    public:
      MalleabilityConstraint(AgentConstraint *parent)
          : AgentConstraint(ConstrType::MALLEABILITY, parent), malleable(true)
      {
      }

      MalleabilityConstraint()
          : MalleabilityConstraint(NULL)
      {
      }

      MalleabilityConstraint(resize_handler_t resizeHandler, resize_env_t resizeEnvPtr, AgentConstraint *parent)
          : MalleabilityConstraint(parent)
      {
        kassert(resizeHandler && resizeEnvPtr);

        this->resizeHandler = resizeHandler;
        this->resizeEnvPtr = resizeEnvPtr;
      }

      MalleabilityConstraint(resize_handler_t resizeHandler, resize_env_t resizeEnvPtr)
          : MalleabilityConstraint(resizeHandler, resizeEnvPtr, NULL)
      {
      }

      MalleabilityConstraint(const MalleabilityConstraint &obj)
          : MalleabilityConstraint(obj.getResizeHandler(), obj.getResizeEnvPointer(), obj.parent)
      {
      }

      bool isMalleable() const
      {
        return this->malleable;
      }

      void setMalleable(bool malleable = true)
      {
        if (!malleable)
        {
          this->resizeHandler = NULL;
          this->resizeEnvPtr = NULL;
        }
        this->malleable = malleable;
      }

      void setResizeHandler(resize_handler_t resizeHandler)
      {
        this->resizeHandler = resizeHandler;
      }

      void setResizeEnvPtr(resize_env_t resizeEnvPtr)
      {
        this->resizeEnvPtr = resizeEnvPtr;
      }

      resize_handler_t getResizeHandler() const
      {
        return this->resizeHandler;
      }

      resize_env_t getResizeEnvPointer() const
      {
        return this->resizeEnvPtr;
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      MalleabilityConstraint &operator=(const MalleabilityConstraint &rhs) = default;

    private:
      bool malleable;

      resize_handler_t resizeHandler = NULL;
      resize_env_t resizeEnvPtr = NULL;
    };

    class AppClassConstraint : public AgentConstraint
    {

    public:
      AppClassConstraint(const int appClass, AgentConstraint *parent = NULL)
          : AgentConstraint(ConstrType::APPCLASS, parent), appClass(appClass)
      {
      }

      AppClassConstraint(const AppClassConstraint &obj)
          : AppClassConstraint(obj.getAppClass(), obj.parent)
      {
      }

      int getAppClass() const
      {
        return this->appClass;
      }

      void setAppClass(const int appClass)
      {
        this->appClass = appClass;
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      AppClassConstraint &operator=(const AppClassConstraint &rhs) = default;

    private:
      int appClass;
    };

    class AppNumberConstraint : public AgentConstraint
    {

    public:
      AppNumberConstraint(const int appNumber, AgentConstraint *parent = NULL)
          : AgentConstraint(ConstrType::APPNUMBER, parent), appNumber(appNumber)
      {
      }

      AppNumberConstraint(const AppNumberConstraint &obj)
          : AppNumberConstraint(obj.getAppNumber(), obj.parent)
      {
      }

      int getAppNumber() const
      {
        return this->appNumber;
      }

      void setAppNumber(const int appNumber)
      {
        this->appNumber = appNumber;
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      AppNumberConstraint &operator=(const AppNumberConstraint &rhs) = default;

    private:
      int appNumber;
    };

    class TileSharingConstraint : public AgentConstraint
    {

    public:
      TileSharingConstraint(AgentConstraint *parent = NULL, const bool shareable = false)
          : AgentConstraint(ConstrType::TILESHARING, parent), shareable(shareable)
      {
      }

      TileSharingConstraint(const bool shareable)
          : TileSharingConstraint(NULL, shareable)
      {
      }

      TileSharingConstraint(const TileSharingConstraint &obj)
          : TileSharingConstraint(obj.parent, obj.isTileShareable())
      {
      }

      bool isTileShareable() const
      {
        return this->shareable;
      }

      void setShareable(bool shareable = true)
      {
        this->shareable = shareable;
      }

      void siftPool(AgentClaim &pool) const;
      bool violatedBy(const AgentClaim &claim, const bool claimNotFulfills = false, const bool malleableContext = false) const;
      bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const;

      bool claimResources(const AgentClaim &claim, AgentInstance *agent, int slot) const;

      size_t flatten(char *buffer, size_t maxSize) const;

      TileSharingConstraint &operator=(const TileSharingConstraint &rhs) = default;

    private:
      bool shareable;
    };

    class ViPGConstraint : public AgentConstraint
    {

    public:
      ViPGConstraint(const bool enabled = true, AgentConstraint *parent = NULL)
          : AgentConstraint(ConstrType::VIPG, parent), vipgEnabled(enabled)
      {
      }

      ViPGConstraint(AgentConstraint *parent)
          : ViPGConstraint(true, parent)
      {
      }

      ViPGConstraint(const ViPGConstraint &obj)
          : ViPGConstraint(obj.isViPGEnabled(), obj.parent)
      {
      }

      bool isViPGEnabled() const
      {
        return this->vipgEnabled;
      }

      void setViPGEnabled(bool enabled = true)
      {
        this->vipgEnabled = enabled;
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      ViPGConstraint &operator=(const ViPGConstraint &rhs) = default;

    private:
      bool vipgEnabled;
    };

    class ReinvadeHandlerConstraint : public AgentConstraint
    {

    public:
      ReinvadeHandlerConstraint(AgentConstraint *parent)
          : AgentConstraint(ConstrType::REINVADE, parent)
      {
      }

      ReinvadeHandlerConstraint()
          : ReinvadeHandlerConstraint(NULL)
      {
      }

      ReinvadeHandlerConstraint(reinvade_handler_t reinvadeHandler, AgentConstraint *parent)
          : ReinvadeHandlerConstraint(parent)
      {
        kassert(reinvadeHandler);
        this->reinvadeHandler = reinvadeHandler;
      }

      ReinvadeHandlerConstraint(const ReinvadeHandlerConstraint &obj)
          : ReinvadeHandlerConstraint(obj.getReinvadeHandler(), obj.parent)
      {
      }

      void setReinvadeHandler(reinvade_handler_t reinvadeHandler)
      {
        this->reinvadeHandler = reinvadeHandler;
      }

      reinvade_handler_t getReinvadeHandler() const
      {
        return this->reinvadeHandler;
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      ReinvadeHandlerConstraint &operator=(const ReinvadeHandlerConstraint &rhs) = default;

    private:
      reinvade_handler_t reinvadeHandler = NULL;
    };

    class LocalMemoryConstraint : public AgentConstraint
    {

    public:
      LocalMemoryConstraint(AgentConstraint *parent, int min, int max)
          : AgentConstraint(ConstrType::LOCALMEMORY, parent), min(min), max(max)
      {
      }

      LocalMemoryConstraint(int min, int max)
          : LocalMemoryConstraint(NULL, min, max)
      {
      }

      bool fulfilledBy(const AgentClaim &claim, bool *improvable = NULL, const bool respect_max = true) const;

      bool claimResources(const AgentClaim &claim, AgentInstance *agent, int slot) const;

      size_t flatten(char *buffer, size_t maxSize) const;

      int getMin() const
      {
        return min;
      }

      int getMax() const
      {
        return max;
      }

      void setMin(int min)
      {
        this->min = min;
      }

      void setMax(int max)
      {
        this->max = max;
      }

      LocalMemoryConstraint &operator=(const LocalMemoryConstraint &rhs) = default;

    private:
      int min;
      int max;
    };

    class SearchLimitConstraint : public AgentConstraint
    {

    public:
      SearchLimitConstraint(AgentConstraint *parent = NULL)
          : AgentConstraint(ConstrType::SEARCHLIMIT, parent)
      {
        this->setDefault();
      }

      SearchLimitConstraint(const SearchLimitConstraint &obj)
          : SearchLimitConstraint(obj.parent)
      {
        this->timeLimit = obj.timeLimit;
        this->ratingLimit = obj.ratingLimit;
      }

      void setDefault();

      uint32_t getTimeLimit() const
      {
        return timeLimit;
      }

      void setTimeLimit(uint32_t timeLimit)
      {
        if (timeLimit >= 500 && timeLimit <= 600000)
        {
          this->timeLimit = timeLimit;
        }
      }

      uint8_t getRatingLimit() const
      {
        return ratingLimit;
      }

      void setRatingLimit(uint8_t ratingLimit)
      {
        if (ratingLimit >= 50 && ratingLimit <= 100)
        {
          this->ratingLimit = ratingLimit;
        }
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      SearchLimitConstraint &operator=(const SearchLimitConstraint &rhs) = default;

    private:
      // The time after which the solving algorithm aborts. If a solution is found till than, it is returned.
      uint32_t timeLimit;
      // The percentage of the maximum possible rating at which the solving algorithm stops.
      uint8_t ratingLimit;

      static const uint32_t DEFAULT_TIME_LIMIT;
      static const uint8_t DEFAULT_RATING_LIMIT;
    };

    class MalleableRatingConstraint : public AgentConstraint
    {

    public:
      MalleableRatingConstraint(AgentConstraint *parent = NULL)
          : AgentConstraint(ConstrType::MALLEABLERATING, parent)
      {
        this->setDefault();
      }

      MalleableRatingConstraint(const MalleableRatingConstraint &obj)
          : MalleableRatingConstraint(obj.parent)
      {
        this->activated = obj.activated;
        this->ratingMultiplier = obj.ratingMultiplier;
        this->claimingAgent = obj.claimingAgent;
      }

      void setDefault();

      ResourceRating rateClaim(const AgentClaim &claim, const bool claimFulfills = false) const;

      void setActivated(bool activated)
      {
        this->activated = activated;
      }

      void setRatingMultiplier(uint8_t ratingMultiplier)
      {
        this->ratingMultiplier = ratingMultiplier;
      }

      void setClaimingAgent(AgentInstance *claimingAgent)
      {
        this->claimingAgent = claimingAgent;
      }

      size_t flatten(char *buffer, size_t maxSize) const;

      MalleableRatingConstraint &operator=(const MalleableRatingConstraint &rhs) = default;

    private:
      bool activated;
      uint8_t ratingMultiplier;
      AgentInstance *claimingAgent;

      static const uint8_t DEFAULT_MALLEABLE_RATING_MULTIPLIER;
    };

    class FlatConstraints
    {

#define FLATCONSTR_SIZE (1 << 10)

    public:
      FlatConstraints()
          : dataUsed(0)
      {
      }

      /*
     * Consumes the given Constraint and stores it in its data container.
     * Returns true if everything went well
     *         false otherwise, e.g., when the constraint hierarchy is to big for the container.
     */
      bool flatten(AgentConstraint *source)
      {
        DBG(SUB_AGENT, "Flattening Constraint %p\n", source);
        this->dataUsed = source->flatten(data, FLATCONSTR_SIZE);

        return (this->dataUsed != 0);
      }

      /*
     * Unflattens the Constraint it holds.
     * This function creates new objects for each Constraints, i.e., it reserves space for the
     * Constraints on the heap and copies the content into these objects. While this takes
     * longer, the constraint hierarchy may be used after this object has been deleted.
     */
      inline void unflatten(AgentConstraint **target) const
      {
        this->unflattenOutside(target, 0);
      }

      void *operator new(size_t s)
      {
        return os::agent::AgentMemory::agent_mem_allocate(s);
      }

      void operator delete(void *c, size_t s)
      {
        os::agent::AgentMemory::agent_mem_free(c);
      }

    private:
      /*
     * Recursively builds a Constraint hierarchy out of flat data.
     * The new Constraint hierarchy will consist of newly created (i.e., dynamically allocated) objects
     *
     * Returns the size of the consumed data.
     *
     * @param target    Will hold a pointer to the "newly created" object.
     * @param offset    The offset (relative to the object's member 'data') of the Constraint to create
     */
      size_t unflattenOutside(AgentConstraint **target, size_t offset) const;

      /*
     * This chunk of memory has the following layout:
     * Each Constraint is preceeded by its type. Listmembers are appended to the list-object
     * It is basically an array of structs of different sizes, with the first member being of type ConstraintType
     * and the second member being the respective Constraint object
     */
      char data[FLATCONSTR_SIZE];

      size_t dataUsed;
    };

    class ResourceIndex
    {
    public:
      uint8_t orderIdx;
      uint8_t listIdx;
    };

    class SolverClaim
    {
    public:
      SolverClaim(AgentClaim aClaim, ResType HWOrder[], AgentClaim *agentResPool = NULL)
      {
        constructSolverClaim(aClaim, HWOrder, agentResPool);
      }

      ~SolverClaim()
      {
        for (uint8_t type = 0; type < HWTypes; ++type)
        {
          delete[] resList[type].list;
        }
      }

      void *operator new(size_t s)
      {
        return AgentMemory::agent_mem_allocate(s);
      }

      void operator delete(void *c, size_t s)
      {
        AgentMemory::agent_mem_free(c);
      }

      uint16_t debugPrint(ResType HWOrder[]);

      struct ResList
      {
        uint8_t coreCount;
        os::agent::ResourceID *list;
      } resList[HWTypes];

    private:
      void constructSolverClaim(AgentClaim aClaim, ResType HWOrder[], AgentClaim *agentResPool)
      {
        arraySize = aClaim.getResourceCount();
        for (uint8_t type = 0; type < HWTypes; ++type)
        {
          resList[type].coreCount = 0;
          resList[type].list = new os::agent::ResourceID[arraySize];
        }

        AgentClaim tempClaim;
        os::agent::ResourceID res;
        bool preferOwnRes = true;

        bool oneType = false;
        for (uint8_t type = 0; type < HWTypes; ++type)
        {
          if (aClaim.getTypeCount(type) > 0)
          {
            if (oneType)
            {
              preferOwnRes = false;
            }
            else
            {
              oneType = true;
            }
          }
        }

        // adding resources of agent's resource pool first to avoid using free resources
        if (agentResPool && preferOwnRes)
        {
          for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
          {
            if (!agentResPool->containsTile(res.tileID) || !aClaim.containsTile(res.tileID))
            {
              continue;
            }
            for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
            {
              if (agentResPool->contains(res) && aClaim.contains(res))
              {
                aClaim.remove(res);
                tempClaim.add(res);

                // add resource to correct type list
                os::agent::ResourceID newTileRes = res;
                for (uint8_t tileOrderNo = 0; tileOrderNo < HWTypes; ++tileOrderNo)
                {
                  if (HardwareMap[res.tileID][res.resourceID].type == HWOrder[tileOrderNo])
                  {
                    resList[tileOrderNo].list[resList[tileOrderNo].coreCount] = newTileRes;
                    resList[tileOrderNo].coreCount++;
                    break;
                  }
                }
              }
            }
          }
        }

        // add resources to solverClaim ordered by resource type
        for (uint8_t orderNo = 0; orderNo < HWTypes; ++orderNo)
        {
          for (res.tileID = 0; res.tileID < hw::hal::Tile::getTileCount(); ++res.tileID)
          {
            if (!aClaim.containsTile(res.tileID))
            {
              continue;
            }
            for (res.resourceID = 0; res.resourceID < MAX_RES_PER_TILE; ++res.resourceID)
            {

              if (aClaim.contains(res) &&
                  HardwareMap[res.tileID][res.resourceID].type == HWOrder[orderNo])
              {

                aClaim.remove(res);
                tempClaim.add(res);

                os::agent::ResourceID newRes = res;
                resList[orderNo].list[resList[orderNo].coreCount] = newRes;
                resList[orderNo].coreCount++;

                // add other resources from current tile first (in their resource list)
                os::agent::ResourceID tileRes;
                tileRes.tileID = res.tileID;
                for (tileRes.resourceID = 0; tileRes.resourceID < MAX_RES_PER_TILE; ++tileRes.resourceID)
                {
                  if (aClaim.contains(tileRes))
                  {
                    aClaim.remove(tileRes);
                    tempClaim.add(tileRes);

                    os::agent::ResourceID newTileRes = tileRes;
                    for (uint8_t tileOrderNo = 0; tileOrderNo < HWTypes; ++tileOrderNo)
                    {
                      if (HardwareMap[res.tileID][res.resourceID].type == HWOrder[tileOrderNo])
                      {
                        resList[tileOrderNo].list[resList[tileOrderNo].coreCount] = newTileRes;
                        resList[tileOrderNo].coreCount++;
                        break;
                      }
                    }
                  }
                }
                break;
              }

            } // for resourceID
          }   // for tileID
        }     // for order

        // add resources back to pool
        aClaim.addClaim(tempClaim);
      }

      uint8_t arraySize;
    };

    class ConstraintSolver
    {

    private:
      enum ConstraintSolverState
      {
        NEW,
        SOLVING,
        MOLDING,
        CLEANUP,
        SOLVED,
        FAILED
      };

    public:
      ConstraintSolver(AgentInstance *const parent, int slot, AgentConstraint *constr, AgentClaim pool)
          : myAgent(parent), slot(slot), state(NEW), constraints(constr), pool(pool), claimRating(0), malleabilityPools(NULL), numMalleabilityPools(0)
      {
        setHWOrder();
        determineMaxRating();
      }

      ConstraintSolver(AgentInstance *const parent, int slot, AgentConstraint *constr, AgentClaim pool, AgentClaim *claim)
          : myAgent(parent), slot(slot), state(NEW), constraints(constr), pool(pool), malleabilityPools(NULL), numMalleabilityPools(0)
      {
        setHWOrder();
        determineMaxRating();
        ResourceRating r = constraints->rateClaim(*claim);
        updateClaim(claim, r);
      }

      ~ConstraintSolver()
      {
        if (state == MOLDING || state == CLEANUP)
        {
          DBG(SUB_AGENT, "Warning: ConstraintSolver is deleted in MOLDING or CLEANUP state. That should not be what you want");
          cleanUpMalleabilityPools();
        }
      }

      void *operator new(size_t s)
      {
        return AgentMemory::agent_mem_allocate(s);
      }

      void operator delete(void *c, size_t s)
      {
        AgentMemory::agent_mem_free(c);
      }

      AgentClaim *getClaim() const
      {
        return claim;
      }

      void setClaim(AgentClaim *claim)
      {
        ResourceRating r = constraints->rateClaim(*claim);
        updateClaim(claim, r);
      }

      ResourceRating getRating() const
      {
        return this->claimRating;
      }

      AgentClaim &getPool()
      {
        return pool;
      }

      void setPool(AgentClaim pool)
      {
        this->pool = pool;
      }

      /**
     * Tries to extend the given AgentClaim to a solution for the Constraints
     *
     * Returns a ResourceRating which indicates how good the found solution is.
     * If no solution is found, 0 is returned.
     */
      ResourceRating solve();

      /**
     * \brief Creates the MalleabilityClaims needed for the claim.
     *
     * The call to this function is only valid when the Solver's state is MOLDING,
     * and can thus be called only once during one resolve process.
     */
      uint8_t createMalleabilityClaims(AgentMalleabilityClaim ***claims)
      {
        if (state != MOLDING)
        {
          return 0;
        }

        intersectPools();
        *claims = malleabilityPools;

        state = CLEANUP;
        return numMalleabilityPools;
      }

      void cleanup()
      {
        if (state != CLEANUP)
        {
          panic("cleanup called outside of state CLEANUP");
        }

        cleanUpMalleabilityPools();
        state = SOLVED;
      }

    private:
      void setHWOrder()
      {
        HWOrder[0] = ResType::TCPA;
        HWOrder[1] = ResType::iCore;
        HWOrder[2] = ResType::RISC;
        uint8_t type = 0;
        for (uint8_t orderNo = 3; orderNo < HWTypes; ++orderNo)
        {
          while (type < HWTypes && (type == ResType::TCPA ||
                                    type == ResType::iCore ||
                                    type == ResType::RISC))
          {
            type++;
          }
          HWOrder[orderNo] = static_cast<ResType>(type);
        }
      }

      void determineMaxRating();

      ResourceRating backtrack_OLD(AgentClaim &claim, ResourceID &res, bool malleability = false,
                                   ResourceRating oldRating = 0, uint8_t depth = 0);

      ResourceRating backtrack(AgentClaim &claim, ResourceIndex &resIdx,
                               ResourceRating oldRating = 0, uint8_t depth = 0);

      ResourceRating backtrackMalleableResources(AgentClaim &claim, ResourceID &res,
                                                 ResourceRating oldRating, uint8_t depth);

      inline bool moveFromTo(AgentMalleabilityClaim &from, AgentClaim &to, const os::agent::ResourceID &res)
      {
        if (from.take(res))
        {
          to.add(res);
          return true;
        }
        else
        {
          return false;
        }
      }

      inline bool moveFromTo(AgentClaim &from, AgentClaim &to, const os::agent::ResourceID &res)
      {
        if (!from.contains(res))
        {
          return false;
        }

        from.remove(res);
        to.add(res);

        return true;
      }

      bool moveFromPoolToClaim(const os::agent::ResourceID &res);

      inline bool moveFromClaimToPool(const os::agent::ResourceID &res)
      {
        return moveFromTo(*claim, pool, res);
      }

      void updateClaim(AgentClaim &claim, ResourceRating &rating)
      {
        *(this->claim) = claim;
        claim.setOwningAgent(myAgent);
        DBG(SUB_AGENT, "Setting new best claim (Rating: %" PRIu64 "):\n",
            rating);
        claim.print();
        claimRating = rating;
      }

      void updateClaim(AgentClaim *claim, ResourceRating &rating)
      {
        this->claim = claim;
        claim->setOwningAgent(myAgent);
        DBG(SUB_AGENT, "Setting new best claim (Rating: %" PRIu64 "):\n",
            rating);
        claim->print();
        claimRating = rating;
      }

      void cleanUpMalleabilityPools()
      {
        DBG(SUB_AGENT, "Going to clean up %" PRIu8 " malleability pools\n", numMalleabilityPools);
        for (uint8_t i = 0; i < numMalleabilityPools; ++i)
        {
          delete (malleabilityPools[i]);
        }

        numMalleabilityPools = 0;
        AgentMemory::agent_mem_free(malleabilityPools);
        malleabilityPools = NULL;
      }

      void intersectPools()
      {
        DBG(SUB_AGENT, "intersecting %" PRIu8 " malleability pools\n", numMalleabilityPools);
        uint8_t remainingPools = 0;
        for (uint8_t i = 0; i < numMalleabilityPools; ++i)
        {
          malleabilityPools[i]->intersect(*claim);

          /*
			 * we now check whether we used resources of this pool.
			 * If we didn't, we delete the pool.
			 * If we did, we move the pool further to the front.
			 * After the loop, this results in all used pools being stored at the beginning
			 * of the array, and all unused pools being deleted.
			 */
          if (malleabilityPools[i]->isEmpty())
          {
            delete malleabilityPools[i];
            malleabilityPools[i] = NULL;
          }
          else
          {
            malleabilityPools[remainingPools++] = malleabilityPools[i];
          }
        }
        DBG(SUB_AGENT, "%" PRIu8 " pools are left\n", remainingPools);

        numMalleabilityPools = remainingPools;
      }

      AgentInstance *myAgent;
      const int slot;
      ConstraintSolverState state;

      ResType HWOrder[HWTypes];

      AgentConstraint *constraints;
      ResourceRating maxRating;
      uint64_t startTime;
      uint64_t timeLimit;

      AgentClaim pool;
      SolverClaim *solverPool;
      AgentClaim *claim;
      ResourceRating claimRating;

      AgentMalleabilityClaim **malleabilityPools;
      uint8_t numMalleabilityPools;

      bool stopAlgo;
      bool violationAppeared;
      bool malleability;
    };

  } // namespace agent
} // namespace os

#endif // AGENT_CONSTRAINT_H
