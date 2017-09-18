#ifndef __LWIP_CHECK_H__
#define __LWIP_CHECK_H__

/* Common header file for lwIP unit tests using the check framework */

#ifndef NO_CONFIG_H
#include <config.h>
#endif
#include <check.h>
#include <stdlib.h>

#define FAIL_RET() do { fail(); return; } while(0)
#define EXPECT(x) fail_unless(x)
#define EXPECT_RET(x) do { fail_unless(x); if(!(x)) { return; }} while(0)
#define EXPECT_RETX(x, y) do { fail_unless(x); if(!(x)) { return y; }} while(0)
#define EXPECT_RETNULL(x) EXPECT_RETX(x, NULL)

/** typedef for a function returning a test suite */
typedef Suite* (suite_getter_fn)(void);

/** Create a test suite */
Suite* create_suite(const char* name, TFun *tests, size_t num_tests, SFun setup, SFun teardown);

#endif /* __LWIP_CHECK_H__ */
