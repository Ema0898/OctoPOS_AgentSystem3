#include <stdio.h>
#include <stdlib.h>

#include <octopos.h>
#include <octo_agent3.h>

/* Shared queue definitions  */
#define MSGS_PER_TILE_AND_CORE 10
#define MSG_MAX_PAYLOAD_SIZE ((uint32_t)108)
#define CORE_PER_ILET 1

#define QUEUE_MAX_MESSAGE_COUNT ((uint32_t)8)
#define QUEUE_MAX_HANDLER_ILETS ((uint32_t)4)
#define QUEUE_MAX_MESSAGE_SIZE ((uint32_t)sizeof(MessageData))

typedef struct
{
  uint32_t size;
  uint32_t tileID;
  uint32_t coreID;
  uint32_t msgID;
  char buffer[MSG_MAX_PAYLOAD_SIZE];
  uint32_t checksum;
} MessageData;

/* Agent system definitions */
#define PE_TYPE 0 // "RISC type PE"
#define DOWNEY_A 20000
#define DOWNEY_SIGMA 20
#define MIN_PE 2         // minimum required PE
#define MAX_PE 64        // maximum required PE
#define TILE_BIT_MASK 15 // 000...1111 - Invade on 4 tiles

/* Shared queue global variables */
uintptr_t messageReceived[cf_hw_sys_max_tiles][cf_hw_sys_max_cores_per_tile][MSGS_PER_TILE_AND_CORE];

static simple_signal messagesSignal[cf_hw_sys_max_tiles][cf_hw_sys_max_cores_per_tile];
static simple_signal handlersSignal;
static sharq_t queueHandle;

/* Functions */
static void die(const char message[]);
static uint32_t sender(sharq_t queue);
static void senderIlet(void *queuePtr);
static uint32_t receiver(sharq_t queue, int isHandler);
static void receiverIlet(void *queuePtr);

void main_ilet(claim_t claim)
{
  // Wraps the initial claim with an agent
  agentclaim_t initialClaim = agent_claim_get_initial(claim);

  // Prints original the claim
  agent_claim_print(initialClaim);

  // Gets initial constraints
  constraints_t newConstr = agent_claim_get_constr(initialClaim);
  // Sets the Downey parameters (A and sigma)
  agent_constr_set_downey_speedup_parameter(newConstr, DOWNEY_A, DOWNEY_SIGMA);
  // Modifies the number of PE in constraints
  agent_constr_set_quantity(newConstr, MIN_PE, MAX_PE, PE_TYPE);
  // Allows invasion on 4 tiles (bit map: 000...1111)
  agent_constr_set_tile_bitmap(newConstr, TILE_BIT_MASK);
  // Reinvades claim with updated constraints
  agent_claim_reinvade_constraints(initialClaim, newConstr);
  if (initialClaim == NULL)
  {
    printf("Reinvade failed... shutting off \n");
    shutdown(0);
  }

  uint32_t tileCount = get_compute_tile_count();
  if (tileCount <= 1)
  {
    fputs("SHARQ test needs more than one compute tile.\n", stderr);
    abort();
  }

  uint32_t sendingTileCount = tileCount - 1;
  uint32_t receivingTileCount = 1;

  if (sharq_is_native())
  {
    printf("SHARQ unit available.\n");
  }
  else
  {
    //fputs("SHARQ unit is not available.\n", stderr);
    //abort();
    printf("SHARQ emulated in software.\n");
  }

  uint32_t coreCount = CORE_PER_ILET;

  uint32_t totalMessages = MSGS_PER_TILE_AND_CORE * sendingTileCount * coreCount;
  for (uint32_t tid = 1; tid < sendingTileCount + 1; tid++)
  {
    for (uint32_t cid = 0; cid < coreCount; cid++)
    {
      simple_signal_init(&(messagesSignal[tid][cid]), MSGS_PER_TILE_AND_CORE);
    }
  }

  simple_signal_init(&handlersSignal, 0);

  // Create SHARQ queue
  queueHandle = sharq_create_queue(QUEUE_MAX_MESSAGE_COUNT, QUEUE_MAX_MESSAGE_SIZE);
  if (queueHandle == NULL)
  {
    die("sharq_create_queue");
  }

  simple_ilet iLet;
  simple_ilet_init(&iLet, receiverIlet, queueHandle);
  int res = sharq_register_handler_ilet(queueHandle, &iLet, QUEUE_MAX_HANDLER_ILETS);
  if (res != 0)
  {
    die("sharq_register_handler_ilet");
  }

  /* Infect tiles */
  for (int tile = 1; tile < get_tile_count(); tile++)
  {
    // Get available PEs
    int pes = agent_claim_get_pecount_tile_type(initialClaim, tile, PE_TYPE);
    printf("Tile %d, PE's %d \n", tile, pes);

    // If there are available PEs in this tile's claim.
    if (pes != 0)
    {
      // Get a proxy claim in tile for a "RISC" type PE
      proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(initialClaim, tile, PE_TYPE);
      if (pClaim == NULL)
      {
        printf("Couldn't get proxy claim \n");
      }
      // Create iLet team
      simple_ilet ILet;

      simple_ilet_init(&ILet, senderIlet, queueHandle);

      proxy_infect(pClaim, &ILet, 1);
    }
  }

  simple_signal_wait(&handlersSignal);

  printf("Waiting for queue signals\n");
  for (uint32_t tid = 1; tid < sendingTileCount + 1; tid++)
  {
    for (uint32_t cid = 0; cid < coreCount; cid++)
    {
      simple_signal_wait(&(messagesSignal[tid][cid]));
    }
  }
  printf("Queue signals passed\n");

  for (unsigned i = 1; i < tileCount; ++i)
  {
    for (unsigned int coreID = 0; coreID < coreCount; coreID++)
    {
      for (uint32_t msgID = 0; msgID < MSGS_PER_TILE_AND_CORE; msgID++)
      {
        if (messageReceived[i][coreID][msgID] != 1)
        {
          printf("missing message (tileID=%d coreID=%d msgID=%" PRIu32 ").\n", i, coreID, msgID);
          die("missing message");
        }
      }
    }
  }

  if (sharq_destroy_queue(queueHandle) != 0)
  {
    die("sharq_destroy_queue");
  }

  puts("Test successful");

  shutdown(0);
  return;
}

/* Error function */
static void die(const char message[])
{
  fprintf(stderr, "%s: failed\n", message);
  abort();
}

/* Sends messages to the queue */
static uint32_t sender(sharq_t queue)
{
  uint32_t numEnqueued = 0;

  MessageData buffer;
  for (size_t i = 0; i < MSG_MAX_PAYLOAD_SIZE; i++)
  {
    buffer.buffer[i] = i % 0xff;
  }

  buffer.size = MSG_MAX_PAYLOAD_SIZE;
  buffer.checksum = 42;
  buffer.tileID = get_tile_id();
  buffer.coreID = get_cpu_id();

  printf("SENDER CPU ID = %d on TILE = %d\n", buffer.coreID, buffer.tileID);

  while (numEnqueued < MSGS_PER_TILE_AND_CORE)
  {
    buffer.msgID = numEnqueued;
    //printf("Enqueuing message %d on tile %d and core %d...\n", buffer.msgID, buffer.tileID, buffer.coreID);
    int res = sharq_enqueue(queue, &buffer, sizeof(MessageData));
    if (res == 0)
    {
      // enqueued
      numEnqueued++;
      //DBGPRINTF("Enqueuing message %" PRIu32 " on tile %" PRIu32 " and core %" PRIu32 "... done.\n", buffer.msgID, buffer.tileID, buffer.coreID);
    }
    else if (res == 128)
    {
      // queue was full
      //TODO: wait some time ?
      cilk_create_continuation();
      cilk_yield();
      //DBGPRINTF("Enqueuing message %" PRIu32 " on tile %" PRIu32 " and core %" PRIu32 "... full.\n", buffer.msgID, buffer.tileID, buffer.coreID);
    }
    else
    {
      // other error
      //DBGPRINTF("Enqueuing message %" PRIu32 " on tile %" PRIu32 " and core %" PRIu32 "... failed with error %d\n", buffer.msgID, buffer.tileID, buffer.coreID, res);
      die("sharq_enqueue");
    }
  }

  return numEnqueued;
}

/* ilet for sender tiles */
static void senderIlet(void *queuePtr)
{
  sharq_t queue = (sharq_t)queuePtr;

  //DBGPRINTF("Sender started on tile %d and core %d...\n", get_tile_id(), get_cpu_id());
  //printf("Sender started on tile %d and core %d...\n", get_tile_id(), get_cpu_id());

  uint32_t sentMessages = sender(queue);

  //DBGPRINTF("Sender terminated on tile %d and core %d... (sent messages: %" PRIu32 ")\n", get_tile_id(), get_cpu_id(), sentMessages);
}

/* receives the messages */
static uint32_t receiver(sharq_t queue, int isHandler)
{
  uint32_t numDequeued = 0;
  unsigned tileCount = get_compute_tile_count();
  unsigned int coreCount = get_tile_core_count();

  MessageData buffer;

  //DBGPRINTF("Dequeuing message %" PRIu32 " on tile %d and core %d...\n", numDequeued, get_tile_id(), get_cpu_id());

  int res;
  if (isHandler)
  {
    res = sharq_dequeue_from_handler(queue, &buffer, sizeof(MessageData));
  }
  else
  {
    res = sharq_dequeue(queue, &buffer, sizeof(MessageData));
  }
  while (res != 128)
  {
    // queue was not empty

    //simple_spinlock_lock(&spinLock);
    if (res == 0)
    {
      // dequeued
      if (buffer.size == MSG_MAX_PAYLOAD_SIZE && buffer.checksum == 42 && buffer.tileID < tileCount && buffer.coreID < coreCount && buffer.msgID < MSGS_PER_TILE_AND_CORE && messageReceived[buffer.tileID][buffer.coreID][buffer.msgID] == 0)
      {
        //&& nextMessageIDExpected[buffer.tileID][buffer.coreID] == buffer.msgID){
        if (cas(&(messageReceived[buffer.tileID][buffer.coreID][buffer.msgID]), 0, 1) == 0)
        {
          //DBGPRINTF("Dequeuing message %" PRIu32 " on tile %d and core %d... invalid message (tileID=%" PRIu32 " coreID=%" PRIu32 " msgID=%" PRIu32 ").\n", numDequeued, get_tile_id(), get_cpu_id(), buffer.tileID, buffer.coreID, buffer.msgID);
          die("sharq_dequeue");
        }
        else
        {
          //DBGPRINTF("Dequeuing message %" PRIu32 " on tile %d and core %d... done (tileID=%" PRIu32 " coreID=%" PRIu32 " msgID=%" PRIu32 ").\n", numDequeued, get_tile_id(), get_cpu_id(), buffer.tileID, buffer.coreID, buffer.msgID);
          numDequeued++;
          //nextMessageIDExpected[buffer.tileID][buffer.coreID] = buffer.msgID + 1;
          simple_signal_signal(&(messagesSignal[buffer.tileID][buffer.coreID]));
        }
      }
      else
      {
        //DBGPRINTF("Dequeuing message %" PRIu32 " on tile %d and core %d... invalid message (tileID=%" PRIu32 " coreID=%" PRIu32 " msgID=%" PRIu32 ").\n", numDequeued, get_tile_id(), get_cpu_id(), buffer.tileID, buffer.coreID, buffer.msgID);
        printf("Bad response res = %d\n", res);
        die("sharq_dequeue");
      }
    }
    else
    {
      // other error
      //DBGPRINTF("Dequeuing message %" PRIu32 " on tile %d and core %d... failed with error %d\n", numDequeued, get_tile_id(), get_cpu_id(), res);
      die("sharq_dequeue");
    }
    //simple_spinlock_unlock(&spinLock);

    //printf("Dequeuing message %d on tile %d and core %d...\n", numDequeued, get_tile_id(), get_cpu_id());
    if (isHandler)
    {
      res = sharq_dequeue_from_handler(queue, &buffer, sizeof(MessageData));
    }
    else
    {
      res = sharq_dequeue(queue, &buffer, sizeof(MessageData));
    }
  }

  //DBGPRINTF("Dequeuing message %" PRIu32 " on tile %d and core %d... empty.\n", numDequeued, get_tile_id(), get_cpu_id());

  return numDequeued;
}

/* ilet for receiver tiles */
static void receiverIlet(void *queuePtr)
{
  sharq_t queue = (sharq_t)queuePtr;

  if (queue != queueHandle)
  {
    printf("queue parameter = 0x%" PRIxPTR " vs. global queueHandle = 0x%" PRIxPTR "\n", (uintptr_t)queue, (uintptr_t)queueHandle);
    die("invalid ilet param");
  }

  simple_signal_add_signalers(&handlersSignal, 1);

  receiver(queue, 1);

  simple_signal_signal(&handlersSignal);
}
