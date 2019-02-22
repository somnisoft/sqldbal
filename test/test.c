/**
 * @file
 * @brief Test the SQLDBAL library.
 * @author James Humphrey (mail@somnisoft.com)
 * @version 0.99
 *
 * This SQLDBAL testing framework has 100% branch coverage on POSIX
 * systems.
 *
 * This test framework requires the following:
 *   - MariaDB server
 *   - PostgreSQL server
 *   - SQLite library
 *
 * This software has been placed into the public domain using CC0.
 */
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "test.h"

/**
 * Maximum number of parameters allowed in @ref g_q.
 */
#define MAX_QUERY_PARAMS 100

/**
 * Number of statements to create at once in
 * @ref sqldbal_functional_test_multiple_statements.
 */
#define NUM_STATEMENTS_MULTI_TEST 101

/**
 * Maximum number of bytes in SQL string used by @ref g_sql.
 */
#define SQLDBAL_TEST_SQL_LIMIT 1000

/**
 * Parameterized placeholders to use when constructing queries.
 *
 * PostgreSQL uses integer placeholders for parameterized statements. Other
 * drivers use '?' for placeholders. This global allows constructing queries
 * that work with all databases.
 *
 * PostgreSQL example:
 * INSERT INTO test VALUES($1, $2);
 *
 * MariaDB/SQLite example:
 * INSERT INTO test VALUES(?, ?);
 *
 * See @ref sqldbal_test_stmt_generate_placeholders.
 */
static char g_q[MAX_QUERY_PARAMS][4];

/**
 * Test data structure used to add article rows to the databases.
 *
 * See @ref g_article_list.
 */
struct sqldbal_test_article{
  /**
   * Primary key.
   */
  int article_id;

  /**
   * Author name.
   */
  const char *const author;

  /**
   * Article title.
   */
  const char *const title;

  /**
   * Number of views per article.
   *
   * Tests integer type.
   */
  size_t view_count;

  /**
   * Article content.
   *
   * Stored as binary data.
   */
  const char *const content;
};

/**
 * Example data to add to database for testing INSERT and SELECT.
 */
static struct sqldbal_test_article
g_article_list[] = {
  {
    1,
    "somnisoft",
    "SQLDBAL",
    100,
    "SQL Database Abstraction Library"
  },
  {
    2,
    "James Humphrey",
    "SQLDBAL Testing Framework",
    9,
    "This test framework has full branch coverage"
  },
  {
    3,
    "Anonymous",
    "Test Article",
    1,
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY0123456789"
  },
  {
    4,
    "",
    "abc",
    1,
    "test"
  }
};

/**
 * Number of articles in @ref g_article_list.
 */
static const size_t
g_num_articles = sizeof(g_article_list) / sizeof(g_article_list[0]);

/**
 * Database connection parameters for the test databases used by this testing
 * framework.
 *
 *   - 0 MariaDB
 *   - 1 PostgreSQL
 *   - 2 SQLite
 */
static struct sqldbal_test_db_config
g_db_config_list[3];

/**
 * Number of test databases in @ref g_db_config_list.
 */
static const size_t
g_db_num = sizeof(g_db_config_list) / sizeof(g_db_config_list[0]);

/**
 * Convenience global that tests can use when creating databases,
 * so that they do not need to redeclare this in every function that needs
 * a database handle.
 */
static struct sqldbal_db *g_db;

/**
 * Convenience global that tests can use when preparing statements, so
 * that they do not need to redeclare this in every function that needs a
 * statement handle.
 */
static struct sqldbal_stmt *g_stmt;

/**
 * Convenience global that tests can use to easily check sqldbal status and
 * throw errors in response.
 */
static enum sqldbal_status_code g_rc = SQLDBAL_STATUS_OK;

/**
 * Convenience global that tests can use to easily check the fetch status
 * so that they do not need to declare a variable in every function this
 * occurrs in.
 */
static enum sqldbal_fetch_result g_fetch_rc;

/**
 * Convenience global that allows tests to save constructed SQL statements
 * to.
 */
static char g_sql[SQLDBAL_TEST_SQL_LIMIT];

/**
 * Test scenarios with invalid SQL command.
 */
static const char *const g_sql_invalid =
"INVALID SQL COMMAND";

/**
 * Test scenarios where we need a valid SQL command, but we don't care about
 * the results.
 */
static const char *const g_sql_valid_sel =
"SELECT * FROM article";

/**
 * Get the index to use in @ref g_db_config_list for the corresponding driver
 * type.
 *
 * @param[in] driver See @ref sqldbal_driver.
 * @return Index value in @ref g_db_config_list.
 */
static size_t
sqldbal_test_get_driver_config_i(enum sqldbal_driver driver){
  size_t i;

  for(i = 0; g_db_config_list[i].driver != driver; i++){
  }
  return i;
}

/**
 * Test harness for @ref sqldbal_str_hex2bin.
 *
 * @param[in] hex_str    Hex string to convert to binary.
 * @param[in] expect_str Expected binary string value.
 * @param[in] expect_len Expected length of binary data.
 */
static void
sqldbal_unit_test_hex2bin(const char *const hex_str,
                          const char *const expect_str,
                          size_t expect_len){
  char *result;
  size_t result_len;

  result = sqldbal_str_hex2bin(hex_str, &result_len);
  if(expect_str){
    assert(result_len == expect_len);
    assert(memcmp(result, expect_str, expect_len) == 0);
    free(result);
  }
  else{
    assert(result == expect_str);
  }
}

/**
 * Run all test cases for @ref sqldbal_str_hex2bin.
 */
static void
sqldbal_unit_test_all_hex2bin(void){
  sqldbal_unit_test_hex2bin("", "", 0);
  sqldbal_unit_test_hex2bin("0", NULL, 0);
  sqldbal_unit_test_hex2bin("1", NULL, 0);
  sqldbal_unit_test_hex2bin("ZZ", NULL, 0);
  sqldbal_unit_test_hex2bin("0Z", NULL, 0);
  sqldbal_unit_test_hex2bin("Z0", NULL, 0);
  sqldbal_unit_test_hex2bin("004500", "\0E\0", 3);
  sqldbal_unit_test_hex2bin("202345", " #E", 3);
  sqldbal_unit_test_hex2bin("34545567", "4TUg", 4);

  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_unit_test_hex2bin("00", NULL, 0);
  g_sqldbal_err_malloc_ctr = -1;
}

/**
 * Test harness for @ref sqldbal_reallocarray.
 *
 * @param[in] ptr          Pointer to existing memory allocation, or NULL
 *                         to allocate a new memory region.
 * @param[in] nelem        Number of elements in the array.
 * @param[in] size         Number of bytes for each @p nelem.
 * @param[in] expect_alloc 1 if the allocation should succeed, or 0 if it
 *                         should fail.
 */
static void
sqldbal_unit_test_reallocarray(void *ptr,
                               size_t nelem,
                               size_t size,
                               int expect_alloc){
  void *result;

  result = sqldbal_reallocarray(ptr, nelem, size);
  if(expect_alloc){
    assert(result);
    free(result);
  }
  else{
    assert(result == NULL);
  }
}

/**
 * Run all test cases for @ref sqldbal_reallocarray.
 */
static void
sqldbal_unit_test_all_reallocarray(void){
  sqldbal_unit_test_reallocarray(NULL, 1, 1, 1);
  sqldbal_unit_test_reallocarray(NULL, 0, 1, 1);
  sqldbal_unit_test_reallocarray(NULL, 1, 0, 1);

  /* unsigned wrapping */
  sqldbal_unit_test_reallocarray(NULL, SIZE_MAX, 2, 0);

  /* realloc */
  g_sqldbal_err_realloc_ctr = 0;
  sqldbal_unit_test_reallocarray(NULL, 2, 2, 0);
  g_sqldbal_err_realloc_ctr = -1;
}

/**
 * Test harness for @ref si_add_size_t and @ref si_mul_size_t.
 *
 * @param[in]  si_fp         One of the si wrapping functions for size_t types.
 * @param[in]  a             Add this value with @p b.
 * @param[in]  b             Add this value with @p a.
 * @param[in]  expect_result Expected result of adding @p a and @p b.
 * @param[in]  expect_wrap   Expectation on whether the addition will wrap.
 */
static void
sqldbal_unit_test_si_size_t(int (*si_fp)(const size_t a,
                                         const size_t b,
                                         size_t *const result),
                            size_t a,
                            size_t b,
                            size_t expect_result,
                            int expect_wrap){
  int wraps;
  size_t result;

  wraps = si_fp(a, b, &result);
  assert(wraps == expect_wrap);
  assert(result == expect_result);
}

/**
 * Test harness for @ref si_size_to_uint.
 *
 * @param[in] size            Value to convert.
 * @param[in] expect_result   Expected converted value.
 * @param[in] expect_truncate Expected truncation result.
 */
static void
sqldbal_unit_test_si_size_to_uint(const size_t size,
                                  unsigned int expect_result,
                                  int expect_truncate){
  int truncate;
  unsigned int result;

  truncate = si_size_to_uint(size, &result);
  assert(truncate == expect_truncate);
  if(!truncate){
    assert(result == expect_result);
  }
}

/**
 * Test harness for @ref si_int64_to_llong.
 *
 * @param[in] i64             Value to convert.
 * @param[in] expect_result   Expected converted value.
 * @param[in] expect_overflow Expected overflow result.
 */
static void
sqldbal_unit_test_si_int64_to_llong(const int64_t i64,
                                    long long expect_result,
                                    int expect_overflow){
  int overflow;
  long long result;

  overflow = si_int64_to_llong(i64, &result);
  assert(overflow == expect_overflow);
  if(!overflow){
    assert(result == expect_result);
  }
}

/**
 * Test harness for @ref si_llong_to_int64.
 *
 * @param[in] lli             Value to convert.
 * @param[in] expect_result   Expected converted value.
 * @param[in] expect_overflow Expected overflow result.
 */
static void
sqldbal_unit_test_si_llong_to_int64(const long long int lli,
                                    int64_t expect_result,
                                    int expect_overflow){
  int overflow;
  int64_t result;

  overflow = si_llong_to_int64(lli, &result);
  assert(overflow == expect_overflow);
  if(!overflow){
    assert(result == expect_result);
  }
}

/**
 * Test harness for @ref si_size_to_int.
 *
 * @param[in] size            Value to convert.
 * @param[in] expect_result   Expected converted value.
 * @param[in] expect_overflow Expected overflow result.
 */
static void
sqldbal_unit_test_si_size_to_int(const size_t size,
                                 int expect_result,
                                 int expect_overflow){
  int overflow;
  int result;

  overflow = si_size_to_int(size, &result);
  assert(overflow == expect_overflow);
  if(!overflow){
    assert(result == expect_result);
  }
}

/**
 * Test harness for @ref si_int_to_size.
 *
 * @param[in] i             Value to convert.
 * @param[in] expect_result Expected converted value.
 * @param[in] expect_wrap   Expected wrap result.
 */
static void
sqldbal_unit_test_si_int_to_size(const int i,
                                 size_t expect_result,
                                 int expect_wrap){
  int wrap;
  size_t result;

  wrap = si_int_to_size(i, &result);
  assert(wrap == expect_wrap);
  if(!wrap){
    assert(result == expect_result);
  }
}

/**
 * Test harness for @ref si_int64_to_uint64.
 *
 * @param[in] i64             Value to convert.
 * @param[in] expect_result   Expected converted value.
 * @param[in] expect_overflow Expected overflow result.
 */
static void
sqldbal_unit_test_si_int64_to_uint64(const int64_t i64,
                                     uint64_t expect_result,
                                     int expect_overflow){
  int overflow;
  uint64_t result;

  overflow = si_int64_to_uint64(i64, &result);
  assert(overflow == expect_overflow);
  if(!overflow){
    assert(result == expect_result);
  }
}

/**
 * Run all test cases for integer wrapping and conversions.
 */
static void
sqldbal_unit_test_all_si(void){
  /* add */
  sqldbal_unit_test_si_size_t(si_add_size_t, 0, 0, 0, 0);
  sqldbal_unit_test_si_size_t(si_add_size_t, 0, 1, 1, 0);
  sqldbal_unit_test_si_size_t(si_add_size_t, 1, 0, 1, 0);
  sqldbal_unit_test_si_size_t(si_add_size_t, 1, 1, 2, 0);
  sqldbal_unit_test_si_size_t(si_add_size_t, SIZE_MAX, 0, SIZE_MAX, 0);
  sqldbal_unit_test_si_size_t(si_add_size_t, SIZE_MAX, 1, 0, 1);
  sqldbal_unit_test_si_size_t(si_add_size_t, SIZE_MAX - 1, 2, 0, 1);
  sqldbal_unit_test_si_size_t(si_add_size_t, SIZE_MAX, 2, 1, 1);
  sqldbal_unit_test_si_size_t(si_add_size_t,
                              SIZE_MAX / 2,
                              SIZE_MAX / 2,
                              SIZE_MAX - 1,
                              0);

  /* multiply */
  sqldbal_unit_test_si_size_t(si_mul_size_t, 0, 0, 0, 0);
  sqldbal_unit_test_si_size_t(si_mul_size_t, 0, 1, 0, 0);
  sqldbal_unit_test_si_size_t(si_mul_size_t, 1, 0, 0, 0);
  sqldbal_unit_test_si_size_t(si_mul_size_t, 1, 1, 1, 0);
  sqldbal_unit_test_si_size_t(si_mul_size_t, 100, 12, 1200, 0);
  sqldbal_unit_test_si_size_t(si_mul_size_t, SIZE_MAX, 1, SIZE_MAX, 0);
  sqldbal_unit_test_si_size_t(si_mul_size_t, SIZE_MAX, 2, SIZE_MAX * 2, 1);

  /* convert size_t to unsigned int */
  sqldbal_unit_test_si_size_to_uint(0, 0, 0);
  sqldbal_unit_test_si_size_to_uint(10, 10, 0);
  sqldbal_unit_test_si_size_to_uint(SIZE_MAX, UINT_MAX, 1);
  sqldbal_unit_test_si_size_to_uint((size_t)UINT_MAX + 1, 0, 1);

  /* convert int64_t to long long */
  sqldbal_unit_test_si_int64_to_llong(0, 0, 0);
  sqldbal_unit_test_si_int64_to_llong(-10, -10, 0);
  sqldbal_unit_test_si_int64_to_llong(10, 10, 0);
  g_sqldbal_err_si_int64_to_llong_ctr = 0;
  sqldbal_unit_test_si_int64_to_llong(0, 0, 1);
  g_sqldbal_err_si_int64_to_llong_ctr = -1;

  /* convert long long to int64_t */
  sqldbal_unit_test_si_llong_to_int64(0, 0, 0);
  sqldbal_unit_test_si_llong_to_int64(10, 10, 0);
  sqldbal_unit_test_si_llong_to_int64(-10, -10, 0);
  g_sqldbal_err_si_llong_to_int64_ctr = 0;
  sqldbal_unit_test_si_llong_to_int64(0, 0, 1);
  g_sqldbal_err_si_llong_to_int64_ctr = -1;

  /* convert size_t to int */
  sqldbal_unit_test_si_size_to_int(0, 0, 0);
  sqldbal_unit_test_si_size_to_int(10, 10, 0);
  sqldbal_unit_test_si_size_to_int(INT_MAX, INT_MAX, 0);
  sqldbal_unit_test_si_size_to_int(SIZE_MAX, INT_MAX, 1);
  sqldbal_unit_test_si_size_to_int((size_t)INT_MAX + 1, 0, 1);

  /* convert int to size_t */
  sqldbal_unit_test_si_int_to_size(0, 0, 0);
  sqldbal_unit_test_si_int_to_size(10, 10, 0);
  sqldbal_unit_test_si_int_to_size(INT_MAX, INT_MAX, 0);
  sqldbal_unit_test_si_int_to_size(-1, SIZE_MAX, 1);

  /* convert int64_t to uint64_t */
  sqldbal_unit_test_si_int64_to_uint64(0, 0, 0);
  sqldbal_unit_test_si_int64_to_uint64(10, 10, 0);
  sqldbal_unit_test_si_int64_to_uint64(INT64_MAX, INT64_MAX, 0);
  sqldbal_unit_test_si_int64_to_uint64(-1, UINT64_MAX, 1);
}

/**
 * Test harness for @ref sqldbal_stpcpy.
 *
 * @param[in] init   Set the destination buffer to this initial string.
 * @param[in] s2     Concatenate this string into the destination buffer.
 * @param[in] expect Expected string result.
 */
static void
sqldbal_unit_test_stpcpy(const char *const init,
                         const char *const s2,
                         const char *const expect){
  char *buf;
  size_t bufsz;
  char *endptr;
  char *expect_ptr;

  bufsz = strlen(init) + strlen(s2) + 1;
  buf = malloc(bufsz);
  assert(buf);

  strcpy(buf, init);
  endptr = buf + strlen(init);

  endptr = sqldbal_stpcpy(endptr, s2);
  expect_ptr = buf + bufsz - 1;
  assert(endptr == expect_ptr);
  assert(*endptr == '\0');
  assert(strcmp(buf, expect) == 0);
  free(buf);
}

/**
 * Run all test cases for @ref sqldbal_stpcpy.
 */
static void
sqldbal_unit_test_all_stpcpy(void){
  sqldbal_unit_test_stpcpy("", "", "");
  sqldbal_unit_test_stpcpy("", "a", "a");
  sqldbal_unit_test_stpcpy("", "ab", "ab");
  sqldbal_unit_test_stpcpy("", "abc", "abc");

  sqldbal_unit_test_stpcpy("a", "", "a");
  sqldbal_unit_test_stpcpy("ab", "", "ab");
  sqldbal_unit_test_stpcpy("abc", "", "abc");

  sqldbal_unit_test_stpcpy("a", "a", "aa");
}

/**
 * Test harness for @ref sqldbal_strdup.
 *
 * @param[in] s      String to duplicate.
 * @param[in] expect Expected string result.
 */
static void
sqldbal_unit_test_strdup(const char *const s,
                         const char *const expect){
  char *result;

  result = sqldbal_strdup(s);
  if(expect){
    assert(strcmp(result, expect) == 0);
    free(result);
  }
  else{ /* NULL */
    assert(result == expect);
  }
}

/**
 * Run all test cases for @ref sqldbal_strdup.
 */
static void
sqldbal_unit_test_all_strdup(void){
  sqldbal_unit_test_strdup("", "");
  sqldbal_unit_test_strdup("a", "a");
  sqldbal_unit_test_strdup("ab", "ab");

  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_unit_test_strdup("", NULL);
  g_sqldbal_err_malloc_ctr = -1;

  g_sqldbal_err_strlen_ctr = 0;
  g_sqldbal_err_strlen_ret_value = SIZE_MAX;
  sqldbal_unit_test_strdup("a", NULL);
  g_sqldbal_err_strlen_ctr = -1;
  g_sqldbal_err_strlen_ret_value = 0;
}

/**
 * Convenience function that creates a database handle to a SQLite database.
 *
 * The caller must call @ref sqldbal_close on the @ref g_db reference
 * when done.
 */
static void
sqldbal_test_open_db_sqlite(void){
  g_rc = sqldbal_open(SQLDBAL_DRIVER_SQLITE,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      SQLDBAL_FLAG_NONE,
                      NULL,
                      0,
                      &g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Test harness for @ref sqldbal_strtoui.
 *
 * @param[in] str           See @ref sqldbal_strtoui.
 * @param[in] maxval        See @ref sqldbal_strtoui.
 * @param[in] expect_ui     Expected conversion integer.
 * @param[in] expect_status Expected status code response.
 */
static void
sqldbal_unit_test_strtoui(const char *const str,
                          unsigned int maxval,
                          unsigned int expect_ui,
                          enum sqldbal_status_code expect_status){
  unsigned int result;

  sqldbal_test_open_db_sqlite();
  g_rc = sqldbal_strtoui(g_db, str, maxval, &result);
  assert(g_rc == expect_status);
  assert(result == expect_ui);
  sqldbal_close(g_db);
}

/**
 * Run all test cases for @ref sqldbal_strtoui.
 */
static void
sqldbal_unit_test_all_strtoui(void){
  sqldbal_unit_test_strtoui(NULL, 0, 0, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoui(NULL, 1, 0, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoui(NULL, 10, 0, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoui("0", 0, 0, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoui("0", 10, 0, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoui("1", 10, 1, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoui("1", 1, 1, SQLDBAL_STATUS_OK);

  sqldbal_unit_test_strtoui("", 10, 0, SQLDBAL_STATUS_PARAM);
  sqldbal_unit_test_strtoui("1", 0, 0, SQLDBAL_STATUS_PARAM);
  sqldbal_unit_test_strtoui("abc", 10, 0, SQLDBAL_STATUS_PARAM);
  sqldbal_unit_test_strtoui("1abc", 10, 0, SQLDBAL_STATUS_PARAM);
}

/**
 * Test harness for @ref sqldbal_strtoi64.
 *
 * @param[in] str           See @ref sqldbal_strtoi64.
 * @param[in] expect_int64  Expected conversion integer.
 * @param[in] expect_status Expected status code response.
 */
static void
sqldbal_unit_test_strtoi64(const char *const str,
                           int64_t expect_int64,
                           enum sqldbal_status_code expect_status){
  int64_t result;

  sqldbal_test_open_db_sqlite();
  sqldbal_strtoi64(g_db, str, &result);
  g_rc = sqldbal_status_code_get(g_db);
  assert(g_rc == expect_status);
  assert(result == expect_int64);
  sqldbal_close(g_db);
}

/**
 * Run all test cases for @ref sqldbal_strtoi64.
 */
static void
sqldbal_unit_test_all_strtoi64(void){
  sqldbal_unit_test_strtoi64("", 0, SQLDBAL_STATUS_COLUMN_COERCE);
  sqldbal_unit_test_strtoi64("a", 0, SQLDBAL_STATUS_COLUMN_COERCE);
  sqldbal_unit_test_strtoi64("1", 1, SQLDBAL_STATUS_OK);
  sqldbal_unit_test_strtoi64("-1", -1, SQLDBAL_STATUS_OK);

  g_sqldbal_err_strtoll_ctr = 0;
  g_sqldbal_err_strtoll_value = LLONG_MAX;
  sqldbal_unit_test_strtoi64("1", 0, SQLDBAL_STATUS_COLUMN_COERCE);
  g_sqldbal_err_strtoll_ctr = -1;
  g_sqldbal_err_strtoll_value = -1;

  g_sqldbal_err_strtoll_ctr = 0;
  g_sqldbal_err_strtoll_value = LLONG_MIN;
  sqldbal_unit_test_strtoi64("1", 0, SQLDBAL_STATUS_COLUMN_COERCE);
  g_sqldbal_err_strtoll_ctr = -1;
  g_sqldbal_err_strtoll_value = -1;

  /* ERANGE but not out of range */
  g_sqldbal_err_strtoll_ctr = 0;
  g_sqldbal_err_strtoll_value = 1;
  sqldbal_unit_test_strtoi64("1", 1, SQLDBAL_STATUS_OK);
  g_sqldbal_err_strtoll_ctr = -1;
  g_sqldbal_err_strtoll_value = -1;

  g_sqldbal_err_si_llong_to_int64_ctr = 0;
  sqldbal_unit_test_strtoi64("1", 0, SQLDBAL_STATUS_COLUMN_COERCE);
  g_sqldbal_err_si_llong_to_int64_ctr = -1;
}

/**
 * Unit testing functions.
 */
static void
sqldbal_unit_test_all(void){
  sqldbal_unit_test_all_hex2bin();
  sqldbal_unit_test_all_reallocarray();
  sqldbal_unit_test_all_si();
  sqldbal_unit_test_all_stpcpy();
  sqldbal_unit_test_all_strdup();
  sqldbal_unit_test_all_strtoui();
  sqldbal_unit_test_all_strtoi64();
}

/**
 * Open a database connection with default settings.
 *
 * @param[in] driver See @ref sqldbal_driver.
 */
static void
sqldbal_test_db_open(enum sqldbal_driver driver){
  size_t driver_config_i;

  driver_config_i = sqldbal_test_get_driver_config_i(driver);
  g_rc = sqldbal_open(driver,
                      g_db_config_list[driver_config_i].location,
                      g_db_config_list[driver_config_i].port,
                      g_db_config_list[driver_config_i].username,
                      g_db_config_list[driver_config_i].password,
                      g_db_config_list[driver_config_i].database,
                      g_db_config_list[driver_config_i].flags,
                      NULL,
                      0,
                      &g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Prepare the @ref g_sql SQL statement and check the result.
 */
static void
sqldbal_test_stmt_prepare_sql(void){
  g_rc = sqldbal_stmt_prepare(g_db, g_sql, SIZE_MAX, &g_stmt);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Close global statement and check the result.
 */
static void
sqldbal_test_stmt_close_sql(void){
  g_rc = sqldbal_stmt_close(g_stmt);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Close the database handle cleanly and check the result.
 */
static void
sqldbal_test_db_close(void){
  sqldbal_status_code_clear(g_db);
  g_rc = sqldbal_close(g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Execute a SQL query, but do not perform the callback routine.
 *
 * Useful for INSERT and UPDATE actions. A callback routine would normally
 * get used if comparing returned SELECT data.
 *
 * @param[in] sql SQL query to exec.
 */
static void
sqldbal_test_exec_plain(const char *const sql){
  g_rc = sqldbal_exec(g_db,
                      sql,
                      NULL,
                      NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Return the database type used to store binary data.
 *
 * @retval "BYTEA" Returned for PostgreSQL database driver.
 * @retval "BLOB"  Returned for all other database drivers.
 */
static const char *
sqldbal_test_get_db_blob_type(void){
  if(sqldbal_driver_type(g_db) == SQLDBAL_DRIVER_POSTGRESQL){
    return "BYTEA";
  }
  else{
    return "BLOB";
  }
}

/**
 * Return the sequence data type used when creating tables.
 *
 * @retval "SERIAL"  Returned for PostgreSQL database driver.
 * @retval "INTEGER" Returned for all other database drivers.
 */
static const char *
sqldbal_test_get_db_seq_type(void){
  if(sqldbal_driver_type(g_db) == SQLDBAL_DRIVER_POSTGRESQL){
    return "SERIAL";
  }
  else{
    return "INTEGER";
  }
}

/**
 * Return the sequence attribute to describe the primary key sequence
 * when creating tables.
 *
 * @retval ""               Returned for PostgreSQL database driver.
 * @retval "AUTO_INCREMENT" Returned for all other database drivers.
 */
static const char *
sqldbal_test_get_db_seq_attribute(void){
  if(sqldbal_driver_type(g_db) == SQLDBAL_DRIVER_POSTGRESQL){
    return "";
  }
  else{
    return "AUTO_INCREMENT";
  }
}

/**
 * Drop and recreate all test tables in the database.
 */
static void
sqldbal_functional_test_create_table(void){
  const char *blob_type;
  const char *seq_type;
  const char *seq_attribute;

  blob_type = sqldbal_test_get_db_blob_type();
  seq_type = sqldbal_test_get_db_seq_type();
  seq_attribute = sqldbal_test_get_db_seq_attribute();

  sprintf(g_sql,
          "CREATE TABLE article("
          "  article_id INTEGER,"
          "  author     TEXT,"
          "  title      TEXT,"
          "  view_count INTEGER,"
          "  content    %s,"
          "  PRIMARY KEY(article_id)"
          ")",
          blob_type);
  sqldbal_test_exec_plain("DROP TABLE IF EXISTS article");
  sqldbal_test_exec_plain(g_sql);

  sprintf(g_sql,
          "CREATE TABLE test_null("
          "  test_null_id INTEGER,"
          "  test         TEXT,"
          "  PRIMARY KEY(test_null_id)"
          ")");
  sqldbal_test_exec_plain("DROP TABLE IF EXISTS test_null");
  sqldbal_test_exec_plain(g_sql);

  sprintf(g_sql,
          "CREATE TABLE test_float("
          "  test_float_id INTEGER,"
          "  test          FLOAT,"
          "  PRIMARY KEY(test_float_id)"
          ")");
  sqldbal_test_exec_plain("DROP TABLE IF EXISTS test_float");
  sqldbal_test_exec_plain(g_sql);

  sprintf(g_sql,
          "CREATE TABLE simple("
          "  simple_id INTEGER,"
          "  test      TEXT,"
          "  PRIMARY KEY(simple_id)"
          ")");
  sqldbal_test_exec_plain("DROP TABLE IF EXISTS simple");
  sqldbal_test_exec_plain(g_sql);

  sprintf(g_sql,
          "CREATE TABLE simple2("
          "  simple_id INTEGER,"
          "  test      TEXT,"
          "  PRIMARY KEY(simple_id)"
          ")");
  sqldbal_test_exec_plain("DROP TABLE IF EXISTS simple2");
  sqldbal_test_exec_plain(g_sql);

  sprintf(g_sql,
          "CREATE TABLE test_sequence("
          "  id   %s %s,"
          "  test TEXT,"
          "  PRIMARY KEY(id)"
          ")",
          seq_type,
          seq_attribute);
  sqldbal_test_exec_plain("DROP TABLE IF EXISTS test_sequence");
  sqldbal_test_exec_plain(g_sql);
}

/**
 * Generate placeholder strings stored in @ref g_q.
 */
static void
sqldbal_test_stmt_generate_placeholders(void){
  size_t i;

  for(i = 0; i < MAX_QUERY_PARAMS; i++){
    if(sqldbal_driver_type(g_db) == SQLDBAL_DRIVER_POSTGRESQL){
      sprintf(g_q[i], "$%zu", i + 1);
    }
    else{
      strcpy(g_q[i], "?");
    }
  }
}

/**
 * Generate SQL to test scenarios where we need to use a prepared
 * statement to insert values.
 */
static void
sqldbal_test_stmt_generate_insert(void){
  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql,
          "INSERT INTO article(article_id, author, title, view_count, content)"
          "             VALUES(        %s,     %s,    %s,         %s,      %s)",
          g_q[0], g_q[1], g_q[2], g_q[3], g_q[4]);
}

/**
 * Generate SQL to test scenarios where we need a prepared statement to update
 * values.
 */
static void
sqldbal_test_stmt_generate_update(void){
  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql,
          "UPDATE simple SET test = %s",
          g_q[0]);
}

/**
 * Generate SQL to test scenarios where we need to use a prepared
 * select statement.
 */
static void
sqldbal_test_stmt_generate_select(void){
  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql,
          "SELECT * FROM simple WHERE test = %s",
          g_q[0]);
}

/**
 * Generate SQL to select all fields in the article table.
 */
static void
sqldbal_test_stmt_generate_select_article(void){
  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql, "SELECT * FROM article");
}

/**
 * Generate SQL to test scenarios where we need to insert into the
 * test_sequence table.
 */
static void
sqldbal_test_stmt_generate_insert_id(void){
  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql,
          "INSERT INTO test_sequence(test)"
          "                   VALUES(%s)",
          g_q[0]);
}

/**
 * Test harness for @ref sqldbal_stmt_execute.
 *
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_execute(enum sqldbal_status_code expect_status){
  g_rc = sqldbal_stmt_execute(g_stmt);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_fetch.
 *
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 * @param[in] expect_fetch  Expected fetch result.
 */
static void
sqldbal_test_stmt_fetch(enum sqldbal_status_code expect_status,
                        enum sqldbal_fetch_result expect_fetch){
  enum sqldbal_fetch_result fetch;

  fetch = sqldbal_stmt_fetch(g_stmt);
  assert(fetch == expect_fetch);

  g_rc = sqldbal_status_code_get(g_db);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_column_type.
 *
 * @param[in] col_idx       Column index.
 * @param[in] expect_type   Expected column type to get returned.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_column_type(size_t col_idx,
                              enum sqldbal_column_type expect_type,
                              enum sqldbal_status_code expect_status){
  enum sqldbal_column_type type;

  type = sqldbal_stmt_column_type(g_stmt, col_idx);
  assert(type == expect_type);

  g_rc = sqldbal_status_code_get(g_db);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_close.
 *
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_close(enum sqldbal_status_code expect_status){
  g_rc = sqldbal_stmt_close(g_stmt);
  assert(g_rc == expect_status);
}

/**
 * Insert test article rows from @ref g_article_list.
 */
static void
sqldbal_functional_test_prepared_insert(void){
  size_t i;
  struct sqldbal_test_article *article;

  sqldbal_test_stmt_generate_insert();

  sqldbal_test_stmt_prepare_sql();

  for(i = 0; i < g_num_articles; i++){
    article = &g_article_list[i];

    g_rc = sqldbal_stmt_bind_int64(g_stmt, 0, article->article_id);
    assert(g_rc == SQLDBAL_STATUS_OK);

    g_rc = sqldbal_stmt_bind_text(g_stmt, 1, article->author, SIZE_MAX);
    assert(g_rc == SQLDBAL_STATUS_OK);

    g_rc = sqldbal_stmt_bind_text(g_stmt, 2, article->title, SIZE_MAX);
    assert(g_rc == SQLDBAL_STATUS_OK);

    g_rc = sqldbal_stmt_bind_int64(g_stmt, 3, (int64_t)article->view_count);
    assert(g_rc == SQLDBAL_STATUS_OK);

    g_rc = sqldbal_stmt_bind_blob(g_stmt,
                                  4,
                                  article->content,
                                  strlen(article->content));
    assert(g_rc == SQLDBAL_STATUS_OK);

    g_rc = sqldbal_stmt_execute(g_stmt);
    assert(g_rc == SQLDBAL_STATUS_OK);
  }

  sqldbal_test_stmt_close_sql();
}

/**
 * Used by the application callback to keep track of current row index.
 *
 * See @ref sqldbal_test_exec_callback.
 */
struct sqldbal_test_exec_sel_data{
  /**
   * The current row in the result set.
   */
  size_t row_i;
};

/**
 * Verify that all article rows exist after we inserted into the database.
 *
 * The library calls this function for every row in the result set.
 *
 * @param[in] user_data       See @ref sqldbal_test_exec_sel_data.
 * @param[in] num_cols        Number of columns in @p col_result_list
 *                            and @p col_length_list.
 * @param[in] col_result_list Column values in the current row.
 * @param[in] col_length_list Length of the corresponding column
 *                            in @p col_result_list.
 * @retval 0 This function always succeeds or throws an assertion.
 */
static int
sqldbal_test_exec_callback(void *user_data,
                           size_t num_cols,
                           char **col_result_list,
                           size_t *col_length_list){
  struct sqldbal_test_exec_sel_data *sel_data;
  struct sqldbal_test_article *article;
  char article_id_str[10];
  char view_count_str[10];

  sel_data = user_data;

  assert(num_cols == 5);

  article = &g_article_list[sel_data->row_i];

  sprintf(article_id_str, "%d", article->article_id);
  assert(strcmp(article_id_str, col_result_list[0]) == 0);

  assert(strcmp(article->author, col_result_list[1]) == 0);

  assert(strcmp(article->title, col_result_list[2]) == 0);

  sprintf(view_count_str, "%zu", article->view_count);
  assert(strcmp(view_count_str, col_result_list[3]) == 0);

  assert(strlen(article->content) == col_length_list[4]);
  assert(memcmp(article->content,
                col_result_list[4],
                col_length_list[4]) == 0);

  sel_data->row_i += 1;
  return 0;
}

/**
 * Test the @ref sqldbal_exec function and compare all rows that get returned.
 */
static void
sqldbal_functional_test_exec_select(void){
  struct sqldbal_test_exec_sel_data user_data;
  const char *const sql =
  "SELECT article_id AS article_id,"
  "       author     AS author    ,"
  "       title      AS title     ,"
  "       view_count AS view_count,"
  "       content    AS content    "
  "  FROM article";

  user_data.row_i = 0;
  g_rc = sqldbal_exec(g_db, sql, sqldbal_test_exec_callback, &user_data);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Test @ref sqldbal_exec without a callback function.
 */
static void
sqldbal_functional_test_exec_select_no_callback(void){
  const char *const sql =
  "SELECT article_id AS article_id,"
  "       author     AS author    ,"
  "       title      AS title     ,"
  "       view_count AS view_count,"
  "       content    AS content    "
  "  FROM article";

  g_rc = sqldbal_exec(g_db, sql, NULL, NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Test @ref sqldbal_exec with an INSERT statement.
 */
static void
sqldbal_functional_test_exec_insert(void){
  const char *const sql_ins =
  "INSERT INTO simple2(simple_id, test)"
  "  VALUES(1, \'test\')";

  g_rc = sqldbal_exec(g_db,
                      sql_ins,
                      NULL,
                      NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Read back the inserted rows from
 * @ref sqldbal_functional_test_prepared_insert.
 */
static void
sqldbal_functional_test_prepared_select(void){
  enum sqldbal_column_type expect_type;
  enum sqldbal_driver driver;
  const char *title;
  size_t titlesz;
  int64_t view_count;
  const void *content;
  size_t content_sz;

  driver = sqldbal_driver_type(g_db);

  sprintf(g_sql,
          "SELECT a.title      AS title,"
          "       a.view_count AS view_count,"
          "       a.content    AS content"
          "  FROM article      AS a"
          "  WHERE article_id = %s",
          g_q[0]);

  sqldbal_test_stmt_prepare_sql();

  g_rc = sqldbal_stmt_bind_int64(g_stmt, 0, 1);
  assert(g_rc == SQLDBAL_STATUS_OK);

  g_rc = sqldbal_stmt_execute(g_stmt);
  assert(g_rc == SQLDBAL_STATUS_OK);

  g_fetch_rc = sqldbal_stmt_fetch(g_stmt);
  assert(g_fetch_rc == SQLDBAL_FETCH_ROW);

  /*
   * The PostgreSQL and MariaDB drivers return BLOB data type.
   */
  if(driver == SQLDBAL_DRIVER_MARIADB || driver == SQLDBAL_DRIVER_POSTGRESQL){
    expect_type = SQLDBAL_TYPE_BLOB;
  }
  else{
    expect_type = SQLDBAL_TYPE_TEXT;
  }
  sqldbal_test_stmt_column_type(0, expect_type, SQLDBAL_STATUS_OK);

  g_rc = sqldbal_stmt_column_text(g_stmt, 0, &title, &titlesz);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(titlesz == strlen(g_article_list[0].title));
  assert(strcmp(title, g_article_list[0].title) == 0);

  /* Get the text column without the strlen. */
  g_rc = sqldbal_stmt_column_text(g_stmt, 0, &title, NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(strcmp(title, g_article_list[0].title) == 0);

  /*
   * The PostgreSQL and MariaDB drivers return BLOB data type.
   */
  if(driver == SQLDBAL_DRIVER_MARIADB || driver == SQLDBAL_DRIVER_POSTGRESQL){
    expect_type = SQLDBAL_TYPE_BLOB;
  }
  else{
    expect_type = SQLDBAL_TYPE_INT;
  }
  sqldbal_test_stmt_column_type(1, expect_type, SQLDBAL_STATUS_OK);

  g_rc = sqldbal_stmt_column_int64(g_stmt, 1, &view_count);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(view_count > 0);
  assert((size_t)view_count == g_article_list[0].view_count);

  expect_type = SQLDBAL_TYPE_BLOB;
  sqldbal_test_stmt_column_type(2, expect_type, SQLDBAL_STATUS_OK);

  g_rc = sqldbal_stmt_column_blob(g_stmt, 2, &content, &content_sz);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(content_sz == strlen(g_article_list[0].content));
  assert(memcmp(content, g_article_list[0].content, content_sz) == 0);

  g_fetch_rc = sqldbal_stmt_fetch(g_stmt);
  assert(g_fetch_rc == SQLDBAL_FETCH_DONE);

  sqldbal_test_stmt_close_sql();
}

/**
 * Used by @ref sqldbal_functional_test_transaction to keep track if which
 * insert entries to verify.
 */
struct sqldbal_test_transaction_sel{
  /**
   * Flag indicating that entry should exist if set to 1.
   */
  char check_result[2][256];

  /**
   * Number of rows that have been processed.
   */
  size_t row_i;
};

/**
 * Verify the results in @ref sqldbal_functional_test_transaction.
 *
 * @param[in] user_data       See @ref sqldbal_test_transaction_sel.
 * @param[in] num_cols        Number of columns in @p col_result_list
 *                            and @p col_length_list.
 * @param[in] col_result_list Column values in the current row.
 * @param[in] col_length_list Length of the corresponding column
 *                            in @p col_result_list.
 * @retval 0 This function always succeeds or throws an assertion.
 */
static int
sqldbal_test_transaction_exec_fp(void *user_data,
                                 size_t num_cols,
                                 char **col_result_list,
                                 size_t *col_length_list){
  struct sqldbal_test_transaction_sel *transaction_sel;
  const char *expect_str;

  (void)col_length_list;
  transaction_sel = user_data;
  assert(num_cols == 2);
  expect_str = transaction_sel->check_result[transaction_sel->row_i];
  assert(strcmp(col_result_list[0], expect_str) == 0);
  transaction_sel->row_i += 1;
  return 0;
}

/**
 * Test the following interfaces.
 *   - @ref sqldbal_begin_transaction
 *   - @ref sqldbal_commit
 *   - @ref sqldbal_rollback
 *
 * This will first insert an entry and then rollback. It will verify the
 * entry before and after the rollback. Then it will insert two different
 * entries and commit them.
 */
static void
sqldbal_functional_test_transaction(void){
  const char *const sql_ins_1 =
  "INSERT INTO simple(simple_id, test)"
  "  VALUES(1, \'1\')";
  const char *const sql_ins_2 =
  "INSERT INTO simple(simple_id, test)"
  "  VALUES(2, \'2\')";
  const char *const sql_ins_3 =
  "INSERT INTO simple(simple_id, test)"
  "  VALUES(3, \'3\')";
  const char *const sql_sel =
  "SELECT simple_id, test"
  "  FROM simple";
  struct sqldbal_test_transaction_sel transaction_sel;

  g_rc = sqldbal_begin_transaction(g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);

  g_rc = sqldbal_exec(g_db,
                      sql_ins_1,
                      NULL,
                      NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);

  /* Verify that the entry exists before the rollback. */
  memset(&transaction_sel, 0, sizeof(transaction_sel));
  strcpy(transaction_sel.check_result[0], "1");
  g_rc = sqldbal_exec(g_db,
                      sql_sel,
                      sqldbal_test_transaction_exec_fp,
                      &transaction_sel);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(transaction_sel.row_i == 1);

  g_rc = sqldbal_rollback(g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);

  /* Verify that the rollback removed the first entry. */
  memset(&transaction_sel, 0, sizeof(transaction_sel));
  g_rc = sqldbal_exec(g_db,
                      sql_sel,
                      sqldbal_test_transaction_exec_fp,
                      &transaction_sel);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(transaction_sel.row_i == 0);

  g_rc = sqldbal_begin_transaction(g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);

  g_rc = sqldbal_exec(g_db,
                      sql_ins_2,
                      NULL,
                      NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);

  g_rc = sqldbal_exec(g_db,
                      sql_ins_3,
                      NULL,
                      NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);

  /* Verify that both entries exist before the commit. */
  memset(&transaction_sel, 0, sizeof(transaction_sel));
  strcpy(transaction_sel.check_result[0], "2");
  strcpy(transaction_sel.check_result[1], "3");
  g_rc = sqldbal_exec(g_db,
                      sql_sel,
                      sqldbal_test_transaction_exec_fp,
                      &transaction_sel);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(transaction_sel.row_i == 2);

  g_rc = sqldbal_commit(g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);

  /* Verify that both entries exist after the commit. */
  memset(&transaction_sel, 0, sizeof(transaction_sel));
  strcpy(transaction_sel.check_result[0], "2");
  strcpy(transaction_sel.check_result[1], "3");
  g_rc = sqldbal_exec(g_db,
                      sql_sel,
                      sqldbal_test_transaction_exec_fp,
                      &transaction_sel);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(transaction_sel.row_i == 2);
}

/**
 * Test the following interfaces.
 *   - @ref sqldbal_db_handle
 *   - @ref sqldbal_stmt_handle
 */
static void
sqldbal_functional_test_handles(void){
  enum sqldbal_driver driver;
  void *db_handle;
  MYSQL *mysql_db;
  PGconn *pq_db;
  sqlite3 *sqlite_db;
  void *stmt_handle;
  MYSQL_STMT *mysql_stmt;
  const char *pq_stmt;
  sqlite3_stmt *sqlite_stmt;
  unsigned int mysql_rc;
  ConnStatusType pq_conn_status;
  PGresult *pg_result;
  ExecStatusType pq_status;
  int sqlite_rc;

  driver = sqldbal_driver_type(g_db);
  db_handle = sqldbal_db_handle(g_db);
  g_rc = sqldbal_stmt_prepare(g_db,
                              g_sql_valid_sel,
                              SIZE_MAX,
                              &g_stmt);
  assert(g_rc == SQLDBAL_STATUS_OK);
  stmt_handle = sqldbal_stmt_handle(g_stmt);

  if(driver == SQLDBAL_DRIVER_MARIADB){
    mysql_db = db_handle;
    mysql_stmt = stmt_handle;

    mysql_rc = mysql_errno(mysql_db);
    assert(mysql_rc == 0);

    mysql_rc = mysql_stmt_errno(mysql_stmt);
    assert(mysql_rc == 0);
  }
  else if(driver == SQLDBAL_DRIVER_POSTGRESQL){
    pq_db = db_handle;
    pq_stmt = stmt_handle;

    pq_conn_status = PQstatus(pq_db);
    assert(pq_conn_status == CONNECTION_OK);

    pg_result = PQdescribePrepared(pq_db, pq_stmt);
    pq_status = PQresultStatus(pg_result);
    assert(pq_status == PGRES_COMMAND_OK);
    PQclear(pg_result);
  }
  else if(driver == SQLDBAL_DRIVER_SQLITE){
    sqlite_db = db_handle;
    sqlite_stmt = stmt_handle;

    sqlite_rc = sqlite3_errcode(sqlite_db);
    assert(sqlite_rc == SQLITE_OK);

    sqlite_rc = sqlite3_stmt_readonly(sqlite_stmt);
    assert(sqlite_rc != 0);
  }
  else{
    assert(1);
  }

  sqldbal_test_stmt_close_sql();
}

/**
 * Test @ref sqldbal_last_insert_id.
 */
static void
sqldbal_functional_test_last_insert_id(void){
  uint64_t ins_id;
  const char *const ins_id_name = "test_sequence_id_seq";

  sqldbal_test_stmt_generate_insert_id();
  sqldbal_test_stmt_prepare_sql();
  g_rc = sqldbal_stmt_bind_text(g_stmt, 0, "James", SIZE_MAX);
  assert(g_rc == SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  g_rc = sqldbal_last_insert_id(g_db, ins_id_name, &ins_id);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(ins_id == 1);
  sqldbal_test_stmt_close_sql();
}

/**
 * Prepare and execute multiple statements.
 */
static void
sqldbal_functional_test_multiple_statements(void){
  struct sqldbal_stmt *stmt_list[NUM_STATEMENTS_MULTI_TEST];
  size_t i;

  for(i = 0; i < NUM_STATEMENTS_MULTI_TEST; i++){
    g_rc = sqldbal_stmt_prepare(g_db,
                                g_sql_valid_sel,
                                SIZE_MAX,
                                &stmt_list[i]);
    assert(g_rc == SQLDBAL_STATUS_OK);
  }

  for(i = 0; i < NUM_STATEMENTS_MULTI_TEST; i++){
    g_rc = sqldbal_stmt_close(stmt_list[i]);
    assert(g_rc == SQLDBAL_STATUS_OK);
  }
}

/**
 * Prepare a SQL statement with an explicit SQL length.
 */
static void
sqldbal_functional_test_stmt_explicit_sql_len(void){
  g_rc = sqldbal_stmt_prepare(g_db,
                              g_sql_valid_sel,
                              strlen(g_sql_valid_sel),
                              &g_stmt);
  assert(g_rc == SQLDBAL_STATUS_OK);

  sqldbal_test_stmt_close(SQLDBAL_STATUS_OK);
}

/**
 * This callback verifies that the column data is NULL.
 *
 * @param[in] user_data       Ignored.
 * @param[in] num_cols        Number of columns in result set.
 * @param[in] col_result_list Ignored.
 * @param[in] col_length_list Ignored.
 * @return Always 0 success if no assertion.
 */
static int
sqldbal_test_exec_is_null_fp(void *user_data,
                             size_t num_cols,
                             char **col_result_list,
                             size_t *col_length_list){
  (void)user_data;

  assert(num_cols == 1);
  assert(col_result_list[0] == NULL);
  assert(col_length_list[0] == 0);
  return 0;
}

/**
 * Test writing and reading NULL columns.
 */
static void
sqldbal_functional_test_null(void){
  enum sqldbal_column_type col_type;
  const char *text;
  int64_t i64;
  const void *blob;
  size_t textsz;
  size_t blobsz;

  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql,
          "INSERT INTO test_null(test_null_id, test)"
          "               VALUES(1           ,  %s)",
          g_q[0]);
  sqldbal_test_stmt_prepare_sql();
  g_rc = sqldbal_stmt_bind_null(g_stmt, 0);
  assert(g_rc == SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_close_sql();

  sprintf(g_sql, "SELECT test FROM test_null");
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);
  col_type = sqldbal_stmt_column_type(g_stmt, 0);
  assert(col_type == SQLDBAL_TYPE_NULL);

  g_rc = sqldbal_stmt_column_text(g_stmt, 0, &text, &textsz);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(text == NULL);
  assert(textsz == 0);

  g_rc = sqldbal_stmt_column_int64(g_stmt, 0, &i64);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(i64 == 0);

  g_rc = sqldbal_stmt_column_blob(g_stmt, 0, &blob, &blobsz);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(blob == NULL);
  assert(blobsz == 0);
  sqldbal_test_stmt_close_sql();

  sprintf(g_sql, "SELECT test FROM test_null");
  g_rc = sqldbal_exec(g_db, g_sql, sqldbal_test_exec_is_null_fp, NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Test FLOAT data type.
 */
static void
sqldbal_functional_test_float(void){
  enum sqldbal_driver driver;
  enum sqldbal_column_type type;

  sqldbal_test_stmt_generate_placeholders();
  driver = sqldbal_driver_type(g_db);

  sprintf(g_sql, "INSERT INTO test_float(test_float_id, test) VALUES(1, 1.0)");
  g_rc = sqldbal_exec(g_db, g_sql, NULL, NULL);
  assert(g_rc == SQLDBAL_STATUS_OK);

  sprintf(g_sql, "SELECT test FROM test_float");
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);

  type = sqldbal_stmt_column_type(g_stmt, 0);

  if(driver == SQLDBAL_DRIVER_SQLITE){
    assert(type == SQLDBAL_TYPE_OTHER);
  }
  else{
    assert(type == SQLDBAL_TYPE_BLOB);
  }

  sqldbal_test_stmt_close_sql();
}

/**
 * Test reading a blank string.
 */
static void
sqldbal_functional_test_blank_string(void){
  const char *text;
  size_t textsz;

  sqldbal_test_stmt_generate_placeholders();
  sprintf(g_sql, "SELECT author FROM article WHERE article_id = 4");
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);
  g_rc = sqldbal_stmt_column_text(g_stmt, 0, &text, &textsz);
  assert(strcmp(text, "") == 0);
  assert(textsz == 0);
  sqldbal_test_stmt_close_sql();
}

/**
 * Run various tests for a single database driver.
 */
static void
sqldbal_functional_test_db(void){
  enum sqldbal_driver driver;

  driver = sqldbal_driver_type(g_db);

  if(driver != SQLDBAL_DRIVER_SQLITE){
    sqldbal_test_exec_plain("DROP DATABASE IF EXISTS test_db");
    sqldbal_test_exec_plain("CREATE DATABASE test_db");
  }

  sqldbal_functional_test_create_table();
  sqldbal_functional_test_prepared_insert();
  sqldbal_functional_test_exec_select();
  sqldbal_functional_test_exec_select_no_callback();
  sqldbal_functional_test_exec_insert();
  sqldbal_functional_test_prepared_select();
  sqldbal_functional_test_transaction();
  sqldbal_functional_test_handles();
  sqldbal_functional_test_last_insert_id();
  sqldbal_functional_test_multiple_statements();
  sqldbal_functional_test_stmt_explicit_sql_len();
  sqldbal_functional_test_null();
  sqldbal_functional_test_float();
  sqldbal_functional_test_blank_string();

  if(driver != SQLDBAL_DRIVER_SQLITE){
    sqldbal_test_exec_plain("DROP DATABASE test_db");
  }
}

/**
 * Run through the same tests in @ref sqldbal_functional_test_db for each
 * database driver.
 */
static void
sqldbal_functional_test_db_list(void){
  size_t i;
  struct sqldbal_test_db_config *config;

  for(i = 0; i < g_db_num; i++){
    config = &g_db_config_list[i];
    g_rc = sqldbal_open(config->driver,
                        config->location,
                        config->port,
                        config->username,
                        config->password,
                        config->database,
                        config->flags,
                        NULL,
                        0,
                        &g_db);
    assert(g_rc == SQLDBAL_STATUS_OK);

    sqldbal_functional_test_db();

    sqldbal_test_db_close();
  }
}

/**
 * Test harness for @ref sqldbal_open which allows the caller to
 * conveniently inject return values prior to calling this and then to
 * have this function check the result.
 *
 * @param[in] driver        See @ref sqldbal_open.
 * @param[in] location      See @ref sqldbal_open.
 * @param[in] port          See @ref sqldbal_open.
 * @param[in] username      See @ref sqldbal_open.
 * @param[in] password      See @ref sqldbal_open.
 * @param[in] database      See @ref sqldbal_open.
 * @param[in] flags         See @ref sqldbal_open.
 * @param[in] num_options   See @ref sqldbal_open.
 * @param[in] option_list   See @ref sqldbal_open.
 * @param[in] expect_status Expected status return code of @ref sqldbal_open.
 */
static void
sqldbal_test_open(enum sqldbal_driver driver,
                  const char *const location,
                  const char *const port,
                  const char *const username,
                  const char *const password,
                  const char *const database,
                  enum sqldbal_flag flags,
                  size_t num_options,
                  const struct sqldbal_driver_option *const option_list,
                  enum sqldbal_status_code expect_status){
  g_rc = sqldbal_open(driver,
                      location,
                      port,
                      username,
                      password,
                      database,
                      flags,
                      option_list,
                      num_options,
                      &g_db);
  assert(g_rc == expect_status);
  g_rc = sqldbal_close(g_db);
  assert(g_rc == expect_status);
}

/**
 * Test connecting using TLS encryption.
 */
static void
sqldbal_functional_test_encryption(void){
  struct sqldbal_driver_option mariadb_option_list[] = {
    {"TLS_KEY"        , "/var/lib/mysql/client-key.pem" },
    {"TLS_CERT"       , "/var/lib/mysql/client-cert.pem"},
    {"TLS_CA"         , "/var/lib/mysql/ca.pem"         },
    {"TLS_CAPATH"     , "/var/lib/mysql/capath"         },
    {"TLS_CIPHER"     , "DHE-RSA-AES256-SHA"            }
  };
  const size_t mariadb_num_option_list = sizeof(mariadb_option_list) /
                                         sizeof(mariadb_option_list[0]);
  struct sqldbal_driver_option pq_option_list[] = {
    {"TLS_MODE"       , "verify-ca"             },
    {"TLS_CERT"       , "/var/lib/pq/client.crt"},
    {"TLS_KEY"        , "/var/lib/pq/client.key"},
    {"TLS_CA"         , "/var/lib/pq/root.crt"  },
  };
  struct sqldbal_driver_option pq_invalid_cert_option_list[] = {
    {"TLS_MODE"       , "verify-ca"              },
    {"TLS_CERT"       , "/var/lib/pq/noexist.crt"},
    {"TLS_KEY"        , "/var/lib/pq/noexist.key"},
    {"TLS_CA"         , "/var/lib/pq/noexist.crt"},
  };
  const size_t pq_num_option_list = sizeof(pq_option_list) /
                                    sizeof(pq_option_list[0]);

  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    g_db_config_list[0].location,
                    g_db_config_list[0].port,
                    g_db_config_list[0].username,
                    g_db_config_list[0].password,
                    g_db_config_list[0].database,
                    g_db_config_list[0].flags,
                    mariadb_num_option_list,
                    mariadb_option_list,
                    SQLDBAL_STATUS_OK);

  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    g_db_config_list[1].location,
                    g_db_config_list[1].port,
                    g_db_config_list[1].username,
                    g_db_config_list[1].password,
                    g_db_config_list[1].database,
                    g_db_config_list[1].flags,
                    pq_num_option_list,
                    pq_option_list,
                    SQLDBAL_STATUS_OK);

  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    g_db_config_list[1].location,
                    g_db_config_list[1].port,
                    g_db_config_list[1].username,
                    g_db_config_list[1].password,
                    g_db_config_list[1].database,
                    g_db_config_list[1].flags,
                    pq_num_option_list,
                    pq_invalid_cert_option_list,
                    SQLDBAL_STATUS_OPEN);
}

/**
 * Test timeout option.
 */
static void
sqldbal_functional_test_timeout(void){
  struct sqldbal_driver_option option_list[] = {
    {"CONNECT_TIMEOUT", "100"}
  };
  const size_t num_option_list = sizeof(option_list) / sizeof(option_list[0]);

  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    g_db_config_list[0].location,
                    g_db_config_list[0].port,
                    g_db_config_list[0].username,
                    g_db_config_list[0].password,
                    g_db_config_list[0].database,
                    g_db_config_list[0].flags,
                    num_option_list,
                    option_list,
                    SQLDBAL_STATUS_OK);

  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    g_db_config_list[1].location,
                    g_db_config_list[1].port,
                    g_db_config_list[1].username,
                    g_db_config_list[1].password,
                    g_db_config_list[1].database,
                    g_db_config_list[1].flags,
                    num_option_list,
                    option_list,
                    SQLDBAL_STATUS_OK);
}

/**
 * Test enabling debug and trace mode.
 */
static void
sqldbal_functional_test_debug(void){
  size_t i;
  struct sqldbal_test_db_config *config;

  for(i = 0; i < g_db_num; i++){
    config = &g_db_config_list[i];
    g_rc = sqldbal_open(config->driver,
                        config->location,
                        config->port,
                        config->username,
                        config->password,
                        config->database,
                        config->flags | SQLDBAL_FLAG_DEBUG,
                        NULL,
                        0,
                        &g_db);
    assert(g_rc == SQLDBAL_STATUS_OK);

    sqldbal_functional_test_exec_select();

    sqldbal_test_db_close();
  }
}

/**
 * Test harness for @ref sqldbal_stmt_bind_text.
 *
 * @param[in] col_idx       Placeholder index referenced in prepared statement.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_bind_text(size_t col_idx,
                            enum sqldbal_status_code expect_status){
  const char *const text = "test";

  g_rc = sqldbal_stmt_bind_text(g_stmt, col_idx, text, strlen(text));
  assert(g_rc == expect_status);
}

/**
 * Test different flags/options when opening SQLite databases.
 */
static void
sqldbal_functional_test_sqlite_open_options(void){
  struct sqldbal_driver_option vfs_option_list[] = {
    {"VFS", NULL}
  };
  const size_t num_option_list = sizeof(vfs_option_list) /
                                 sizeof(vfs_option_list[0]);

  /* Test vfs parameter - default VFS (NULL). */
  sqldbal_test_open(SQLDBAL_DRIVER_SQLITE,
                    g_db_config_list[2].location,
                    g_db_config_list[2].port,
                    g_db_config_list[2].username,
                    g_db_config_list[2].password,
                    g_db_config_list[2].database,
                    g_db_config_list[2].flags,
                    num_option_list,
                    vfs_option_list,
                    SQLDBAL_STATUS_OK);

  /* SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE */
  sqldbal_test_open(SQLDBAL_DRIVER_SQLITE,
                    g_db_config_list[2].location,
                    g_db_config_list[2].port,
                    g_db_config_list[2].username,
                    g_db_config_list[2].password,
                    g_db_config_list[2].database,
                    g_db_config_list[2].flags |
                      SQLDBAL_FLAG_SQLITE_OPEN_CREATE |
                      SQLDBAL_FLAG_SQLITE_OPEN_READWRITE,
                    0,
                    NULL,
                    SQLDBAL_STATUS_OK);

  /* Read-only database. */
  g_rc = sqldbal_open(SQLDBAL_DRIVER_SQLITE,
                      g_db_config_list[2].location,
                      g_db_config_list[2].port,
                      g_db_config_list[2].username,
                      g_db_config_list[2].password,
                      g_db_config_list[2].database,
                      g_db_config_list[2].flags |
                        SQLDBAL_FLAG_SQLITE_OPEN_READONLY,
                      NULL,
                      0,
                      &g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_generate_insert_id();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_rc = sqldbal_stmt_close(g_stmt);
  assert(g_rc == SQLDBAL_STATUS_CLOSE);
  sqldbal_status_code_clear(g_db);
  sqldbal_test_db_close();
}

/**
 * Test harness for @ref sqldbal_errstr.
 *
 * @param[in] status_code   Set this status code prior to calling
 *                          @ref sqldbal_errstr.
 * @param[in] expect_errstr Expected error string description returned from
 *                          @ref sqldbal_errstr.
 */
static void
sqldbal_test_errstr(enum sqldbal_status_code status_code,
                    const char *const expect_errstr){
  const char *result_errstr;

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);

  g_rc = sqldbal_status_code_set(g_db, status_code);
  assert(g_rc == status_code);

  g_rc = sqldbal_errstr(g_db, &result_errstr);
  assert(g_rc == status_code);
  if(expect_errstr){
    assert(strcmp(result_errstr, expect_errstr) == 0);
  }
  else{
    assert(result_errstr == expect_errstr);
  }

  g_rc = sqldbal_close(g_db);
  assert(g_rc == status_code);
}

/**
 * Test scenarios for @ref sqldbal_errstr.
 */
static void
sqldbal_functional_test_errstr(void){
  const char *const test_str = "test error string";
  const char *errstr;

  sqldbal_test_errstr(SQLDBAL_STATUS_OK,
                      "Success");
  sqldbal_test_errstr(SQLDBAL_STATUS_OPEN,
                      "Failed to open database context");
  sqldbal_test_errstr(SQLDBAL_STATUS__LAST,
                      "Unknown error");

  /* Custom error string. */
  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_errstr_set(g_db, test_str);
  g_rc = sqldbal_errstr(g_db, &errstr);
  assert(g_rc == SQLDBAL_STATUS_OK);
  assert(strcmp(errstr, test_str) == 0);
  sqldbal_test_db_close();

  /* sqldbal_errstr_set - sqldbal_strdup */
  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_errstr_set(g_db, test_str);
  g_rc = sqldbal_status_code_get(g_db);
  assert(g_rc == SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_test_db_close();
}

/**
 * Run through different failure scenarios when calling @ref sqldbal_open.
 */
static void
sqldbal_test_all_error_open(void){
  size_t i;

  struct sqldbal_driver_option invalid_option_list[] = {
    {"key", "value"}
  };

  struct sqldbal_driver_option invalid_timeout_option_list[] = {
    {"CONNECT_TIMEOUT", "abc"}
  };

  struct sqldbal_driver_option valid_option_list[] = {
    {"CONNECT_TIMEOUT", "100"}
  };

  /* Memory allocation failed for @ref g_db. */
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_SQLITE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* Driver does not exist. */
  sqldbal_test_open(SQLDBAL_DRIVER_INVALID,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_DRIVER_NOSUPPORT);

  /* sqldbal_mariadb_open - SQLDBAL_DRIVER_MARIADB */

  /* sqldbal_strtoui */
  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    NULL,
                    "---",
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_PARAM);

  /* mysql_init */
  g_sqldbal_err_mysql_init_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_mysql_init_ctr = -1;

  /* invalid CONNECT_TIMEOUT value */
  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    1,
                    invalid_timeout_option_list,
                    SQLDBAL_STATUS_PARAM);

  /* invalid option parameter */
  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    1,
                    invalid_option_list,
                    SQLDBAL_STATUS_PARAM);

  /* mysql_options */
  g_sqldbal_err_mysql_options_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    1,
                    valid_option_list,
                    SQLDBAL_STATUS_PARAM);
  g_sqldbal_err_mysql_options_ctr = -1;

  /* mysql_real_connect */
  sqldbal_test_open(SQLDBAL_DRIVER_MARIADB,
                    "invalid",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_OPEN);

  /* sqldbal_pq_open - SQLDBAL_DRIVER_POSTGRESQL */

  /* malloc */
  g_sqldbal_err_malloc_ctr = 1;
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* sqldbal_pq_conninfo - invalid option */
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    1,
                    invalid_option_list,
                    SQLDBAL_STATUS_PARAM);

  for(i = 0; i < 33; i++){
    /* sqldbal_pq_conninfo - si_add_size_t - i */
    g_sqldbal_err_si_add_size_t_ctr = (int)i;
    sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      0,
                      1,
                      valid_option_list,
                      SQLDBAL_STATUS_NOMEM);
    g_sqldbal_err_si_add_size_t_ctr = -1;
  }

  /* sqldbal_pq_conninfo - malloc */
  g_sqldbal_err_malloc_ctr = 2;
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    1,
                    valid_option_list,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* PQconnectdb */
  g_sqldbal_err_PQconnectdb_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    "invalid",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_OPEN);
  g_sqldbal_err_PQconnectdb_ctr = -1;

  /* PQstatus */
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    "invalid",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_OPEN);

  /* sqldbal_pq_query_oid_list - PQexec */
  g_sqldbal_err_PQexec_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    g_db_config_list[1].location,
                    g_db_config_list[1].port,
                    g_db_config_list[1].username,
                    g_db_config_list[1].password,
                    g_db_config_list[1].database,
                    g_db_config_list[1].flags,
                    0,
                    NULL,
                    SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_PQexec_ctr = -1;

  /* sqldbal_pq_query_oid_list - reallocarray */
  g_sqldbal_err_realloc_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    g_db_config_list[1].location,
                    g_db_config_list[1].port,
                    g_db_config_list[1].username,
                    g_db_config_list[1].password,
                    g_db_config_list[1].database,
                    g_db_config_list[1].flags,
                    0,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  /* sqldbal_pq_query_oid_list - sqldbal_strtoui */
  g_sqldbal_err_strtoul_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_POSTGRESQL,
                    g_db_config_list[1].location,
                    g_db_config_list[1].port,
                    g_db_config_list[1].username,
                    g_db_config_list[1].password,
                    g_db_config_list[1].database,
                    g_db_config_list[1].flags,
                    0,
                    NULL,
                    SQLDBAL_STATUS_COLUMN_COERCE);
  g_sqldbal_err_strtoul_ctr = -1;

  /* sqldbal_sqlite_open - SQLDBAL_DRIVER_SQLITE */

  /* invalid option */
  sqldbal_test_open(SQLDBAL_DRIVER_SQLITE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    1,
                    invalid_option_list,
                    SQLDBAL_STATUS_PARAM);

  /* sqlite3_open_v2 */
  sqldbal_test_open(SQLDBAL_DRIVER_SQLITE,
                    "/invalid",
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0,
                    NULL,
                    SQLDBAL_STATUS_OPEN);

  /* sqlite3_trace_v2 */
  g_sqldbal_err_sqlite3_trace_v2_ctr = 0;
  sqldbal_test_open(SQLDBAL_DRIVER_SQLITE,
                    g_db_config_list[2].location,
                    g_db_config_list[2].port,
                    g_db_config_list[2].username,
                    g_db_config_list[2].password,
                    g_db_config_list[2].database,
                    g_db_config_list[2].flags | SQLDBAL_FLAG_DEBUG,
                    0,
                    NULL,
                    SQLDBAL_STATUS_OPEN);
  g_sqldbal_err_sqlite3_trace_v2_ctr = -1;
}

/**
 * Run through different failure scenarios when calling @ref sqldbal_close.
 */
static void
sqldbal_test_all_error_close(void){
  /* sqldbal_sqlite_close - SQLDBAL_DRIVER_SQLITE  */
  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);

  /* sqlite3_close_v2 */
  g_sqldbal_err_sqlite3_close_v2_ctr = 0;
  g_rc = sqldbal_close(g_db);
  assert(g_rc == SQLDBAL_STATUS_CLOSE);
  g_sqldbal_err_sqlite3_close_v2_ctr = -1;
  sqldbal_status_code_clear(g_db);

  g_rc = sqldbal_close(g_db);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Test harness for @ref sqldbal_exec which allows the caller to
 * conveniently inject return values prior to calling this and then to
 * have this function check the result.
 *
 * @param[in] driver        See @ref sqldbal_open.
 * @param[in] sql           See @ref sqldbal_exec.
 * @param[in] callback      See @ref sqldbal_exec.
 * @param[in] user_data     See @ref sqldbal_exec.
 * @param[in] expect_status Expected status return code of @ref sqldbal_open.
 */
static void
sqldbal_test_exec(enum sqldbal_driver driver,
                  const char *const sql,
                  sqldbal_exec_callback_fp callback,
                  void *user_data,
                  enum sqldbal_status_code expect_status){
  sqldbal_test_db_open(driver);

  g_rc = sqldbal_exec(g_db,
                      sql,
                      callback,
                      user_data);
  assert(g_rc == expect_status);

  g_rc = sqldbal_close(g_db);
  assert(g_rc == expect_status);
}

/**
 * This callback always returns a failure result code.
 *
 * @param[in] user_data       Ignored.
 * @param[in] num_cols        Ignored.
 * @param[in] col_result_list Ignored.
 * @param[in] col_length_list Ignored.
 * @return Always -1 error.
 */
static int
sqldbal_test_error_exec_fp(void *user_data,
                           size_t num_cols,
                           char **col_result_list,
                           size_t *col_length_list){
  (void)user_data;
  (void)num_cols;
  (void)col_result_list;
  (void)col_length_list;
  return -1;
}

/**
 * This callback always returns success and does no processing on the results.
 *
 * @param[in] user_data       Ignored.
 * @param[in] num_cols        Ignored.
 * @param[in] col_result_list Ignored.
 * @param[in] col_length_list Ignored.
 * @return Always 0 success.
 */
static int
sqldbal_test_exec_do_nothing_fp(void *user_data,
                                size_t num_cols,
                                char **col_result_list,
                                size_t *col_length_list){
  (void)user_data;
  (void)num_cols;
  (void)col_result_list;
  (void)col_length_list;
  return 0;
}

/**
 * Run through different failure scenarios when calling @ref sqldbal_exec.
 */
static void
sqldbal_test_all_error_exec(void){
  /* sqldbal_mariadb_exec - SQLDBAL_DRIVER_MARIADB */

  /* mysql_real_query */
  sqldbal_test_exec(SQLDBAL_DRIVER_MARIADB,
                    g_sql_invalid,
                    NULL,
                    NULL,
                    SQLDBAL_STATUS_EXEC);

  /* callback */
  sqldbal_test_exec(SQLDBAL_DRIVER_MARIADB,
                    g_sql_valid_sel,
                    sqldbal_test_error_exec_fp,
                    NULL,
                    SQLDBAL_STATUS_EXEC);

  /* mysql_store_result / mysql_errno */
  g_sqldbal_err_mysql_store_result_ctr = 0;
  g_sqldbal_err_mysql_errno_ctr = 0;
  sqldbal_test_exec(SQLDBAL_DRIVER_MARIADB,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_store_result_ctr = -1;
  g_sqldbal_err_mysql_errno_ctr = -1;

  /* sqldbal_pq_exec - SQLDBAL_DRIVER_POSTGRESQL */

  /* PQexecParams */
  g_sqldbal_err_PQexecParams_ctr = 0;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_invalid,
                    NULL,
                    NULL,
                    SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_PQexecParams_ctr = -1;

  /* PQresultStatus */
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_invalid,
                    NULL,
                    NULL,
                    SQLDBAL_STATUS_EXEC);

  /* si_int_to_size - 1 */
  g_sqldbal_err_si_int_to_size_ctr = 0;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_si_int_to_size_ctr = -1;

  /* sqldbal_reallocarray - 1 */
  g_sqldbal_err_realloc_ctr = 1;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  /* sqldbal_reallocarray - 2 */
  g_sqldbal_err_realloc_ctr = 2;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  /* sqldbal_reallocarray - 3 */
  g_sqldbal_err_realloc_ctr = 3;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  /* si_size_to_int - 1 */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_si_size_to_int_ctr = -1;

  /* si_int_to_size - 2 */
  g_sqldbal_err_si_int_to_size_ctr = 1;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_si_int_to_size_ctr = -1;

  /* sqldbal_str_hex2bin */
  g_sqldbal_err_malloc_ctr = 3;
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* callback */
  sqldbal_test_exec(SQLDBAL_DRIVER_POSTGRESQL,
                    g_sql_valid_sel,
                    sqldbal_test_error_exec_fp,
                    NULL,
                    SQLDBAL_STATUS_EXEC);

  /* sqldbal_sqlite_exec - SQLDBAL_DRIVER_SQLITE */

  /* sqldbal_sqlite_exec_callback - sqldbal_reallocarray */
  g_sqldbal_err_realloc_ctr = 0;
  sqldbal_test_exec(SQLDBAL_DRIVER_SQLITE,
                    g_sql_valid_sel,
                    sqldbal_test_exec_do_nothing_fp,
                    NULL,
                    SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_realloc_ctr = -1;

  /* callback */
  sqldbal_test_exec(SQLDBAL_DRIVER_SQLITE,
                    g_sql_valid_sel,
                    sqldbal_test_error_exec_fp,
                    NULL,
                    SQLDBAL_STATUS_EXEC);
}

/**
 * Test harness for @ref sqldbal_last_insert_id.
 *
 * @param[in] name          Sequence name.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 * @param[in] expect_result Expected last insert id returned.
 */
static void
sqldbal_test_last_insert_id(const char *const name,
                            enum sqldbal_status_code expect_status,
                            uint64_t expect_result){
  uint64_t result;

  g_rc = sqldbal_last_insert_id(g_db, name, &result);
  assert(g_rc == expect_status);
  if(g_rc == SQLDBAL_STATUS_OK){
    assert(result == expect_result);
  }
}

/**
 * Run through different failure scenarios when calling
 * @ref sqldbal_last_insert_id.
 */
static void
sqldbal_test_all_error_last_insert_id(void){
  const char *const pq_seq_name = "test_sequence_id_seq";

  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);
  sqldbal_test_stmt_generate_insert_id();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_pq_last_insert_id */

  /* sqldbal_stmt_column_int64 */
  g_sqldbal_err_si_size_to_int_ctr = 1;
  sqldbal_test_last_insert_id(pq_seq_name, SQLDBAL_STATUS_OVERFLOW, 0);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* si_int64_to_uint64 */
  g_sqldbal_err_si_int64_to_uint64_ctr = 0;
  sqldbal_test_last_insert_id(pq_seq_name, SQLDBAL_STATUS_COLUMN_COERCE, 0);
  g_sqldbal_err_si_int64_to_uint64_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_db_close();

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_test_stmt_generate_insert_id();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_sqlite_last_insert_id */

  /* si_int64_to_uint64 */
  g_sqldbal_err_si_int64_to_uint64_ctr = 0;
  sqldbal_test_last_insert_id(NULL, SQLDBAL_STATUS_OVERFLOW, 0);
  g_sqldbal_err_si_int64_to_uint64_ctr = -1;

  sqldbal_test_db_close();
}

/**
 * Test harness for @ref sqldbal_begin_transaction, @ref sqldbal_commit,
 * and @ref sqldbal_rollback.
 *
 * @param[in] commit_type   0 to test @ref sqldbal_begin_transaction. 1 to
 *                          test @ref sqldbal_commit, and 2 to test
 *                          @ref sqldbal_rollback.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_mariadb_transaction(int commit_type,
                                 enum sqldbal_status_code expect_status){
  assert(commit_type >= 0 && commit_type <= 2);

  sqldbal_test_db_open(SQLDBAL_DRIVER_MARIADB);

  g_rc = sqldbal_begin_transaction(g_db);
  if(commit_type == 0){
    assert(g_rc == expect_status);
  }
  else{
    assert(g_rc == SQLDBAL_STATUS_OK);
  }

  if(commit_type == 1){
    g_rc = sqldbal_commit(g_db);
    assert(g_rc == expect_status);
  }
  else if(commit_type == 2){
    g_rc = sqldbal_rollback(g_db);
    assert(g_rc == expect_status);
  }

  g_rc = sqldbal_close(g_db);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_prepare.
 *
 * @param[in] sql           SQL query to prepare.
 * @param[in] sql_len       Length of @p sql in bytes, or -1 if
 *                          null-terminated.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_prepare(const char *const sql,
                          size_t sql_len,
                          enum sqldbal_status_code expect_status){
  g_rc = sqldbal_stmt_prepare(g_db, sql, sql_len, &g_stmt);
  assert(g_rc == expect_status);

  g_rc = sqldbal_stmt_close(g_stmt);
  assert(g_rc == expect_status);
}

/**
 * Run through different failure scenarios when creating prepared statements.
 */
static void
sqldbal_test_all_error_stmt_prepare(void){
  /* SQLDBAL_DRIVER_MARIADB */

  sqldbal_test_db_open(SQLDBAL_DRIVER_MARIADB);

  /* sqldbal_stmt_prepare */

  /* malloc */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_stmt_prepare(NULL, 0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* sqldbal_mariadb_stmt_prepare */

  /* malloc */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_malloc_ctr = 1;
  sqldbal_test_stmt_prepare(NULL, 0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* mysql_stmt_init */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_mysql_stmt_init_ctr = 0;
  sqldbal_test_stmt_prepare(NULL, 0, SQLDBAL_STATUS_PREPARE);
  g_sqldbal_err_mysql_stmt_init_ctr = -1;

  /* mysql_stmt_prepare */
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_prepare(g_sql_invalid, SIZE_MAX, SQLDBAL_STATUS_PREPARE);

  /* sqldbal_reallocarray */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_realloc_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  /* sqldbal_mariadb_stmt_get_num_cols */

  /* mysql_stmt_errno */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_mysql_stmt_errno_ctr = 0;
  g_sqldbal_err_mysql_stmt_result_metadata_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_mysql_stmt_errno_ctr = -1;
  g_sqldbal_err_mysql_stmt_result_metadata_ctr = -1;

  sqldbal_test_db_close();

  /* SQLDBAL_DRIVER_POSTGRESQL */

  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);

  /* sqldbal_pq_stmt_prepare */

  /* malloc */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_malloc_ctr = 1;
  sqldbal_test_stmt_prepare(NULL, 0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* sqldbal_pq_gen_stmt_name - malloc */
  g_sqldbal_err_malloc_ctr = 2;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;

  /* sqldbal_pq_gen_stmt_name - sprintf */
  g_sqldbal_err_sprintf_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_sprintf_ctr = -1;

  /* PQprepare */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_PQprepare_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_invalid, SIZE_MAX, SQLDBAL_STATUS_PREPARE);
  g_sqldbal_err_PQprepare_ctr = -1;

  /* PQresultStatus */
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_prepare(g_sql_invalid, SIZE_MAX, SQLDBAL_STATUS_PREPARE);

  /* sqldbal_pq_stmt_allocate_param_list */

  /* PQdescribePrepared */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_PQresultStatus_ctr = 1;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_PREPARE);
  g_sqldbal_err_PQresultStatus_ctr = -1;

  /* si_int_to_size */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_si_int_to_size_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_int_to_size_ctr = -1;

  /* calloc */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_calloc_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_calloc_ctr = -1;

  /* sqldbal_reallocarray - 1 */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_realloc_ctr = 0;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  /* sqldbal_reallocarray - 2 */
  sqldbal_status_code_clear(g_db);
  g_sqldbal_err_realloc_ctr = 1;
  sqldbal_test_stmt_prepare(g_sql_valid_sel, SIZE_MAX, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_realloc_ctr = -1;

  sqldbal_test_db_close();

  /* SQLDBAL_DRIVER_SQLITE */

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);

  /* sqldbal_sqlite_stmt_prepare */

  /* sql_len > INT_MAX */
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_prepare(g_sql_invalid, SIZE_MAX - 1, SQLDBAL_STATUS_PARAM);

  /* sqlite3_prepare_v2 */
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_prepare(g_sql_invalid, SIZE_MAX, SQLDBAL_STATUS_PREPARE);

  sqldbal_test_db_close();
}

/**
 * Test harness for @ref sqldbal_stmt_bind_text.
 */
static void
sqldbal_test_bind_text_noerror(void){
  g_rc = sqldbal_stmt_bind_text(g_stmt, 0, "test", SIZE_MAX);
  assert(g_rc == SQLDBAL_STATUS_OK);
}

/**
 * Test harness for @ref sqldbal_stmt_bind_blob.
 *
 * @param[in] col_idx       Placeholder index referenced in prepared statement.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_bind_blob(size_t col_idx,
                       enum sqldbal_status_code expect_status){
  const char *const blob = "test";

  g_rc = sqldbal_stmt_bind_blob(g_stmt, col_idx, blob, strlen(blob));
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_bind_int64.
 *
 * @param[in] col_idx       Placeholder index referenced in prepared statement.
 * @param[in] i64           Integer to bind.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_bind_int64(size_t col_idx,
                        int64_t i64,
                        enum sqldbal_status_code expect_status){
  g_rc = sqldbal_stmt_bind_int64(g_stmt, col_idx, i64);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_column_blob.
 *
 * @param[in] col_idx       Column index.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_column_blob(size_t col_idx,
                              enum sqldbal_status_code expect_status){
  const void *blob;
  size_t blobsz;

  g_rc = sqldbal_stmt_column_blob(g_stmt, col_idx, &blob, &blobsz);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_column_int64.
 *
 * @param[in] col_idx       Column index.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_column_int64(size_t col_idx,
                               enum sqldbal_status_code expect_status){
  int64_t i64;

  g_rc = sqldbal_stmt_column_int64(g_stmt, col_idx, &i64);
  assert(g_rc == expect_status);
}

/**
 * Test harness for @ref sqldbal_stmt_column_text.
 *
 * @param[in] col_idx       Column index.
 * @param[in] expect_status Expected status return code of the function under
 *                          test.
 */
static void
sqldbal_test_stmt_column_text(size_t col_idx,
                              enum sqldbal_status_code expect_status){
  const char *text;
  size_t textsz;

  g_rc = sqldbal_stmt_column_text(g_stmt, col_idx, &text, &textsz);
  assert(g_rc == expect_status);
}

/**
 * Run through different failure scenarios when calling
 * @ref sqldbal_stmt_prepare.
 */
static void
sqldbal_test_all_error_stmt_execute(void){
  sqldbal_test_db_open(SQLDBAL_DRIVER_MARIADB);
  sqldbal_test_stmt_generate_update();

  /* mysql_stmt_attr_set */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_mysql_stmt_attr_set_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_stmt_attr_set_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* mysql_stmt_bind_param */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_mysql_stmt_bind_param_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_stmt_bind_param_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* mysql_stmt_execute */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_mysql_stmt_execute_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_stmt_execute_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* mysql_stmt_store_result */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_mysql_stmt_store_result_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_stmt_store_result_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* The following mysql errors require a select statement with results. */
  sqldbal_test_stmt_generate_select();

  /* mysql_stmt_result_metadata - mysql_stmt_errno */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_mysql_stmt_result_metadata_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_mysql_stmt_result_metadata_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* mysql_stmt_bind_result */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_mysql_stmt_bind_result_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_stmt_bind_result_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_mariadb_stmt_allocate_bind_in_list */

  /* calloc - 1 */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_calloc_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_calloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* calloc - 2 */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_calloc_ctr = 1;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_calloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* calloc - 3 */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_calloc_ctr = 2;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_calloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* si_size_to_uint */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_si_size_to_uint_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_si_size_to_uint_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* malloc */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  sqldbal_test_db_close();

  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);
  sqldbal_test_stmt_generate_update();

  /* PQexecPrepared - PQresultStatus */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_PQresultStatus_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_PQresultStatus_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* si_int_to_size */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_si_int_to_size_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_si_int_to_size_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* The following PostgreSQL error require a select statement with results. */
  sqldbal_test_stmt_generate_select();

  /* calloc */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_calloc_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_calloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  sqldbal_test_db_close();

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_test_stmt_generate_update();

  /* sqlite3_reset */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_sqlite3_reset_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_sqlite3_reset_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqlite3_step - num_retries >= SQLDBAL_SQLITE_MAX_NUM_RETRIES */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_busy_sqlite3_step_ctr = 11;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_busy_sqlite3_step_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqlite3_step - error */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_sqlite3_step_ctr = 0;
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_sqlite3_step_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  sqldbal_test_db_close();
}

/**
 * Run through different failure scenarios when calling
 * @ref sqldbal_stmt_fetch.
 */
static void
sqldbal_test_all_error_stmt_fetch(void){
  sqldbal_test_db_open(SQLDBAL_DRIVER_MARIADB);
  sqldbal_test_stmt_generate_select_article();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);

  /* sqldbal_mariadb_stmt_fetch */

  /* mysql_stmt_fetch */
  g_sqldbal_err_mysql_stmt_fetch_ctr = 0;
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_FETCH, SQLDBAL_FETCH_ERROR);
  g_sqldbal_err_mysql_stmt_fetch_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_stmt_close_sql();
  sqldbal_test_db_close();

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_test_stmt_generate_select_article();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);

  /* sqldbal_sqlite_stmt_fetch */

  /* sqlite3_step - error */
  g_sqldbal_err_sqlite3_step_ctr = 0;
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_FETCH, SQLDBAL_FETCH_ERROR);
  g_sqldbal_err_sqlite3_step_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqlite3_step - SQLITE_BUSY < SQLDBAL_SQLITE_MAX_NUM_RETRIES */
  g_sqldbal_busy_sqlite3_step_ctr = 3;
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);
  g_sqldbal_busy_sqlite3_step_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqlite3_step - SQLITE_BUSY >= SQLDBAL_SQLITE_MAX_NUM_RETRIES */
  g_sqldbal_busy_sqlite3_step_ctr = 11;
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_FETCH, SQLDBAL_FETCH_ERROR);
  g_sqldbal_busy_sqlite3_step_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_stmt_close_sql();
  sqldbal_test_db_close();
}

/**
 * Run through different failure scenarios when calling these functions.
 *   - @ref sqldbal_stmt_bind_blob
 *   - @ref sqldbal_stmt_bind_int64
 *   - @ref sqldbal_stmt_bind_text
 */
static void
sqldbal_test_all_error_stmt_bind(void){
  sqldbal_test_db_open(SQLDBAL_DRIVER_MARIADB);
  sqldbal_test_stmt_generate_update();

  /* sqldbal_stmt_bind_null */

  /* null - sqldbal_stmt_bind_null */
  sqldbal_test_stmt_prepare_sql();
  g_rc = sqldbal_stmt_bind_null(g_stmt, 1);
  assert(g_rc == SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_mariadb_stmt_bind_blob */

  /* blob - sqldbal_stmt_bind_in_range */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_blob(1, SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* malloc */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_bind_blob(0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_mariadb_stmt_bind_int64 */

  /* i64 - sqldbal_stmt_bind_in_range */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_bind_int64(1, 100, SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* si_int64_to_llong */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_int64_to_llong_ctr = 0;
  sqldbal_test_bind_int64(0, 100, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_int64_to_llong_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* malloc */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_bind_int64(0, 1, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_mariadb_stmt_bind_text */

  /* text - sqldbal_stmt_bind_in_range */
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_bind_text(1, SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_stmt_bind_text - si_add_size_t */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_add_size_t_ctr = 0;
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_si_add_size_t_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* malloc */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  sqldbal_test_db_close();
  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);
  sqldbal_test_stmt_generate_update();

  /* sqldbal_pq_stmt_bind_blob */

  /* si_size_to_int */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_bind_blob(0, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* malloc */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_bind_blob(0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_pq_stmt_bind_int64 */

  /* sprintf */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_sprintf_ctr = 0;
  sqldbal_test_bind_int64(0, 1, SQLDBAL_STATUS_BIND);
  g_sqldbal_err_sprintf_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_pq_stmt_bind_text */

  /* malloc */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  sqldbal_test_db_close();
  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_test_stmt_generate_update();

  /* sqldbal_sqlite_stmt_bind_null */

  /* sqldbal_sqlite_get_col_idx - si_add_size_t */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_add_size_t_ctr = 0;
  g_rc = sqldbal_stmt_bind_null(g_stmt, 0);
  assert(g_rc == SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_add_size_t_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqlite3_bind_null */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_sqlite3_bind_null_ctr = 0;
  g_rc = sqldbal_stmt_bind_null(g_stmt, 0);
  assert(g_rc == SQLDBAL_STATUS_BIND);
  g_sqldbal_err_sqlite3_bind_null_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_sqlite_stmt_bind_blob */

  /* si_size_to_int - 1*/
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_bind_blob(0, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* si_size_to_int - 2 */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_size_to_int_ctr = 1;
  sqldbal_test_bind_blob(0, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqlite3_bind_blob */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_sqlite3_bind_blob_ctr = 0;
  sqldbal_test_bind_blob(0, SQLDBAL_STATUS_BIND);
  g_sqldbal_err_sqlite3_bind_blob_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_sqlite_stmt_bind_int64 */

  /* si_size_to_int */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_bind_int64(0, 1, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqlite3_bind_int64 */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_sqlite3_bind_int64_ctr = 0;
  sqldbal_test_bind_int64(0, 1, SQLDBAL_STATUS_BIND);
  g_sqldbal_err_sqlite3_bind_int64_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqldbal_sqlite_stmt_bind_text */

  /* si_size_to_int - 1 */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* si_size_to_int - 2 */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_si_size_to_int_ctr = 1;
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  /* sqlite3_bind_text */
  sqldbal_test_stmt_prepare_sql();
  g_sqldbal_err_sqlite3_bind_text_ctr = 0;
  sqldbal_test_stmt_bind_text(0, SQLDBAL_STATUS_BIND);
  g_sqldbal_err_sqlite3_bind_text_ctr = -1;
  sqldbal_status_code_clear(g_db);
  sqldbal_test_stmt_close_sql();

  sqldbal_test_db_close();
}

/**
 * Run through different failure scenarios when calling these functions.
 *   - @ref sqldbal_stmt_column_blob
 *   - @ref sqldbal_stmt_column_int64
 *   - @ref sqldbal_stmt_column_text
 */
static void
sqldbal_test_all_error_stmt_column(void){
  sqldbal_test_db_open(SQLDBAL_DRIVER_MARIADB);
  sqldbal_test_stmt_generate_select_article();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);

  /* sqldbal_stmt_column_type */
  sqldbal_test_stmt_column_type(11, SQLDBAL_TYPE_ERROR, SQLDBAL_STATUS_PARAM);

  /* sqldbal_stmt_column_blob */

  /* sqldbal_stmt_column_in_range */
  sqldbal_test_stmt_column_blob(12, SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);

  /* sqldbal_stmt_column_int64 */

  /* sqldbal_stmt_column_in_range */
  sqldbal_test_stmt_column_int64(12, SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);

  /* sqldbal_mariadb_stmt_column_int64 */

  /* strtoll */
  g_sqldbal_err_strtoll_ctr = 0;
  g_sqldbal_err_strtoll_value = LLONG_MAX;
  sqldbal_test_stmt_column_int64(0, SQLDBAL_STATUS_COLUMN_COERCE);
  g_sqldbal_err_strtoll_ctr = -1;
  g_sqldbal_err_strtoll_value = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_stmt_column_text */

  /* sqldbal_stmt_column_in_range */
  sqldbal_test_stmt_column_text(12, SQLDBAL_STATUS_PARAM);
  sqldbal_status_code_clear(g_db);

  sqldbal_test_stmt_close_sql();
  sqldbal_test_db_close();

  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);
  sqldbal_test_stmt_generate_select_article();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);

  /* sqldbal_pq_stmt_column_type */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_type(0, SQLDBAL_TYPE_ERROR, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_pq_stmt_column_blob */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_blob(4, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* si_int_to_size */
  g_sqldbal_err_si_int_to_size_ctr = 0;
  sqldbal_test_stmt_column_blob(4, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_int_to_size_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_str_hex2bin */
  g_sqldbal_err_malloc_ctr = 0;
  sqldbal_test_stmt_column_blob(4, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_malloc_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_pq_stmt_column_text */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_text(1, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_pq_stmt_column_int64 */

  /* strtoll */
  g_sqldbal_err_strtoll_ctr = 0;
  g_sqldbal_err_strtoll_value = LLONG_MAX;
  sqldbal_test_stmt_column_int64(0, SQLDBAL_STATUS_COLUMN_COERCE);
  g_sqldbal_err_strtoll_ctr = -1;
  g_sqldbal_err_strtoll_value = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_stmt_close_sql();
  sqldbal_test_db_close();

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_test_stmt_generate_select_article();
  sqldbal_test_stmt_prepare_sql();
  sqldbal_test_stmt_execute(SQLDBAL_STATUS_OK);
  sqldbal_test_stmt_fetch(SQLDBAL_STATUS_OK, SQLDBAL_FETCH_ROW);

  /* sqldbal_sqlite_stmt_column_type */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_type(0, SQLDBAL_TYPE_ERROR, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;

  /* sqldbal_sqlite_stmt_column_blob */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_blob(4, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* si_int_to_size */
  g_sqldbal_err_si_int_to_size_ctr = 0;
  sqldbal_test_stmt_column_blob(4, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_int_to_size_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* *blob == NULL */
  g_sqldbal_err_sqlite3_column_blob_ctr = 0;
  sqldbal_test_stmt_column_blob(4, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_sqlite3_column_blob_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_sqlite_stmt_column_int64 */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_int64(0, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* sqldbal_sqlite_stmt_column_text */

  /* si_size_to_int */
  g_sqldbal_err_si_size_to_int_ctr = 0;
  sqldbal_test_stmt_column_text(1, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_size_to_int_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* si_int_to_size */
  g_sqldbal_err_si_int_to_size_ctr = 0;
  sqldbal_test_stmt_column_text(1, SQLDBAL_STATUS_OVERFLOW);
  g_sqldbal_err_si_int_to_size_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* *text == NULL */
  g_sqldbal_err_sqlite3_column_text_ctr = 0;
  sqldbal_test_stmt_column_text(1, SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_sqlite3_column_text_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_stmt_close_sql();
  sqldbal_test_db_close();
}

/**
 * Run through different failure scenarios when calling
 * @ref sqldbal_stmt_close.
 */
static void
sqldbal_test_all_error_stmt_close(void){
  sqlite3_stmt *stmt;

  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);
  sqldbal_test_stmt_generate_update();

  sqldbal_test_stmt_prepare_sql();
  stmt = sqldbal_stmt_handle(g_stmt);
  sqldbal_test_bind_text_noerror();
  g_sqldbal_err_sqlite3_finalize_ctr = 0;
  sqldbal_test_stmt_close(SQLDBAL_STATUS_CLOSE);
  g_sqldbal_err_sqlite3_finalize_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqlite3_finalize(stmt);

  sqldbal_test_db_close();
}

/**
 * Run through different failure scenarios when calling these functions.
 *   - @ref sqldbal_begin_transaction.
 *   - @ref sqldbal_rollback.
 *   - @ref sqldbal_commit.
 */
static void
sqldbal_test_all_error_transaction(void){
  /* sqldbal_mariadb_begin_transaction - SQLDBAL_DRIVER_MARIADB */

  /* mysql_autocommit */
  g_sqldbal_err_mysql_autocommit_ctr = 0;
  sqldbal_test_mariadb_transaction(0, SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_autocommit_ctr = -1;

  /* sqldbal_mariadb_commit - SQLDBAL_DRIVER_MARIADB */

  /* mysql_commit */
  g_sqldbal_err_mysql_commit_ctr = 0;
  sqldbal_test_mariadb_transaction(1, SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_commit_ctr = -1;

  /* mysql_autocommit */
  g_sqldbal_err_mysql_autocommit_ctr = 1;
  sqldbal_test_mariadb_transaction(1, SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_autocommit_ctr = -1;

  /* sqldbal_mariadb_rollback - SQLDBAL_DRIVER_MARIADB */

  /* mysql_rollback */
  g_sqldbal_err_mysql_rollback_ctr = 0;
  sqldbal_test_mariadb_transaction(2, SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_rollback_ctr = -1;

  /* mysql_autocommit */
  g_sqldbal_err_mysql_autocommit_ctr = 1;
  sqldbal_test_mariadb_transaction(2, SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_mysql_autocommit_ctr = -1;

  /* sqldbal_pq_begin_transaction - SQLDBAL_DRIVER_POSTGRESQL */
  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);

  /* PQexec */
  g_sqldbal_err_PQexec_ctr = 0;
  g_rc = sqldbal_begin_transaction(g_db);
  assert(g_rc == SQLDBAL_STATUS_NOMEM);
  g_sqldbal_err_PQexec_ctr = -1;
  sqldbal_status_code_clear(g_db);

  /* PQresultStatus */
  g_sqldbal_err_PQresultStatus_ctr = 0;
  g_rc = sqldbal_begin_transaction(g_db);
  assert(g_rc == SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_PQresultStatus_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_db_close();

  /* sqldbal_sqlite_begin_transaction - SQLDBAL_DRIVER_SQLITE */
  sqldbal_test_db_open(SQLDBAL_DRIVER_SQLITE);

  /* sqldbal_sqlite_noresult - sqlite3_exec */
  g_sqldbal_err_sqlite3_exec_ctr = 0;
  g_rc = sqldbal_begin_transaction(g_db);
  assert(g_rc == SQLDBAL_STATUS_EXEC);
  g_sqldbal_err_sqlite3_exec_ctr = -1;
  sqldbal_status_code_clear(g_db);

  sqldbal_test_db_close();
}

/**
 * Test failure in @ref sqldbal_pq_is_oid.
 */
static void
sqldbal_test_all_error_pq_is_oid(void){
  const struct sqldbal_pq_db *pq_db;
  int rc;

  sqldbal_test_db_open(SQLDBAL_DRIVER_POSTGRESQL);
  pq_db = sqldbal_pq_get_pq_db_handle(g_db);
  rc = sqldbal_pq_is_oid(pq_db, 9999, "does not exist");
  assert(rc == 1);
  sqldbal_test_db_close();
}

/**
 * Test all failures in @ref sqldbal_sqlite_trace_hook.
 */
static void
sqldbal_test_all_error_trace(void){
  int rc;

  rc = sqldbal_sqlite_trace_hook(9999, NULL, NULL, NULL);
  assert(rc == 0);
}

/**
 * Test all error conditions not already covered by the normal test cases.
 *
 * This includes each error branch that sets an error value
 * in @ref sqldbal_status_code.
 */
static void
sqldbal_functional_test_error_conditions(void){
  sqldbal_test_all_error_open();
  sqldbal_test_all_error_close();
  sqldbal_test_all_error_exec();
  sqldbal_test_all_error_last_insert_id();
  sqldbal_test_all_error_stmt_prepare();
  sqldbal_test_all_error_stmt_execute();
  sqldbal_test_all_error_stmt_fetch();
  sqldbal_test_all_error_stmt_bind();
  sqldbal_test_all_error_stmt_column();
  sqldbal_test_all_error_stmt_close();
  sqldbal_test_all_error_transaction();
  sqldbal_test_all_error_pq_is_oid();
  sqldbal_test_all_error_trace();
}

/**
 * Run all functional testing including creating tables, querying data, and
 * all error conditions for each database driver.
 */
static void
sqldbal_functional_test_all(void){
  sqldbal_test_load_config_file(PATH_CONFIG_PREFIX "/mariadb.txt",
                                SQLDBAL_DRIVER_MARIADB,
                                &g_db_config_list[0]);
  sqldbal_test_load_config_file(PATH_CONFIG_PREFIX "/postgresql.txt",
                                SQLDBAL_DRIVER_POSTGRESQL,
                                &g_db_config_list[1]);
  sqldbal_test_load_config_file(PATH_CONFIG_PREFIX "/sqlite.txt",
                                SQLDBAL_DRIVER_SQLITE,
                                &g_db_config_list[2]);

  sqldbal_functional_test_db_list();
  sqldbal_functional_test_encryption();
  sqldbal_functional_test_timeout();
  sqldbal_functional_test_debug();
  sqldbal_functional_test_sqlite_open_options();
  sqldbal_functional_test_errstr();
  sqldbal_functional_test_error_conditions();
}

/**
 * Main entry point for test program that runs the unit and functional testing.
 *
 * @param[in] argc Number of arguments in @p argv.
 * @param[in] argv String array containing the program name and
 *                 optional arguments.
 * @retval 0 All tests passed.
 * @retval 1 Error.
 */
int main(int argc,
         char *argv[]){
  int c;
  int unit_test_only;

  unit_test_only = 0;
  while((c = getopt(argc, argv, "u")) != -1){
    switch(c){
      case 'u':
        unit_test_only = 1;
        break;
      default:
        return 1;
    }
  }
  argc -= 1;
  argv += 1;

  sqldbal_unit_test_all();

  if(!unit_test_only){
    sqldbal_functional_test_all();
  }

  return 0;
}

