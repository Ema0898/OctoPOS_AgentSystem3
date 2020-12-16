#include <octopos.h>
#include <stdio.h>
#include <stdlib.h>

static void invadeCores(void *arg);
static void doSignal(void *arg);
static void noiseFunc(void *arg);

extern "C" void main_ilet(claim_t claim)
{
  printf("Starting example program\n");

  uint8_t NUM_TILES = get_tile_count();

  invade_future *futures = static_cast<invade_future *>(malloc((NUM_TILES - 1) * sizeof(invade_future)));
  proxy_claim_t *claims = static_cast<proxy_claim_t *>(malloc((NUM_TILES - 1) * sizeof(proxy_claim_t)));
  simple_signal *signals = static_cast<simple_signal *>(malloc((NUM_TILES - 1) * sizeof(simple_signal)));
  simple_ilet *iLets = static_cast<simple_ilet *>(malloc((NUM_TILES - 1) * sizeof(simple_ilet)));

  for (int i = 1; i < NUM_TILES; ++i)
  {
    invade_future future = futures[i - 1];

    if (proxy_invade(i, &future, 1) != 0)
    {
      printf("Can't invade tile %d\n", i);
      abort();
    }

    claims[i - 1] = invade_future_force(&future);

    simple_signal_init(&signals[i - 1], 1);
    simple_ilet_init(&iLets[i - 1], invadeCores, &signals[i - 1]);

    if (claims[i - 1] != 0)
    {
      proxy_infect(claims[i - 1], &iLets[i - 1], 1);
    }
  }

  for (int i = 1; i < NUM_TILES; ++i)
  {
    simple_signal_wait(&signals[i - 1]);
  }

  shutdown(0);
}

static void invadeCores(void *arg)
{
  printf("Hello from tile %d with %d cores\n", get_tile_id(), get_tile_core_count());

  claim_t claim = claim_construct();
  int ret = invade_simple(claim, 1);
  if (ret != 0)
  {
    printf("cannot invade core\n");
    abort();
  }

  simple_signal signal;
  simple_signal_init(&signal, 1);

  simple_ilet ilet;
  simple_ilet_init(&ilet, noiseFunc, &signal);

  ret = infect(claim, &ilet, 1);
  if (ret != 0)
  {
    printf("cannot infect core\n");
    abort();
  }

  simple_signal_wait(&signal);

  simple_ilet reply;
  simple_ilet_init(&reply, doSignal, arg);
  dispatch_claim_send_reply(&reply);
}

static void doSignal(void *arg)
{
  simple_signal_signal(static_cast<simple_signal *>(arg));
}

static void noiseFunc(void *arg)
{
  printf("Hello from tile %d from core 1\n", get_tile_id());

  simple_ilet reply;
  simple_ilet_init(&reply, doSignal, arg);
  dispatch_claim_send_reply(&reply);
}