#include "test_pbuf.h"

#include "lwip/pbuf.h"
#include "lwip/stats.h"

#if !LWIP_STATS || !MEM_STATS ||!MEMP_STATS
#error "This tests needs MEM- and MEMP-statistics enabled"
#endif

/* Setups/teardown functions */

static void
pbuf_setup(void)
{
}

static void
pbuf_teardown(void)
{
}


/* Test functions */

/** Call pbuf_copy on a pbuf with zero length */
START_TEST(test_pbuf_copy_zero_pbuf)
{
  struct pbuf *p1, *p2, *p3;
  err_t err;
  mem_size_t used = 0;
  LWIP_UNUSED_ARG(_i);

  used = lwip_stats.mem.used;
  fail_unless(lwip_stats.memp[MEMP_PBUF_POOL].used == 0);

  // should be bigger than PBUF_POOL_BUFSIZE_ALIGNED get pass this case
  p1 = pbuf_alloc(PBUF_RAW, LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE) * 2, PBUF_RAM);
  fail_unless(p1 != NULL);
  fail_unless(p1->ref == 1);

  p2 = pbuf_alloc(PBUF_RAW, 2, PBUF_POOL);
  fail_unless(p2 != NULL);
  fail_unless(p2->ref == 1);
  p2->len = p2->tot_len = 0;

  pbuf_cat(p1, p2);
  fail_unless(p1->ref == 1);
  fail_unless(p2->ref == 1);

  p3 = pbuf_alloc(PBUF_RAW, p1->tot_len, PBUF_POOL);
  err = pbuf_copy(p3, p1);
  fail_unless(err == ERR_VAL);

  pbuf_free(p1);
  pbuf_free(p3);

  fail_unless(lwip_stats.mem.used - used == 0);
  fail_unless(lwip_stats.memp[MEMP_PBUF_POOL].used == 0);
}
END_TEST

/** Call pbuf_alloced_custom() to make pbufs of different layers and lengths */
START_TEST(test_pbuf_alloced_custom)
{
  struct pbuf_custom p;
  struct pbuf *buf;
  char buffer[LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE)];
  const int len = 10;
  const int buffer_size = LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE);
  LWIP_UNUSED_ARG(_i);

  /* Create different layer pbufs by pbuf_alloced_custom() */
  memset(&p, 0, sizeof(struct pbuf_custom));
  buf = pbuf_alloced_custom(PBUF_TRANSPORT, len, PBUF_RAM, &p, buffer, buffer_size);
  fail_unless(buf->len == len);
  fail_unless(buf->payload != NULL);
  fail_unless(0 == pbuf_header(buf, PBUF_TRANSPORT_HLEN));
  fail_unless(0 == pbuf_header(buf, PBUF_IP_HLEN));
  fail_unless(0 == pbuf_header(buf, PBUF_LINK_HLEN));

  memset(&p, 0, sizeof(struct pbuf_custom));
  buf = pbuf_alloced_custom(PBUF_IP, len, PBUF_RAM, &p, buffer, buffer_size);
  fail_unless(buf->len == len);
  fail_unless(buf->payload != NULL);
  fail_unless(0 == pbuf_header(buf, PBUF_IP_HLEN));
  fail_unless(0 == pbuf_header(buf, PBUF_LINK_HLEN));

  memset(&p, 0, sizeof(struct pbuf_custom));
  buf = pbuf_alloced_custom(PBUF_LINK, len, PBUF_RAM, &p, buffer, buffer_size);
  fail_unless(buf->len == len);
  fail_unless(buf->payload != NULL);
  fail_unless(0 == pbuf_header(buf, PBUF_LINK_HLEN));

  /* bad pbuf layer cause failure */
  memset(&p, 0, sizeof(struct pbuf_custom));
  buf = pbuf_alloced_custom(-1, len, PBUF_RAM, &p, buffer, buffer_size);
  fail_unless(buf == NULL);

  /* payload length exceeds the buffer size cause failure */
  memset(&p, 0, sizeof(struct pbuf_custom));
  buf = pbuf_alloced_custom(PBUF_RAW, buffer_size + 1, PBUF_RAM, &p, buffer, buffer_size);
  fail_unless(buf == NULL);
}
END_TEST

/** Chain 3 pbufs */
START_TEST(test_pbuf_chain)
{
  struct pbuf *p1, *p2, *p3, *p;
  u16_t len1, len2, len3;
  LWIP_UNUSED_ARG(_i);

  len1 = 10;
  p1 = pbuf_alloc(PBUF_RAW, len1, PBUF_RAM);
  memset(p1->payload, 0, len1);
  len2 = 20;
  p2 = pbuf_alloc(PBUF_RAW, len2, PBUF_POOL);
  memset(p2->payload, 0, len2);
  len3 = 30;
  p3 = pbuf_alloc(PBUF_RAW, len3, PBUF_POOL);
  memset(p3->payload, 0, len3);

  /* fill some data in p1 and p2 */
  ((char*)p1->payload)[0] = 'a';
  ((char*)p1->payload)[1] = 'b';
  ((char*)p1->payload)[2] = 'c';
  ((char*)p2->payload)[0] = 'a';
  ((char*)p2->payload)[1] = 'b';

  /* pbuf_memcmp() returns 0 if equal,
     otherwise returns index of the first difference + 1 */
  fail_unless(0 == pbuf_memcmp(p1, 0, p2->payload, 2));
  fail_unless(1 == pbuf_memcmp(p1, 1, p2->payload, 2));

  pbuf_chain(p1, p2);
  fail_unless(p1->tot_len == len1 + len2);
  fail_unless(p2->ref == 2);
  fail_unless(0 == pbuf_memcmp(p1, len1, p2->payload, 2));

  fail_unless(p2 == pbuf_dechain(p1));
  fail_unless(p1->tot_len == len1);
  fail_unless(p2->tot_len == len2);

  pbuf_cat(p1, p2);
  pbuf_cat(p1, p3);

  fail_unless(p1->tot_len == len1 + len2 + len3);
  fail_unless('a' == pbuf_get_at(p1, len1));

  p = pbuf_coalesce(p1, PBUF_RAW);
  fail_unless(p->tot_len == len1 + len2 + len3);
  fail_unless(p->len == len1 + len2 + len3);
  fail_unless('a' == pbuf_get_at(p, len1));

  /* pbuf_strstr() returns 0xffff if not found */
  fail_unless(1 == pbuf_strstr(p, "bc"));
  fail_unless(0xffff == pbuf_strstr(p, NULL));
  fail_unless(0xffff == pbuf_strstr(p, "cc"));

  pbuf_free(p);
}
END_TEST

START_TEST(test_pbuf_alloc)
{
  struct pbuf *p;
  LWIP_UNUSED_ARG(_i);

  /* pbuf_alloc() will not allocate memory for PBUF_REF type pbuf's payload */
  p = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
  fail_unless(p->payload == NULL);
  pbuf_free(p);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
pbuf_suite(void)
{
  TFun tests[] = {
    test_pbuf_chain,
    test_pbuf_alloced_custom,
    test_pbuf_alloc,
    test_pbuf_copy_zero_pbuf
  };
  return create_suite("PBUF", tests, sizeof(tests)/sizeof(TFun), pbuf_setup, pbuf_teardown);
}
