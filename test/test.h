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
#ifndef SQLDBAL_TEST_H
#define SQLDBAL_TEST_H

#include <sys/types.h>

#include <libpq-fe.h>
#include <mysql.h>
#include <sqlite3.h>

#include "../src/sqldbal.h"

struct sqldbal_pq_db;

/**
 * Get the PostgreSQL database structure, used by test suite.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return See @ref sqldbal_pq_db.
 */
const struct sqldbal_pq_db *
sqldbal_pq_get_pq_db_handle(const struct sqldbal_db *const db);

int
sqldbal_pq_is_oid(const struct sqldbal_pq_db *const pq_db,
                  Oid oid,
                  const char *const typname);

char *
sqldbal_str_hex2bin(const char *const hex_str,
                    size_t *const binlen);

void *
sqldbal_reallocarray(void *ptr,
                     size_t nelem,
                     size_t size);

int
si_add_size_t(size_t a,
              size_t b,
              size_t *const result);

int
si_mul_size_t(const size_t a,
              const size_t b,
              size_t *const result);

int
si_size_to_uint(const size_t size,
                unsigned int *const ui);

int
si_int64_to_llong(const int64_t i64,
                  long long *ll);

int
si_llong_to_int64(const long long int lli,
                  int64_t *i64);

int
si_size_to_int(const size_t size,
               int *const i);

int
si_int_to_size(const int i,
               size_t *const size);

int
si_int64_to_uint64(const int64_t i64,
                   uint64_t *const ui64);

char *
sqldbal_stpcpy(char *s1,
               const char *s2);

char *
sqldbal_strdup(const char *s);

void
sqldbal_strtoi64(struct sqldbal_db *const db,
                 const char *const text,
                 int64_t *const lli);

enum sqldbal_status_code
sqldbal_strtoui(struct sqldbal_db *const db,
                const char *const str,
                unsigned int maxval,
                unsigned int *const ui);

enum sqldbal_status_code
sqldbal_status_code_set(struct sqldbal_db *const db,
                        enum sqldbal_status_code status);

void
sqldbal_errstr_set(struct sqldbal_db *const db,
                   const char *const errstr);

int
sqldbal_test_seam_dec_err_ctr(int *const test_err_ctr);

void *
sqldbal_test_seam_calloc(size_t nelem,
                         size_t elsize);

void *
sqldbal_test_seam_malloc(size_t size);

my_bool
sqldbal_test_seam_mysql_autocommit(MYSQL *mysql,
                                   my_bool auto_mode);

my_bool
sqldbal_test_seam_mysql_commit(MYSQL *mysql);

unsigned int
sqldbal_test_seam_mysql_errno(MYSQL *mysql);

MYSQL *
sqldbal_test_seam_mysql_init(MYSQL *mysql);

int
sqldbal_test_seam_mysql_options(MYSQL *mysql,
                                enum mysql_option option,
                                const void *arg);

my_bool
sqldbal_test_seam_mysql_rollback(MYSQL *mysql);

my_bool
sqldbal_test_seam_mysql_stmt_attr_set(MYSQL_STMT *stmt,
                                      enum enum_stmt_attr_type attr_type,
                                      const void *attr);

my_bool
sqldbal_test_seam_mysql_stmt_bind_param(MYSQL_STMT *stmt,
                                        MYSQL_BIND *bind);

my_bool
sqldbal_test_seam_mysql_stmt_bind_result(MYSQL_STMT *stmt,
                                         MYSQL_BIND *bind);

unsigned int
sqldbal_test_seam_mysql_stmt_errno(MYSQL_STMT *stmt);

int
sqldbal_test_seam_mysql_stmt_execute(MYSQL_STMT *stmt);

int
sqldbal_test_seam_mysql_stmt_fetch(MYSQL_STMT *stmt);

MYSQL_STMT *
sqldbal_test_seam_mysql_stmt_init(MYSQL *mysql);

MYSQL_RES *
sqldbal_test_seam_mysql_stmt_result_metadata(MYSQL_STMT *stmt);

MYSQL_RES *
sqldbal_test_seam_mysql_store_result(MYSQL *mysql);

int
sqldbal_test_seam_mysql_stmt_store_result(MYSQL_STMT *stmt);

PGconn *
sqldbal_test_seam_PQconnectdb(const char *conninfo);

PGresult *
sqldbal_test_seam_PQexec(PGconn *conn,
                         const char *command);

PGresult *
sqldbal_test_seam_PQexecParams(PGconn *conn,
                               const char *command,
                               int nParams,
                               const Oid *paramTypes,
                               const char *const *paramValues,
                               const int *paramLengths,
                               const int *paramFormats,
                               int resultFormat);

PGresult *
sqldbal_test_seam_PQprepare(PGconn *conn,
                            const char *stmtName,
                            const char *query,
                            int nParams,
                            const Oid *paramTypes);

ExecStatusType
sqldbal_test_seam_PQresultStatus(const PGresult *res);

void *
sqldbal_test_seam_realloc(void *ptr,
                          size_t size);

int
sqldbal_test_seam_sprintf(char *s,
                          const char *format, ...);

size_t
sqldbal_test_seam_strlen(const char *s);

long long
sqldbal_test_seam_strtoll(const char *str,
                          char **endptr,
                          int base);

unsigned long
sqldbal_test_seam_strtoul(const char *str,
                          char **endptr,
                          int base);
int
sqldbal_test_seam_sqlite3_bind_blob(sqlite3_stmt *stmt,
                                    int index,
                                    const void *value,
                                    int nbytes,
                                    void (*destructor)(void *ptr));

int
sqldbal_test_seam_sqlite3_bind_int64(sqlite3_stmt *stmt,
                                     int index,
                                     sqlite3_int64 value);

int
sqldbal_test_seam_sqlite3_bind_text(sqlite3_stmt *stmt,
                                    int index,
                                    const char *value,
                                    int nbytes,
                                    void (*destructor)(void *ptr));

int
sqldbal_test_seam_sqlite3_bind_null(sqlite3_stmt *stmt,
                                    int index);

int
sqldbal_test_seam_sqlite3_close_v2(sqlite3 *db);

const void *
sqldbal_test_seam_sqlite3_column_blob(sqlite3_stmt *stmt,
                                      int iCol);

const unsigned char *
sqldbal_test_seam_sqlite3_column_text(sqlite3_stmt *stmt,
                                      int iCol);

int
sqldbal_test_seam_sqlite3_errcode(sqlite3 *db);

int
sqldbal_test_seam_sqlite3_exec(sqlite3 *db,
                               const char *sql,
                               int (*callback)(void *, int, char **, char **),
                               void *user_data,
                               char **errmsg);

int
sqldbal_test_seam_sqlite3_finalize(sqlite3_stmt *stmt);

sqlite3_int64
sqldbal_test_seam_sqlite3_last_insert_rowid(sqlite3 *db);

int
sqldbal_test_seam_sqlite3_reset(sqlite3_stmt *stmt);

int
sqldbal_test_seam_sqlite3_step(sqlite3_stmt *stmt);

int
sqldbal_sqlite_trace_hook(unsigned mask,
                          void *context,
                          void *P,
                          void *X);

int
sqldbal_test_seam_sqlite3_trace_v2(sqlite3 *db,
                                   unsigned uMask,
                                   int (*xCallback)(unsigned,
                                                    void *,
                                                    void *,
                                                    void *),
                                   void *pCtx);

/**
 * @section test_seams_countdown_global
 *
 * The test harnesses control most of the test seams through the use of
 * global counter values.
 *
 * Setting a global counter to -1 will make the test seam function operate
 * as it normally would. If set to a positive value, the value will continue
 * to decrement every time the function gets called. When the counter
 * reaches 0, the test seam will force the function to return an error value.
 *
 * For example, initially setting the counter to 0 will force the test seam
 * to return an error condition the first time it gets called. Setting the
 * value to 1 initially will force the test seam to return an error
 * condition on the second time it gets called.
 */

/**
 * Counter for @ref sqldbal_test_seam_calloc.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_calloc_ctr;

/**
 * Counter for @ref sqldbal_test_seam_malloc.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_malloc_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_autocommit.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_autocommit_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_commit.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_commit_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_errno.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_errno_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_init.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_init_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_options.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_options_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_rollback.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_rollback_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_attr_set.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_attr_set_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_bind_param.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_bind_param_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_bind_result.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_bind_result_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_errno.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_errno_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_execute.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_execute_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_fetch.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_fetch_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_init.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_init_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_result_metadata.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_result_metadata_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_stmt_store_result.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_stmt_store_result_ctr;

/**
 * Counter for @ref sqldbal_test_seam_mysql_store_result.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_mysql_store_result_ctr;

/**
 * Counter for @ref sqldbal_test_seam_PQconnectdb.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_PQconnectdb_ctr;

/**
 * Counter for @ref sqldbal_test_seam_PQexec.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_PQexec_ctr;

/**
 * Counter for @ref sqldbal_test_seam_PQexecParams.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_PQexecParams_ctr;

/**
 * Counter for @ref sqldbal_test_seam_PQprepare.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_PQprepare_ctr;

/**
 * Counter for @ref sqldbal_test_seam_PQresultStatus.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_PQresultStatus_ctr;

/**
 * Counter for @ref sqldbal_test_seam_realloc.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_realloc_ctr;

/**
 * Counter for @ref si_add_size_t.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_add_size_t_ctr;

/**
 * Counter for @ref si_int_to_size.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_int_to_size_ctr;

/**
 * Counter for @ref si_size_to_int.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_size_to_int_ctr;

/**
 * Counter for @ref si_size_to_uint.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_size_to_uint_ctr;

/**
 * Counter for @ref si_int64_to_llong.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_int64_to_llong_ctr;

/**
 * Counter for @ref si_llong_to_int64.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_llong_to_int64_ctr;

/**
 * Counter for @ref si_int64_to_uint64.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_si_int64_to_uint64_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sprintf.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sprintf_ctr;

/**
 * Counter for @ref sqldbal_test_seam_strlen.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_strlen_ctr;

/**
 * Value to return in @ref sqldbal_test_seam_strlen.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern size_t g_sqldbal_err_strlen_ret_value;

/**
 * Counter for @ref sqldbal_test_seam_strtoll.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_strtoll_ctr;

/**
 * Value to return in @ref sqldbal_test_seam_strtoll.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern long long int g_sqldbal_err_strtoll_value;

/**
 * Counter for @ref sqldbal_test_seam_strtoul.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_strtoul_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_bind_blob.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_bind_blob_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_bind_int64.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_bind_int64_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_bind_text.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_bind_text_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_bind_null.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_bind_null_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_close_v2.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_close_v2_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_column_blob.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_column_blob_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_column_text.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_column_text_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_errcode.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_errcode_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_exec.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_exec_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_finalize.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_finalize_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_last_insert_rowid.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_last_insert_rowid_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_reset.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_reset_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_step.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_busy_sqlite3_step_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_step.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_step_ctr;

/**
 * Counter for @ref sqldbal_test_seam_sqlite3_trace_v2.
 *
 * See @ref test_seams_countdown_global for more details.
 */
extern int g_sqldbal_err_sqlite3_trace_v2_ctr;

#endif /* SQLDBAL_TEST_H */

