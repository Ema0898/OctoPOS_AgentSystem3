#include <stdio.h>
#include <stdlib.h>

#include <octopos.h>
#include <octo_agent3.h>

/* Shared queue definitions  */
#define MSGS_PER_TILE_AND_CORE 10
#define MSG_MAX_PAYLOAD_SIZE ((uint32_t)108)
#define CORE_PER_ILET 4

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
static simple_signal handlersSignal; // Handler signal
static sharq_t queueHandle;          // Queue handle

/* Functions */
static void die(const char message[]);
static uint32_t sender(sharq_t queue);
static void senderIlet(void *queuePtr);
static uint32_t receiver(sharq_t queue, int isHandler);
static void receiverIlet(void *queuePtr);

void main_ilet(claim_t claim)
{
  /* Wraps the initial claim with an agent */
  agentclaim_t initialClaim = agent_claim_get_initial(claim);

  /* Prints original the claim */
  agent_claim_print(initialClaim);

  /* Gets initial constraints */
  constraints_t newConstr = agent_claim_get_constr(initialClaim);
  /* Sets the Downey parameters (A and sigma) */
  agent_constr_set_downey_speedup_parameter(newConstr, DOWNEY_A, DOWNEY_SIGMA);
  /* Modifies the number of PE in constraints */
  agent_constr_set_quantity(newConstr, MIN_PE, MAX_PE, PE_TYPE);
  /* Allows invasion on 4 tiles (bit map: 000...1111) */
  agent_constr_set_tile_bitmap(newConstr, TILE_BIT_MASK);
  /* Reinvades claim with updated constraints */
  agent_claim_reinvade_constraints(initialClaim, newConstr);
  if (initialClaim == NULL)
  {
    printf("Reinvade failed... shutting off \n");
    shutdown(0);
  }

  /* Gets tile count */
  uint32_t tileCount = get_compute_tile_count();
  if (tileCount <= 1)
  {
    fputs("SHARQ test needs more than one compute tile.\n", stderr);
    abort();
  }

  /* Checks if hardware queue is present */
  if (sharq_is_native())
  {
    printf("SHARQ unit available.\n");
  }
  else
  {
    printf("SHARQ emulated in software.\n");
  }

  simple_signal_init(&handlersSignal, 0);

  /* Creates SHARQ queue */
  queueHandle = sharq_create_queue(QUEUE_MAX_MESSAGE_COUNT, QUEUE_MAX_MESSAGE_SIZE);
  if (queueHandle == NULL)
  {
    die("sharq_create_queue");
  }

  /* Initializes receiver queues */
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
    /* Get available PEs */
    int pes = agent_claim_get_pecount_tile_type(initialClaim, tile, PE_TYPE);
    printf("Tile %d, PE's %d \n", tile, pes);

    /* If there are available PEs in this tile's claim. */
    if (pes != 0)
    {
      /* Get a proxy claim in tile for a "RISC" type PE */
      proxy_claim_t pClaim = agent_claim_get_proxyclaim_tile_type(initialClaim, tile, PE_TYPE);
      if (pClaim == NULL)
      {
        printf("Couldn't get proxy claim \n");
      }

      simple_ilet ILet[pes];

      for (int iletnr = 0; iletnr < pes; ++iletnr)
      {
        simple_ilet_init(&ILet[iletnr], senderIlet, queueHandle);
      }
      /* Infect claim with Ilet team. */
      printf("Infecting %d Ilets on Tile %d\n", pes, tile);
      proxy_infect(pClaim, ILet, pes);
    }
  }

  /* Wait for handler */
  simple_signal_wait(&handlersSignal);

  /*destroys queue */
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

  /* sends messages */
  while (numEnqueued < MSGS_PER_TILE_AND_CORE)
  {
    buffer.msgID = numEnqueued;

    int res = sharq_enqueue(queue, &buffer, sizeof(MessageData));
    if (res == 0)
    {
      numEnqueued++;
    }
    else if (res == 128)
    {
      /* queue was full */
      cilk_create_continuation();
      cilk_yield();
    }
    else
    {
      /* other error */
      die("sharq_enqueue");
    }
  }

  return numEnqueued;
}

/* ilet for sender tiles */
static void senderIlet(void *queuePtr)
{
  /* gets ilet queue */
  sharq_t queue = (sharq_t)queuePtr;

  /* calls sender function */
  uint32_t sentMessages = sender(queue);
}

/* receives the messages */
static uint32_t receiver(sharq_t queue, int isHandler)
{
  uint32_t numDequeued = 0;
  unsigned tileCount = get_compute_tile_count();
  unsigned int coreCount = get_tile_core_count();

  MessageData buffer;

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
    /* queue was not empty */
    if (res == 0)
    {
      /* dequeued */
      if (buffer.size == MSG_MAX_PAYLOAD_SIZE && buffer.checksum == 42 && buffer.tileID < tileCount && buffer.coreID < coreCount)
      {
        numDequeued++;
      }
      else
      {
        printf("INVALID MESSAGE\n");
        die("sharq_dequeue");
      }
    }
    else
    {
      /* other error */
      die("sharq_dequeue");
    }
    if (isHandler)
    {
      res = sharq_dequeue_from_handler(queue, &buffer, sizeof(MessageData));
    }
    else
    {
      res = sharq_dequeue(queue, &buffer, sizeof(MessageData));
    }
  }

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

  /* Signal UP */
  simple_signal_add_signalers(&handlersSignal, 1);

  /* Calls receiver function */
  uint32_t messages = receiver(queue, 1);
  printf("%d messages received from tile %d\n", messages, get_tile_id());

  /* Signal DOWN */
  simple_signal_signal(&handlersSignal);
}
