#include "os/agent3/MetricSender.h"
#include "os/agent3/MetricSystemArchitecture.h"

#include <stdio.h>
//#include <octo_eth.h>
#include <octo_dispatch_claim.h>
#include <octo_clock.h>
#include "os/agent3/AgentMemory.h"
#include "lib/debug.h"

// Initial values for static class variables
// int os::agent::MetricSender::writeChannelNo = 1;
// int os::agent::MetricSender::writeChannelTransportMode = ETH_TRANS_CHUNKS;
// eth_channel_t os::agent::MetricSender::writeChannel;
// bool os::agent::MetricSender::isEnabled = true;
// bool os::agent::MetricSender::isEthernetInitialized = false;
// bool os::agent::MetricSender::isAsyncSendEnabled = false;
// lib::adt::SimpleSpinlock os::agent::MetricSender::lockEthernetInit;

// bool os::agent::MetricSender::startEthernet()
// {
//     DBG(SUB_AGENT_TELEMETRY, "'startEthernet' now executing...\n");

//     writeChannel = eth_open(writeChannelNo, ETH_MODE_WRITE);
//     if (writeChannel != 0)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(startEthernet) OK: Write Channel opened.\n");
//         eth_set_transport(writeChannel, writeChannelTransportMode);
//         return 0;
//     }
//     else
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(startEthernet) ERR: Error during Channel Creation.\n");
//         return -1;
//     }
// }

// bool os::agent::MetricSender::stopEthernet()
// {
//     DBG(SUB_AGENT_TELEMETRY, "'stopEthernet' now executing...\n");

//     int ethCloseReturnStatus = eth_close(writeChannel);
//     if (ethCloseReturnStatus < 0)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(stopEthernet) ERR: Closing the Ethernet Connection failed (eth_close returned value < 0)\n");
//         return -1;
//     }
//     else if (ethCloseReturnStatus > 0)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(stopEthernet) ERR: Ethernet Close Method returned unknown status, likely failed (eth_close returned > 0)\n");
//         return -1;
//     }

//     //ethCloseReturnStatus == 0
//     DBG(SUB_AGENT_TELEMETRY, "(stopEthernet) OK: Closed Ethernet Connection.\n");
//     return 0;
// }

void os::agent::MetricSender::measureMetric(Metric &metric)
{
    // 1. Package metrics data
    char *package = metric.package();
    printf("Metric Message: %s\n", package);

    // 2. Send to remote host asynchronously
    //sendToHostAsync(package);
}

// void os::agent::MetricSender::sendToHostILet(void *arg)
// {
//     // Debug Information Printout #1
//     DBG(SUB_AGENT_TELEMETRY, "(sendToHostILet) now executing on Tile %i\n", hw::hal::Tile::getTileID());

//     // Check for NULL Pointer
//     if (!arg)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(sendToHostILet) ERR:  NULL Pointer passed to method 'sendToHostILet'\n");
//         return;
//     }

//     // For Convienence - We could also use the method argument passed to us directly
//     char *buffer = (char *)arg;

//     // Ethernet SEND Process
//     int ethSendReturnStatus = eth_send(writeChannel, buffer, packageSize, NULL);
//     if (ethSendReturnStatus == 0)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(sendToHostILet) OK: Telemetry Data sent.\n");
//     }
//     else if (ethSendReturnStatus < 0)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(sendToHostILet) ERR: Could not send over Ethernet (eth_send returned value < 0)\n");
//         return;
//     }
//     else if (ethSendReturnStatus > 0)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(sendToHostILet) ERR: Ethernet Send Method returned unknown status, likely failed (eth_send returned > 0)\n");
//         return;
//     }

//     // Free Ressources
//     os::agent::AgentMemory::agent_mem_free(buffer);
// }

// bool os::agent::MetricSender::sendToHostAsync(char *payload)
// {
//     DBG(SUB_AGENT_TELEMETRY, "(sendToHostAsync) 'sendToHostAsync' now executing... \n");

//     // Validate parameter
//     if (!payload)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(sendToHostAsync) ERR: NULL Pointer passed to'sendToHost'\n");
//         return false;
//     }

//     // Verify the payload is located in Tile-Local-Memory (TLM)
//     int payloadMemType = os::agent::AgentMemory::agent_mem_get_type(payload);
//     if (payloadMemType == os::agent::AgentMemory::MEM_SHM)
//     {
//         DBG(SUB_AGENT_TELEMETRY, "(sendToHostAsync) ERR: Source data for Ethernet SEND is located in Shared (global) memory. It has to be located in Tile-Local-Memory (TLM) instead.\n");
//         return false;
//     }

//     // Start Ethernet Connection if not already the case
//     lockEthernetInit.lock();
//     if (!isEthernetInitialized)
//     {
//         printf("InvadeVIEW Visualization is activated. Please run InvadeVIEW agent system tools to continue. \n");
//         if (startEthernet() == 0)
//         {
//             isEthernetInitialized = true;
//             // Start of Ethernet is currently also used as hook
//             // to define a new Session (~ Lifetime of the current
//             // application). It would be better to sendout this
//             // during a "boot complete" point in time.
//             MetricSystemArchitecture m;
//             sendToHostILet(m.package());
//         }
//         else
//         {
//             DBG(SUB_AGENT_TELEMETRY, "(sendToHostAsync) ERR: Establishing Ethernet Channel was not sucessful.\n");
//         }
//     }
//     lockEthernetInit.unlock();

//     /* Now, send this over Ethernet in a seperated iLet
//      *
//      * Note: As long as the Claim of the Agent System does not include more
//      *       more than one Processor, this has not the desired effect,
//      *       as there is no preemtive multitasking in OctoPOS.
//      *       Besides this, the Ethernet functionality apparently also uses
//      *       iLets internally, so this "threading" here might be actually not
//      *       necessary in a tighter sense.
//     */
//     if (isEnabled && isAsyncSendEnabled)
//     {
//         simple_ilet myILet;
//         simple_ilet_init(&myILet, os::agent::MetricSender::sendToHostILet, payload);
//         infect_self_single(&myILet); // Start new iLet (new "Thread")
//     }
//     else if (isEnabled)
//     {
//         sendToHostILet(payload);
//     }
//     return true;
// }
