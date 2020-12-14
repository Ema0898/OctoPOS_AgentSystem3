#ifndef METRIC_SENDER_H
#define METRIC_SENDER_H

#include "lib/adt/SimpleSpinlock.h"
#include <octo_eth.h>
#include "os/agent3/Metric.h"

namespace os
{
    namespace agent
    {

        class MetricSender
        {

        public:
            // Method Definitions
            // TODO: Decide about visibility
            static bool startEthernet();
            static bool stopEthernet();
            static void measureMetric(Metric &metric);

            static bool isEnabled;
            static bool isAsyncSendEnabled;

            static const int packageSize = 512;

        private:
            static bool sendToHostAsync(char *packagedPayload);
            static void sendToHostILet(void *arg);

            static int writeChannelNo;
            static int writeChannelTransportMode;
            static eth_channel_t writeChannel;

            static bool isEthernetInitialized;
            static lib::adt::SimpleSpinlock lockEthernetInit;
            //static lib::adt::SimpleSpinlock lockEthernetSend;
        };

    } // namespace agent
} // namespace os

#endif // METRIC_SENDER_H Include-Guard
