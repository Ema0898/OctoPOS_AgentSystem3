#ifndef OS_AGENT_MEMORY_H
#define OS_AGENT_MEMORY_H

#include <stddef.h>
#include <octo_sys_memory.h>
#include "lib/kmalloc.h"

namespace os {
namespace agent {

class AgentMemory {

public:
	enum {
		MEM_TLM_LOCAL = 0,
		MEM_TLM_GLOBAL = 1,
		MEM_SHM = 2,
		MEM_ICM = 3,
		MEM_TYPES_SIZE = 4,
		MEM_INVALID = -1
	};

	static void *agent_mem_allocate(size_t s) {
		return kmalloc_raw(s);
	}

	static void agent_mem_free(void *p) {
		return kfree(p);
	}

	static int agent_mem_get_type(const void *p) {
		return mem_get_type(p);
	}
};
}
}

#endif // OS_AGENT_MEMORY_H
