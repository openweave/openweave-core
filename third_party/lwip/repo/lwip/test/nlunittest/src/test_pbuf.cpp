#include <string.h>

#include "nltest.h"
#include <test_pbuf.h>
#include <lwip/init.h>
#include <lwip/pbuf.h>
#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwipopts.h>
#include <nlplatform/nlwatchdog.h>
#include <nlplatform.h>

// These tests assume that LWIP has been initialized via lwip_init() call

#if LWIP_PBUF_FROM_CUSTOM_POOLS
extern int num_used_pool[];
#else
extern int num_used_pool;
#endif

extern "C" {
    memp_t pbuf_get_target_pool(u16_t length, u16_t offset);
}

static int *get_num_used_pool_ptr(memp_t type)
{
#if LWIP_PBUF_FROM_CUSTOM_POOLS
    return &num_used_pool[PBUF_CUSTOM_POOL_IDX_START - type];
#else
    return &num_used_pool;
#endif
}

static void assert_pbuf_pools_empty(nlTestSuite *inSuite)
{
    memp_t target_pool;

#if LWIP_PBUF_FROM_CUSTOM_POOLS
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_LARGE, 0);
    NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == 0);

    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_MEDIUM, 0);
    NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == 0);

    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_SMALL, 0);
    NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == 0);
#else
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE, 0);
    NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == 0);
#endif
}


#if LWIP_PBUF_FROM_CUSTOM_POOLS
static void exhaust_pbuf_pool(nlTestSuite *inSuite, struct pbuf **pbuf, unsigned pool_size, int pbuf_size)
{
    uint8_t count;
    nlwatchdog_refresh();
    memp_t target_pool;

    target_pool = pbuf_get_target_pool(pbuf_size, 0);

    // Allocate all pbufs from the pbuf pool, checking
    // to make sure we are allocating from the right pool
    for (unsigned i = 0; i < pool_size; i++)
    {
        pbuf[i] = pbuf_alloc(PBUF_RAW, pbuf_size, PBUF_POOL);
        NL_TEST_ASSERT(inSuite, pbuf[i] != NULL);
        if (pbuf[i] == NULL)
        {
            // Clean up before returning
            for (unsigned j = 0; j < i; j++)
            {
                pbuf_free(pbuf[j]);
            }
            goto done;
        }
        NL_TEST_ASSERT(inSuite, pbuf[i]->next == NULL);
        NL_TEST_ASSERT(inSuite, pbuf[i]->len == pbuf_size);
        NL_TEST_ASSERT(inSuite, pbuf[i]->tot_len == pbuf_size);
        // Make sure we actually allocated from the proper pool
        NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == (int)i+1);
    }

    for (unsigned i = 0; i < pool_size; i++)
    {
        count = pbuf_free(pbuf[i]);
        NL_TEST_ASSERT(inSuite, count == 1);
    }

done:
    // Make sure pbuf pool is empty again
    assert_pbuf_pools_empty(inSuite);
}


// Verify we can allocate all large pbufs
static void TestPbufAllocLarge(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf[PBUF_POOL_SIZE_LARGE];

    exhaust_pbuf_pool(inSuite, pbuf, ARRAY_SIZE(pbuf), PBUF_POOL_BUFSIZE_LARGE);
}

// Verify we can allocate all medium pbufs
static void TestPbufAllocMedium(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf[PBUF_POOL_SIZE_MEDIUM];

    exhaust_pbuf_pool(inSuite, pbuf, ARRAY_SIZE(pbuf), PBUF_POOL_BUFSIZE_MEDIUM);
}

// Verify we can allocate all small pbufs
static void TestPbufAllocSmall(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf[PBUF_POOL_SIZE_SMALL];

    exhaust_pbuf_pool(inSuite, pbuf, ARRAY_SIZE(pbuf), PBUF_POOL_BUFSIZE_SMALL);
}

#define PBUF_CHAIN_TAIL_LEN 24

// Allocate an amount slightly larger than the largest pbuf. This will
// chain a small pbuf to a large pbuf.  Verify this is successful, and
// verify everything is freed properly.
static void TestPbufAllocChain(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf = NULL;
    uint8_t count;
    memp_t target_pool;

    nlwatchdog_refresh();
    // Allocate something that is guaranteed to spill over into
    // a chain of pbufs, one large then one small
    pbuf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE_LARGE + PBUF_CHAIN_TAIL_LEN, PBUF_POOL);
    NL_TEST_ASSERT(inSuite, pbuf != NULL);
    if (pbuf == NULL)
    {
        goto done;
    }
    NL_TEST_ASSERT(inSuite, pbuf->tot_len == PBUF_POOL_BUFSIZE_LARGE + PBUF_CHAIN_TAIL_LEN);
    NL_TEST_ASSERT(inSuite, pbuf->len == PBUF_POOL_BUFSIZE_LARGE);
    NL_TEST_ASSERT(inSuite, pbuf->next != NULL);
    if (pbuf->next == NULL)
    {
        pbuf_free(pbuf);
        goto done;
    }
    NL_TEST_ASSERT(inSuite, pbuf->next->tot_len == PBUF_CHAIN_TAIL_LEN);
    NL_TEST_ASSERT(inSuite, pbuf->next->len == PBUF_CHAIN_TAIL_LEN);

    // Make sure we have allocated one large and one small pbuf
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_LARGE, 0);
    NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == 1);
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_SMALL, 0);
    NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == 1);

    // Make sure we free exactly two pbufs
    count = pbuf_free(pbuf);
    NL_TEST_ASSERT(inSuite, count == 2);

done:
    // Make sure pbuf pools are empty again
    assert_pbuf_pools_empty(inSuite);
}

// Allocate all but one large pbuf, and attempt to allocate an amount that
// would require two large pbufs chained together.  Verify that the large
// allocation returns NULL and that everything is freed successfully
static void TestPbufAllocChainFull(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf = NULL;
    struct pbuf *pbuf_large[PBUF_POOL_SIZE_LARGE - 1];
    uint8_t count;
    memp_t target_pool;

    nlwatchdog_refresh();
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_LARGE, 0);
    for (unsigned j = 0; j < PBUF_POOL_SIZE_LARGE - 1; j++)
    {
        pbuf_large[j] = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE_LARGE, PBUF_POOL);
        NL_TEST_ASSERT(inSuite, pbuf_large[j] != NULL);
        // Make sure we have allocated from the large pool
        NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == (int)j+1);
    }
    // Allocate something that is guaranteed to spill over into
    // a chain of large pbufs.  There is only one pbuf left so the
    // chain operation should fail and gracefully recover
    pbuf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE_LARGE * 2, PBUF_POOL);
    NL_TEST_ASSERT(inSuite, pbuf == NULL);

    // Make sure we successfully free all PBUFs
    for (unsigned j = 0; j < PBUF_POOL_SIZE_LARGE - 1; j++)
    {
        count = pbuf_free(pbuf_large[j]);
        NL_TEST_ASSERT(inSuite, count == 1);
    }

    // Make sure pbuf pools are empty again
    assert_pbuf_pools_empty(inSuite);
}

// Exhaust a medium sized pbuf pool, and continue to allocate medium sized pbufs
// until the next largest pool is exhausted.  Verify that attempting to allocate
// a final pbuf returns NULL, and verify that we are able to correctly free all
// pbufs to their proper pools
static void TestPbufAllocOverflow(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *medium_pbufs[PBUF_POOL_SIZE_MEDIUM];
    struct pbuf *large_pbufs[PBUF_POOL_SIZE_LARGE];
    struct pbuf *overflow_pbuf = NULL;
    uint8_t count;
    memp_t target_pool;

    nlwatchdog_refresh();
    // Allocate all the medium pbufs
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_MEDIUM, 0);
    for (unsigned i = 0; i < PBUF_POOL_SIZE_MEDIUM; i++)
    {
        medium_pbufs[i] = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE_MEDIUM, PBUF_POOL);
        NL_TEST_ASSERT(inSuite, medium_pbufs[i] != NULL);
        if (medium_pbufs[i] == NULL)
        {
            // Clean up before returning
            for (unsigned j = 0; j < i-1; j++)
            {
                pbuf_free(medium_pbufs[j]);
            }
            goto done;
        }
        NL_TEST_ASSERT(inSuite, medium_pbufs[i]->next == NULL);
        NL_TEST_ASSERT(inSuite, medium_pbufs[i]->tot_len == PBUF_POOL_BUFSIZE_MEDIUM);
        NL_TEST_ASSERT(inSuite, medium_pbufs[i]->len == PBUF_POOL_BUFSIZE_MEDIUM);
        // Make sure we have allocated from the medium pool
        NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == (int)i+1);
    }

    // Allocate medium pbufs to fill the large pool
    target_pool = pbuf_get_target_pool(PBUF_POOL_BUFSIZE_LARGE, 0);
    for (unsigned i = 0; i < PBUF_POOL_SIZE_LARGE; i++)
    {
        large_pbufs[i] = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE_MEDIUM, PBUF_POOL);
        NL_TEST_ASSERT(inSuite, large_pbufs[i] != NULL);
        if (large_pbufs[i] == NULL)
        {
            // Clean up before returning
            for (unsigned j = 0; j < i-1; j++)
            {
                pbuf_free(large_pbufs[j]);
            }
            goto done;
        }
        NL_TEST_ASSERT(inSuite, large_pbufs[i]->next == NULL);
        NL_TEST_ASSERT(inSuite, large_pbufs[i]->tot_len == PBUF_POOL_BUFSIZE_MEDIUM);
        NL_TEST_ASSERT(inSuite, large_pbufs[i]->len == PBUF_POOL_BUFSIZE_MEDIUM);
        NL_TEST_ASSERT(inSuite, *get_num_used_pool_ptr(target_pool) == (int)i+1);
    }

    // Make sure trying to allocate one more pbuf is unsuccessful
    overflow_pbuf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE_MEDIUM, PBUF_POOL);
    NL_TEST_ASSERT(inSuite, overflow_pbuf == NULL);

    // Free all our pbufs
    for (unsigned i = 0; i < PBUF_POOL_SIZE_LARGE; i++)
    {
        count = pbuf_free(large_pbufs[i]);
        NL_TEST_ASSERT(inSuite, count == 1);
    }

    for (unsigned i = 0; i < PBUF_POOL_SIZE_MEDIUM; i++)
    {
        count = pbuf_free(medium_pbufs[i]);
        NL_TEST_ASSERT(inSuite, count == 1);
    }

done:
    // Make sure pbuf pools are empty again
    assert_pbuf_pools_empty(inSuite);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("Allocate Large Pool", TestPbufAllocLarge),
    NL_TEST_DEF("Allocate Medium Pool", TestPbufAllocMedium),
    NL_TEST_DEF("Allocate Small Pool", TestPbufAllocSmall),
    NL_TEST_DEF("Allocate Chained Pbuf", TestPbufAllocChain),
    NL_TEST_DEF("Allocate Chained Overlow Pbuf", TestPbufAllocChainFull),
    NL_TEST_DEF("Allocate Overflow Pbuf", TestPbufAllocOverflow),
    NL_TEST_SENTINEL()
};

#else // LWIP_PBUF_FROM_CUSTOM_POOLS
static void TestPbufExhaust(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf[PBUF_POOL_SIZE];
    struct pbuf *pbuf_overflow = NULL;
    uint8_t count;

    nlwatchdog_refresh();

    for (unsigned i = 0; i < PBUF_POOL_SIZE; i++)
    {
        pbuf[i] = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
        NL_TEST_ASSERT(inSuite, pbuf[i] != NULL);
        if (pbuf[i] == NULL)
        {
            // Clean up before returning
            for (unsigned j = 0; j < i-1; j++)
            {
                pbuf_free(pbuf[j]);
            }
            goto done;
        }
        NL_TEST_ASSERT(inSuite, pbuf[i]->next == NULL);
        NL_TEST_ASSERT(inSuite, pbuf[i]->len == PBUF_POOL_BUFSIZE);
        NL_TEST_ASSERT(inSuite, pbuf[i]->tot_len == PBUF_POOL_BUFSIZE);
    }

    pbuf_overflow = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
    NL_TEST_ASSERT(inSuite, pbuf_overflow == NULL);

    for (unsigned i = 0; i < PBUF_POOL_SIZE; i++)
    {
        count = pbuf_free(pbuf[i]);
        NL_TEST_ASSERT(inSuite, count == 1);
    }

done:
    // Make sure pbuf pools are empty again
    assert_pbuf_pools_empty(inSuite);
}

static void TestPbufAllocChain(nlTestSuite *inSuite, void *inContext)
{
    struct pbuf *pbuf = NULL;
    uint8_t count;

    nlwatchdog_refresh();
    // Allocate something that is guaranteed to spill over into
    // a chain of pbufs
    pbuf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE + 24, PBUF_POOL);
    NL_TEST_ASSERT(inSuite, pbuf != NULL);
    if (pbuf == NULL)
    {
        goto done;
    }
    NL_TEST_ASSERT(inSuite, pbuf->next != NULL);

    // Make sure we free exactly two pbufs
    count = pbuf_free(pbuf);
    NL_TEST_ASSERT(inSuite, count == 2);

done:
    // Make sure pbuf pools are empty again
    assert_pbuf_pools_empty(inSuite);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("Exhaust Pbuf Pool", TestPbufExhaust),
    NL_TEST_DEF("Allocate Chained Pbuf", TestPbufAllocChain),
    NL_TEST_SENTINEL()
};

#endif
//This function creates the Suite (i.e: the name of your test and points to the array of test functions)
int pbuftestsuite(void)
{
    nlTestSuite theSuite = {
        "pbuf",
        &sTests[0],
        NULL,
        NULL
    };
    nlTestRunner(&theSuite, NULL);
    return nlTestRunnerStats(&theSuite);
}
