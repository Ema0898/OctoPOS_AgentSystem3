#include "AbstractAgentOctoClaim.h"

// see AbstractAgentOctoClaim.h for code documentation

void* os::agent::AbstractAgentOctoClaim::operator new(size_t s) throw() {
	return os::agent::AgentMemory::agent_mem_allocate(s);
}

void os::agent::AbstractAgentOctoClaim::operator delete(void *p) {
	os::agent::AgentMemory::agent_mem_free(p);
}

