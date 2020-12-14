#include <octopos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct DMA_info
{
  void *src_buffer;
  void *dst_buffer;
  uintptr_t buffer_size;
  proxy_claim_t dst_claim;
};

static void print_buffer(void *addr)
{
  printf("PRINT FROM TILE = %d\n", get_tile_id());
  //print buffer addr and data
  char *buffer = (char *)addr;
  printf("Buffer addr: 0x%p\ndata: %s\n", buffer, buffer);
  free(buffer);
}

static void start_push_DMA(void *_dst_buff_addr, void *_dma_info)
{
  printf("START PUSH FROM TILE = %d\n", get_tile_id());
  struct DMA_info *dma_info = (struct DMA_info *)_dma_info;
  dma_info->dst_buffer = _dst_buff_addr;

  // init ilets
  simple_ilet dstilet;
  simple_ilet_init(&dstilet, print_buffer,
                   get_local_address(dma_info->dst_buffer));

  // dma
  int ret = proxy_push_dma(dma_info->dst_claim, dma_info->src_buffer,
                           dma_info->dst_buffer, dma_info->buffer_size, 0x00, &dstilet);

  if (ret)
  {
    printf("Error: DMA failed\n");
    abort();
  }

  free(dma_info);
}

static void init_push_DMA(void *_size, void *_dma_info)
{

  uintptr_t size = (uintptr_t)_size;

  printf("INIT PUSH FROM TILE = %d\n", get_tile_id());

  // init destination buffer
  char *buffer = (char *)mem_allocate(MEM_TLM_LOCAL, sizeof(char) * size);
  if (!buffer)
  {
    printf("Error: unable to alloc destination buffer\n");
    abort();
  }
  memset(buffer, 0x0, sizeof(char) * size);

  // get global addr
  void *buff_global_addr = get_global_address_for_tile(buffer, 1);

  // sendDMA ilet to parent claim
  simple_ilet ilet;
  dual_ilet_init(&ilet, start_push_DMA, buff_global_addr, _dma_info);

  dispatch_claim_send_reply(&ilet);
}

void main_ilet(claim_t claim)
{
  printf("Hello World\n");

  uintptr_t size = 256;
  char *buffer = (char *)mem_allocate(MEM_TLM_LOCAL, sizeof(char) * size);

  snprintf(buffer, size, "HOLA MUNDO");

  invade_future_t future;
  int ret = proxy_invade(1, &future, 1);
  if (ret)
  {
    printf("Error: proxy_invade failed\n");
    abort();
  }
  proxy_claim_t cl = invade_future_force(&future);
  if (!cl)
  {
    printf("Error: invade failed\n");
    abort();
  }

  struct DMA_info *dma_info = (struct DMA_info *)mem_allocate(MEM_TLM_LOCAL,
                                                              sizeof(struct DMA_info));
  dma_info->src_buffer = buffer;
  dma_info->dst_claim = cl;
  dma_info->buffer_size = size;

  // create ilet and infect
  simple_ilet ilet;
  dual_ilet_init(&ilet, init_push_DMA, (void *)dma_info->buffer_size, dma_info);
  proxy_infect(cl, &ilet, 1);

  return;
}