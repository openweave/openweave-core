#include "test_mem.h"

#include "lwip/mem.h"
#include "lwip/stats.h"

#if !LWIP_STATS || !MEM_STATS
#error "This tests needs MEM-statistics enabled"
#endif

/* Setups/teardown functions */

static void
mem_setup(void)
{
}

static void
mem_teardown(void)
{
}


/* Test functions */

/** Call mem_malloc, mem_free and mem_trim and check stats */
START_TEST(test_mem_one)
{
#define SIZE1   16
#define SIZE1_2 12
#define SIZE2   16
  void *p1, *p2;
  mem_size_t s1, s2;
  mem_size_t used;
  LWIP_UNUSED_ARG(_i);

  used = lwip_stats.mem.used;
  fail_unless(lwip_stats.mem.used - used == 0);

  p1 = mem_malloc(SIZE1);
  fail_unless(p1 != NULL);
  fail_unless(lwip_stats.mem.used - used >= SIZE1);
  s1 = lwip_stats.mem.used - used;

  p2 = mem_malloc(SIZE2);
  fail_unless(p2 != NULL);
  fail_unless(lwip_stats.mem.used - used >= SIZE2 + s1);
  s2 = lwip_stats.mem.used - used;

  mem_trim(p1, SIZE1_2);

  mem_free(p2);
  fail_unless(lwip_stats.mem.used - used <= s2 - SIZE2);

  mem_free(p1);
  fail_unless(lwip_stats.mem.used - used == 0);
}
END_TEST

/** malloc_keep_x - Four steps are implemented by this function
    1) Allocates num blocks of size bytes.
    2) Free's every freestep block and prevents block x from being freed.
    3) Free's every remaining block except block x.
    4) Free's block x.
  Used by test_mem_random to exercise the heap by freeing blocks in
  pseudo random order.*/
static void malloc_keep_x(int x, int num, int size, int freestep)
{
   int i;
   void* p[16];
   LWIP_ASSERT("invalid size", size >= 0 && size < (mem_size_t)-1);
   memset(p, 0, sizeof(p));
   for(i = 0; i < num && i < 16; i++) {
      p[i] = mem_malloc((mem_size_t)size);
      fail_unless(p[i] != NULL);
   }
   for(i = 0; i < num && i < 16; i += freestep) {
      if (i == x) {
         continue;
      }
      mem_free(p[i]);
      p[i] = NULL;
   }
   for(i = 0; i < num && i < 16; i++) {
      if (i == x) {
         continue;
      }
      if (p[i] != NULL) {
         mem_free(p[i]);
         p[i] = NULL;
      }
   }
   fail_unless(p[x] != NULL);
   mem_free(p[x]);
}

START_TEST(test_mem_random)
{
  const int num = 16;
  int x;
  int size;
  int freestep;
  mem_size_t used;
  LWIP_UNUSED_ARG(_i);

  used = lwip_stats.mem.used;

  for (x = 0; x < num; x++) {
     for (size = 1; size < 32; size++) {
        for (freestep = 1; freestep <= 3; freestep++) {
          fail_unless(lwip_stats.mem.used - used == 0);
          malloc_keep_x(x, num, size, freestep);
          fail_unless(lwip_stats.mem.used - used == 0);
        }
     }
  }
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
mem_suite(void)
{
  TFun tests[] = {
    test_mem_random,
    test_mem_one
  };
  return create_suite("MEM", tests, sizeof(tests)/sizeof(TFun), mem_setup, mem_teardown);
}
