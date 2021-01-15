#include "os/agent3/AbstractConstraint.h"
#include "os/agent3/AgentConstraint.h"
#include "os/agent3/ActorConstraint.h"

os::agent::AbstractConstraint::~AbstractConstraint()
{
}

void *os::agent::AbstractConstraint::operator new(size_t s)
{
    return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::AbstractConstraint::operator delete(void *c, size_t s)
{
    os::agent::AgentMemory::agent_mem_free(c);
}
