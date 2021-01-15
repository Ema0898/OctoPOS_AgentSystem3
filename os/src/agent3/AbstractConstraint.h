#ifndef OS_AGENT_CONSTRAINTS_H
#define OS_AGENT_CONSTRAINTS_H

#include <stddef.h>

namespace os {
namespace agent {


class AbstractConstraint{
public:
    /**
     * \brief Class destructor
     */
    virtual ~AbstractConstraint();

    /**
     * \brief Implements the new operator which is needed for creating new instances of this class.
     *
     * Uses class AbstractConstraintAllocator to allocate new instances of this class.
     *
     * \param size Size in bytes of the requested memory block.
     *
     * \return New instance of this class.
     */
    void* operator new(size_t s);

    /**
     * \brief Implements the delete operator which is needed for deleting instances of this class.
     *
     * Uses class AbstractConstraintAllocator to delete instances of this class.
     *
     * \param p AbstractConstraint handle
     */
    void operator delete(void *c, size_t s);

    /**
     * \brief returns if the constraint is an agent constraint
     */
    virtual bool isAgentConstraint() const = 0;
};

} // agent
} // os

#endif
