#ifndef FLATOPERATINGPOINT_H
#define FLATOPERATINGPOINT_H

#include "os/agent3/AgentMemory.h"
#include <stddef.h>

namespace os
{
    namespace agent
    {

        class OperatingPoint;

        class FlatOperatingPoint
        {

#define FLAT_OPERATING_POINT_SIZE (1 << 10)

        public:
            FlatOperatingPoint()
                : dataUsed(0)
            {
            }

            /*
     * Consumes the given ClusterGuarantee and stores it in its data container.
     * Returns true iff everything went well
     *         false otherwise, e.g., when the ClusterGuarantee hierarchy is to big for the container.
     */
            bool flatten(OperatingPoint *source, uint8_t number_of_ops);

            /*
     * Unflattens the ClusterGuarantee it holds.
     * Creates new objects for each ClusterGuarantee, i.e., it reserves space for the
     * ClusterGuarantees on the heap and copies the content into these objects. While this takes
     * longer, the constraint hierarchy may be used after this object has been deleted.
     */
            void unflatten(OperatingPoint **target, uint8_t number_of_ops) const;

            void *operator new(size_t s)
            {
                return AgentMemory::agent_mem_allocate(s);
            }

            void operator delete(void *c, size_t s)
            {
                AgentMemory::agent_mem_free(c);
            }

        private:
            /*
     * Recursively builds a ClusterGuarantee hierarchy out of flat data.
     * The new ClusterGuarantee hierarchy will consist of newly created (i.e., dynamically allocated) objects
     *
     * Returns the size of the consumed data.
     *
     * @param target    Will hold a pointer to the "newly created" object.
     * @param offset    The offset (relative to the object's member 'data') of the ClusterGuarantee to create
     */
            size_t unflattenMember(OperatingPoint *operating_point, size_t offset) const;

            /*
     * This chunk of memory has the following layout:
     * Each ClusterGuarantee is preceeded by its type. Listmembers are appended to the list-object
     * It is basically an array of structs of different sizes, with the first member being of type ConstraintType
     * and the second member being the respective Constraint object
     */
            char data[FLAT_OPERATING_POINT_SIZE];

            size_t dataUsed;
        };
    } // namespace agent
} // namespace os
#endif // FLATOPERATINGPOINT_H
