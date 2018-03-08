#include "test_pbuf.h"

#include "lwip/pbuf.h"
#include "lwip/stats.h"

#if !LWIP_STATS || !MEM_STATS ||!MEMP_STATS
#error "This tests needs MEM- and MEMP-statistics enabled"
#endif
#if LWIP_DNS
#warning "This test needs DNS turned off (as it mallocs on init)"
#endif
#if !LWIP_TCP || !TCP_QUEUE_OOSEQ || !LWIP_WND_SCALE
#error "This test needs TCP OOSEQ queueing and window scaling enabled"
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


#define TESTBUFSIZE_1 65535
#define TESTBUFSIZE_2 65530
#define TESTBUFSIZE_3 50050
static u8_t testbuf_1[TESTBUFSIZE_1];
static u8_t testbuf_1a[TESTBUFSIZE_1];
static u8_t testbuf_2[TESTBUFSIZE_2];
static u8_t testbuf_2a[TESTBUFSIZE_2];
static u8_t testbuf_3[TESTBUFSIZE_3];
static u8_t testbuf_3a[TESTBUFSIZE_3];

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

START_TEST(test_pbuf_split_64k_on_small_pbufs)
{
  struct pbuf *p, *rest=NULL;
  LWIP_UNUSED_ARG(_i);

  p = pbuf_alloc(PBUF_RAW, 1, PBUF_POOL);
  pbuf_split_64k(p, &rest);
  fail_unless(p->tot_len == 1);
  pbuf_free(p);
}
END_TEST

START_TEST(test_pbuf_queueing_bigger_than_64k)
{
  int i;
  err_t err;
  struct pbuf *p1, *p2, *p3, *rest2=NULL, *rest3=NULL;
  LWIP_UNUSED_ARG(_i);

  for(i = 0; i < TESTBUFSIZE_1; i++) {
    testbuf_1[i] = (u8_t)rand();
  }
  for(i = 0; i < TESTBUFSIZE_2; i++) {
    testbuf_2[i] = (u8_t)rand();
  }
  for(i = 0; i < TESTBUFSIZE_3; i++) {
    testbuf_3[i] = (u8_t)rand();
  }

  p1 = pbuf_alloc(PBUF_RAW, TESTBUFSIZE_1, PBUF_POOL);
  fail_unless(p1 != NULL);
  p2 = pbuf_alloc(PBUF_RAW, TESTBUFSIZE_2, PBUF_POOL);
  fail_unless(p2 != NULL);
  p3 = pbuf_alloc(PBUF_RAW, TESTBUFSIZE_3, PBUF_POOL);
  fail_unless(p3 != NULL);
  err = pbuf_take(p1, testbuf_1, TESTBUFSIZE_1);
  fail_unless(err == ERR_OK);
  err = pbuf_take(p2, testbuf_2, TESTBUFSIZE_2);
  fail_unless(err == ERR_OK);
  err = pbuf_take(p3, testbuf_3, TESTBUFSIZE_3);
  fail_unless(err == ERR_OK);

  pbuf_cat(p1, p2);
  pbuf_cat(p1, p3);

  pbuf_split_64k(p1, &rest2);
  fail_unless(p1->tot_len == TESTBUFSIZE_1);
  fail_unless(rest2->tot_len == (u16_t)((TESTBUFSIZE_2+TESTBUFSIZE_3) & 0xFFFF));
  pbuf_split_64k(rest2, &rest3);
  fail_unless(rest2->tot_len == TESTBUFSIZE_2);
  fail_unless(rest3->tot_len == TESTBUFSIZE_3);

  pbuf_copy_partial(p1, testbuf_1a, TESTBUFSIZE_1, 0);
  pbuf_copy_partial(rest2, testbuf_2a, TESTBUFSIZE_2, 0);
  pbuf_copy_partial(rest3, testbuf_3a, TESTBUFSIZE_3, 0);
  for(i = 0; i < TESTBUFSIZE_1; i++)
    fail_unless(testbuf_1[i] == testbuf_1a[i]);
  for(i = 0; i < TESTBUFSIZE_2; i++)
    fail_unless(testbuf_2[i] == testbuf_2a[i]);
  for(i = 0; i < TESTBUFSIZE_3; i++)
    fail_unless(testbuf_3[i] == testbuf_3a[i]);

  pbuf_free(p1);
  pbuf_free(rest2);
  pbuf_free(rest3);
}
END_TEST

/* Test for bug that writing with pbuf_take_at() did nothing
 * and returned ERR_OK when writing at beginning of a pbuf
 * in the chain.
 */
START_TEST(test_pbuf_take_at_edge)
{
  err_t res;
  u8_t *out;
  int i;
  u8_t testdata[] = { 0x01, 0x08, 0x82, 0x02 };
  struct pbuf *p = pbuf_alloc(PBUF_RAW, 1024, PBUF_POOL);
  struct pbuf *q = p->next;
  LWIP_UNUSED_ARG(_i);
  /* alloc big enough to get a chain of pbufs */
  fail_if(p->tot_len == p->len);
  memset(p->payload, 0, p->len);
  memset(q->payload, 0, q->len);

  /* copy data to the beginning of first pbuf */
  res = pbuf_take_at(p, &testdata, sizeof(testdata), 0);
  fail_unless(res == ERR_OK);

  out = (u8_t*)p->payload;
  for (i = 0; i < (int)sizeof(testdata); i++) {
    fail_unless(out[i] == testdata[i],
      "Bad data at pos %d, was %02X, expected %02X", i, out[i], testdata[i]);
  }

  /* copy data to the just before end of first pbuf */
  res = pbuf_take_at(p, &testdata, sizeof(testdata), p->len - 1);
  fail_unless(res == ERR_OK);

  out = (u8_t*)p->payload;
  fail_unless(out[p->len - 1] == testdata[0],
    "Bad data at pos %d, was %02X, expected %02X", p->len - 1, out[p->len - 1], testdata[0]);
  out = (u8_t*)q->payload;
  for (i = 1; i < (int)sizeof(testdata); i++) {
    fail_unless(out[i-1] == testdata[i],
      "Bad data at pos %d, was %02X, expected %02X", p->len - 1 + i, out[i-1], testdata[i]);
  }

  /* copy data to the beginning of second pbuf */
  res = pbuf_take_at(p, &testdata, sizeof(testdata), p->len);
  fail_unless(res == ERR_OK);

  out = (u8_t*)p->payload;
  for (i = 0; i < (int)sizeof(testdata); i++) {
    fail_unless(out[i] == testdata[i],
      "Bad data at pos %d, was %02X, expected %02X", p->len+i, out[i], testdata[i]);
  }
}
END_TEST

/* Verify pbuf_put_at()/pbuf_get_at() when using
 * offsets equal to beginning of new pbuf in chain
 */
START_TEST(test_pbuf_get_put_at_edge)
{
  u8_t *out;
  u8_t testdata = 0x01;
  u8_t getdata;
  struct pbuf *p = pbuf_alloc(PBUF_RAW, 1024, PBUF_POOL);
  struct pbuf *q = p->next;
  LWIP_UNUSED_ARG(_i);
  /* alloc big enough to get a chain of pbufs */
  fail_if(p->tot_len == p->len);
  memset(p->payload, 0, p->len);
  memset(q->payload, 0, q->len);

  /* put byte at the beginning of second pbuf */
  pbuf_put_at(p, p->len, testdata);

  out = (u8_t*)q->payload;
  fail_unless(*out == testdata,
    "Bad data at pos %d, was %02X, expected %02X", p->len, *out, testdata);

  getdata = pbuf_get_at(p, p->len);
  fail_unless(*out == getdata,
    "pbuf_get_at() returned bad data at pos %d, was %02X, expected %02X", p->len, getdata, *out);
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
  testfunc tests[] = {
    TESTFUNC(test_pbuf_copy_zero_pbuf),
    TESTFUNC(test_pbuf_split_64k_on_small_pbufs),
    TESTFUNC(test_pbuf_queueing_bigger_than_64k),
    TESTFUNC(test_pbuf_take_at_edge),
    TESTFUNC(test_pbuf_get_put_at_edge),
    TESTFUNC(test_pbuf_alloced_custom),
    TESTFUNC(test_pbuf_chain),
    TESTFUNC(test_pbuf_alloc)
  };
  return create_suite("PBUF", tests, sizeof(tests)/sizeof(testfunc), pbuf_setup, pbuf_teardown);
}
