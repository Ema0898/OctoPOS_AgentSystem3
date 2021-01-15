#ifndef METRIC_NEW_AGENT_RENAME_H
#define METRIC_NEW_AGENT_RENAME_H

#include "os/agent3/Metric.h"
#include "os/agent3/Agent.h"
#include "lib/debug.h"

namespace os
{
	namespace agent
	{
		class MetricAgentRename : public Metric
		{
		public:
			MetricAgentRename();
			MetricAgentRename(uint8_t id, const char *name);
			char *package();

			uint8_t agentId;
			char newName[os::agent::AgentInstance::maxNameLength];

		private:
			void init();

			const int metricId = 24;
			int timestamp;
		};
	} // namespace agent
} // namespace os

#endif