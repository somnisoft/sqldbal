/**
 * @file
 * @brief Test seams for the SQLDBAL library.
 * @author James Humphrey (mail@somnisoft.com)
 * @version 0.99
 *
 * Allows the SQLDBAL testing framwork to inject specific return values
 * by some of the standard library functions.
 *
 * This makes it possible to test less common errors like out of memory
 * conditions and network errors.
 *
 * This software has been placed into the public domain using CC0.
 */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

/**
 * See @ref g_sqldbal_err_calloc_ctr and @ref test_seams_countdown_global.
 */
int g_sqldbal_err_calloc_ctr = -1;

/**
 * See @ref g_sqldbal_err_malloc_ctr and @ref test_seams_countdown_global.
 */
int g_sqldbal_err_malloc_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_autocommit_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_autocommit_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_commit_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_commit_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_errno_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_errno_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_init_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_init_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_options_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_options_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_rollback_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_rollback_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_attr_set_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_attr_set_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_bind_param_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_bind_param_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_bind_result_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_bind_result_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_errno_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_errno_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_execute_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_execute_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_fetch_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_fetch_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_init_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_init_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_result_metadata_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_result_metadata_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_stmt_store_result_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_stmt_store_result_ctr = -1;

/**
 * See @ref g_sqldbal_err_mysql_store_result_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_mysql_store_result_ctr = -1;

/**
 * See @ref g_sqldbal_err_PQconnectdb_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_PQconnectdb_ctr = -1;

/**
 * See @ref g_sqldbal_err_PQexec_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_PQexec_ctr = -1;

/**
 * See @ref g_sqldbal_err_PQexecParams_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_PQexecParams_ctr = -1;

/**
 * See @ref g_sqldbal_err_PQprepare_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_PQprepare_ctr = -1;

/**
 * See @ref g_sqldbal_err_PQresultStatus_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_PQresultStatus_ctr = -1;

/**
 * See @ref g_sqldbal_err_realloc_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_realloc_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_add_size_t_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_add_size_t_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_int_to_size_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_int_to_size_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_size_to_int_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_size_to_int_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_size_to_uint_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_size_to_uint_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_int64_to_llong_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_int64_to_llong_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_ulong_to_size_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_ulong_to_size_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_llong_to_int64_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_llong_to_int64_ctr = -1;

/**
 * See @ref g_sqldbal_err_si_int64_to_uint64_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_si_int64_to_uint64_ctr = -1;

/**
 * See @ref g_sqldbal_err_sprintf_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sprintf_ctr = -1;

/**
 * See @ref g_sqldbal_err_strlen_ctr.
 */
int g_sqldbal_err_strlen_ctr = -1;

/**
 * See @ref g_sqldbal_err_strtoll_ctr.
 */
int g_sqldbal_err_strtoll_ctr = -1;

/**
 * See @ref g_sqldbal_err_strtoll_value.
 */
long long int g_sqldbal_err_strtoll_value = -1;

/**
 * See @ref g_sqldbal_err_strtoul_ctr.
 */
int g_sqldbal_err_strtoul_ctr = -1;

/**
 * See @ref g_sqldbal_err_strlen_ret_value.
 */
size_t g_sqldbal_err_strlen_ret_value = 0;

/**
 * See @ref g_sqldbal_err_sqlite3_bind_blob_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_bind_blob_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_bind_int64_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_bind_int64_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_bind_text_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_bind_text_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_bind_null_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_bind_null_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_close_v2_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_close_v2_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_column_blob_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_column_blob_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_column_text_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_column_text_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_errcode_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_errcode_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_exec_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_exec_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_finalize_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_finalize_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_last_insert_rowid_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_last_insert_rowid_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_reset_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_reset_ctr = -1;

/**
 * See @ref g_sqldbal_busy_sqlite3_step_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_busy_sqlite3_step_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_step_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_step_ctr = -1;

/**
 * See @ref g_sqldbal_err_sqlite3_trace_v2_ctr and
 * @ref test_seams_countdown_global.
 */
int g_sqldbal_err_sqlite3_trace_v2_ctr = -1;

/**
 * Decrement an error counter until it reaches -1.
 *
 * Once a counter reaches -1, it will return a successful response (1). This
 * typically gets used to denote when to cause a function to fail. For example,
 * the unit test or functional test might need to cause the realloc() function
 * to fail after calling it a third time.
 *
 * @param[in,out] test_err_ctr Integer counter to decrement.
 * @retval 0 The counter has been decremented, but did not reach -1 yet.
 * @retval 1 The counter has reached -1.
 */
int
sqldbal_test_seam_dec_err_ctr(int *const test_err_ctr){
  if(*test_err_ctr >= 0){
    *test_err_ctr -= 1;
    if(*test_err_ctr < 0){
      return 1;
    }
  }
  return 0;
}

/**
 * Allows the test harness to control when calloc() fails.
 *
 * @param[in] nelem  Number of elements to allocate.
 * @param[in] elsize Number of bytes in each @p nelem.
 * @retval void* Pointer to new allocated memory.
 * @retval NULL  Memory allocation failed.
 */
void *
sqldbal_test_seam_calloc(size_t nelem,
                         size_t elsize){
  void *alloc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_calloc_ctr)){
    errno = ENOMEM;
    alloc = NULL;
  }
  else{
    alloc = calloc(nelem, elsize);
  }
  return alloc;
}

/**
 * Allows the test harness to control when malloc() fails.
 *
 * @param[in] size Number of bytes to allocate.
 * @retval void* Pointer to new allocated memory.
 * @retval NULL  Memory allocation failed.
 */
void *
sqldbal_test_seam_malloc(size_t size){
  void *alloc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_malloc_ctr)){
    errno = ENOMEM;
    alloc = NULL;
  }
  else{
    alloc = malloc(size);
  }
  return alloc;
}

/**
 * Allows the test harness to control when mysql_autocommit() fails.
 *
 * @param[in] mysql     MySQL database handle.
 * @param[in] auto_mode Turn on/off automode.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_autocommit(MYSQL *mysql,
                                   char auto_mode){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_autocommit_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_autocommit(mysql, auto_mode);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_commit() fails.
 *
 * @param[in] mysql MySQL database handle.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_commit(MYSQL *mysql){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_commit_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_commit(mysql);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_errno() fails.
 *
 * @param[in] mysql MySQL database handle.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
unsigned int
sqldbal_test_seam_mysql_errno(MYSQL *mysql){
  unsigned int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_errno_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_errno(mysql);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_init() fails.
 *
 * @param[in] mysql Ignored.
 * @retval MYSQL* Pointer to new instance of MYSQL structure.
 * @retval NULL   Memory allocation failed.
 */
MYSQL *
sqldbal_test_seam_mysql_init(MYSQL *mysql){
  MYSQL *mysql_result;

  (void)mysql;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_init_ctr)){
    mysql_result = NULL;
  }
  else{
    mysql_result = mysql_init(NULL);
  }
  return mysql_result;
}

/**
 * Allows the test harness to control when mysql_options() fails.
 *
 * @param[in] mysql  MySQL database handle.
 * @param[in] option Option to set.
 * @param[in] arg    Option value.
 * @retval  0 Success.
 * @retval !0 Failed.
 */
int
sqldbal_test_seam_mysql_options(MYSQL *mysql,
                                enum mysql_option option,
                                const void *arg){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_options_ctr)){
    rc = -1;
  }
  else{
    rc = mysql_options(mysql, option, arg);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_rollback() fails.
 *
 * @param[in] mysql MySQL database handle.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_rollback(MYSQL *mysql){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_rollback_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_rollback(mysql);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_stmt_attr_set() fails.
 *
 * @param[in] stmt      MySQL statement handle.
 * @param[in] attr_type Attribute to set.
 * @param[in] attr      Value of attribute.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_stmt_attr_set(MYSQL_STMT *stmt,
                                      enum enum_stmt_attr_type attr_type,
                                      const void *attr){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_attr_set_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_stmt_attr_set(stmt, attr_type, attr);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_stmt_bind_param() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @param[in] bind List of MYSQL_BIND structures.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_stmt_bind_param(MYSQL_STMT *stmt,
                                        MYSQL_BIND *bind){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_bind_param_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_stmt_bind_param(stmt, bind);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_stmt_bind_result() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @param[in] bind List of MYSQL_BIND structures.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_stmt_bind_result(MYSQL_STMT *stmt,
                                         MYSQL_BIND *bind){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_bind_result_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_stmt_bind_result(stmt, bind);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_stmt_errno() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @retval  0 No error occurred.
 * @retval !0 mySQL error code.
 */
unsigned int
sqldbal_test_seam_mysql_stmt_errno(MYSQL_STMT *stmt){
  unsigned int error_code;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_errno_ctr)){
    error_code = !0;
  }
  else{
    error_code = mysql_stmt_errno(stmt);
  }
  return error_code;
}

/**
 * Allows the test harness to control when mysql_stmt_execute() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_stmt_execute(MYSQL_STMT *stmt){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_execute_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_stmt_execute(stmt);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_stmt_fetch() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_stmt_fetch(MYSQL_STMT *stmt){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_fetch_ctr)){
    rc = -2;
  }
  else{
    rc = mysql_stmt_fetch(stmt);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_stmt_init() fails.
 *
 * @param[in] mysql MySQL database handle.
 * @retval MYSQL_STMT* New statement handle.
 * @retval NULL        Error occurred.
 */
MYSQL_STMT *
sqldbal_test_seam_mysql_stmt_init(MYSQL *mysql){
  MYSQL_STMT *stmt;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_init_ctr)){
    stmt = NULL;
  }
  else{
    stmt = mysql_stmt_init(mysql);
  }
  return stmt;
}

/**
 * Allows the test harness to control when mysql_stmt_result_metadata() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @retval MYSQL_RES* New statement handle.
 * @retval NULL       Error occurred.
 */
MYSQL_RES *
sqldbal_test_seam_mysql_stmt_result_metadata(MYSQL_STMT *stmt){
  MYSQL_RES *result;

  if(sqldbal_test_seam_dec_err_ctr(
       &g_sqldbal_err_mysql_stmt_result_metadata_ctr)){
    result = NULL;
  }
  else{
    result = mysql_stmt_result_metadata(stmt);
  }
  return result;
}

/**
 * Allows the test harness to control when mysql_stmt_store_result() fails.
 *
 * @param[in] stmt MySQL statement handle.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_mysql_stmt_store_result(MYSQL_STMT *stmt){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_stmt_store_result_ctr)){
    rc = 1;
  }
  else{
    rc = mysql_stmt_store_result(stmt);
  }
  return rc;
}

/**
 * Allows the test harness to control when mysql_store_result() fails.
 *
 * @param[in] mysql MySQL database handle.
 * @retval MYSQL_RES* Buffered result set.
 * @retval NULL       Error occurred.
 */
MYSQL_RES *
sqldbal_test_seam_mysql_store_result(MYSQL *mysql){
  MYSQL_RES *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_mysql_store_result_ctr)){
    result = NULL;
  }
  else{
    result = mysql_store_result(mysql);
  }
  return result;
}

/**
 * Allows the test harness to control when PQconnectdb() fails.
 *
 * @param[in] conninfo Database connection parameters.
 * @retval PGconn* Database connection handle.
 * @retval NULL    Out of memory.
 */
PGconn *
sqldbal_test_seam_PQconnectdb(const char *conninfo){
  PGconn *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_PQconnectdb_ctr)){
    result = NULL;
  }
  else{
    result = PQconnectdb(conninfo);
  }
  return result;
}

/**
 * Allows the test harness to control when PQexec() fails.
 *
 * @param[in] conn    PostgreSQL database connection.
 * @param[in] command SQL command.
 * @retval PGresult*  Result of command execution.
 * @retval NULL       Out of memory.
 */
PGresult *
sqldbal_test_seam_PQexec(PGconn *conn,
                         const char *command){
  PGresult *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_PQexec_ctr)){
    result = NULL;
  }
  else{
    result = PQexec(conn, command);
  }
  return result;
}

/**
 * Allows the test harness to control when PQexecParams() fails.
 *
 * @param[in] conn         PostgreSQL database connection.
 * @param[in] command      SQL command.
 * @param[in] nParams      Number of parameters provided in
 *                         @p paramTypes, @p paramValues,
 *                         @p paramLengths, and @p paramFormats.
 * @param[in] paramTypes   Data types corresponding to @p paramValues.
 * @param[in] paramValues  Values of the parameters.
 * @param[in] paramLengths Lengths of values in @p paramValues.
 * @param[in] paramFormats Indicates text or binary value stored
 *                         in @p paramValues.
 * @param[in] resultFormat Indicates text or binary result.
 * @retval PGresult*  Result of command execution.
 * @retval NULL       Out of memory.
 */
PGresult *
sqldbal_test_seam_PQexecParams(PGconn *conn,
                               const char *command,
                               int nParams,
                               const Oid *paramTypes,
                               const char *const *paramValues,
                               const int *paramLengths,
                               const int *paramFormats,
                               int resultFormat){
  PGresult *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_PQexecParams_ctr)){
    result = NULL;
  }
  else{
    result = PQexecParams(conn,
                          command,
                          nParams,
                          paramTypes,
                          paramValues,
                          paramLengths,
                          paramFormats,
                          resultFormat);
  }
  return result;
}

/**
 * Allows the test harness to control when PQprepare() fails.
 *
 * @param[in] conn       PostgreSQL database connection.
 * @param[in] stmtName   Unique name to identify the statement.
 * @param[in] query      The query string to prepare.
 * @param[in] nParams    Number of parameters in @p paramTypes.
 * @param[in] paramTypes Data types to assign to the parameters.
 * @retval PGresult*  Result of command execution.
 * @retval NULL       Out of memory.
 */
PGresult *
sqldbal_test_seam_PQprepare(PGconn *conn,
                            const char *stmtName,
                            const char *query,
                            int nParams,
                            const Oid *paramTypes){
  PGresult *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_PQprepare_ctr)){
    result = NULL;
  }
  else{
    result = PQprepare(conn,
                       stmtName,
                       query,
                       nParams,
                       paramTypes);
  }
  return result;
}

/**
 * Allows the test harness to control when PQresultStatus() fails.
 *
 * @param[in] res Result handle.
 * @return ExecStatusType.
 */
ExecStatusType
sqldbal_test_seam_PQresultStatus(const PGresult *res){
  ExecStatusType status_type;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_PQresultStatus_ctr)){
    status_type = PGRES_EMPTY_QUERY;
  }
  else{
    status_type = PQresultStatus(res);
  }
  return status_type;
}

/**
 * Allows the test harness to control when realloc() fails.
 *
 * @param[in] ptr  Pointer to existing memory allocation, or NULL to allocate
 *                 a new memory region.
 * @param[in] size Number of bytes to allocate.
 * @retval void* Pointer to new allocated memory.
 * @retval NULL  Memory allocation failed.
 */
void *
sqldbal_test_seam_realloc(void *ptr,
                          size_t size){
  void *alloc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_realloc_ctr)){
    errno = ENOMEM;
    alloc = NULL;
  }
  else{
    alloc = realloc(ptr, size);
  }
  return alloc;
}

/**
 * Allows the test harness to control when sprintf() fails.
 *
 * @param[out] s      Store the result in this buffer.
 * @param[in]  format Format string to store in @p s.
 * @return Number of bytes written to @p s (excluding null-terminating byte).
 */
int
sqldbal_test_seam_sprintf(char *s,
                          const char *format, ...){
  int num_bytes;
  va_list ap;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sprintf_ctr)){
    num_bytes = -1;
  }
  else{
    va_start(ap, format);
    num_bytes = vsprintf(s, format, ap);
    va_end(ap);
  }
  return num_bytes;
}

/**
 * Allows the test harness to control the return value of strlen().
 *
 * @param[out] s String to get length of.
 * @return Length of string @p s.
 */
size_t
sqldbal_test_seam_strlen(const char *s){
  size_t len;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_strlen_ctr)){
    len = g_sqldbal_err_strlen_ret_value;
  }
  else{
    len = strlen(s);
  }
  return len;
}

/**
 * Allows the test harness to control when strtoll() fails.
 *
 * @param[in]  str    String to convert.
 * @param[out] endptr Last character processed in @p str.
 * @param[in]  base   Radix.
 * @return Converted value.
 */
long long
sqldbal_test_seam_strtoll(const char *str,
                          char **endptr,
                          int base){
  long long int result;
  static char endptr_null[] = "";

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_strtoll_ctr)){
    result = g_sqldbal_err_strtoll_value;
    errno = ERANGE;
    if(endptr){
      *endptr = endptr_null;
    }
  }
  else{
    result = strtoll(str, endptr, base);
  }
  return result;
}

/**
 * Allows the test harness to control when strtoul() fails.
 *
 * @param[in]  str    String to convert.
 * @param[out] endptr Last character processed in @p str.
 * @param[in]  base   Radix.
 * @return Converted value.
 */
unsigned long
sqldbal_test_seam_strtoul(const char *str,
                          char **endptr,
                          int base){
  unsigned long result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_strtoul_ctr)){
    result = 0;
    errno = EINVAL;
    if(endptr){
      *endptr = (char*)str;
    }
  }
  else{
    result = strtoul(str, endptr, base);
  }
  return result;
}

/**
 * Allows the test harness to control when sqlite3_bind_blob() fails.
 *
 * @param[in] stmt       SQLite statement handle.
 * @param[in] index      Column index.
 * @param[in] value      Value to bind.
 * @param[in] nbytes     Number of bytes in @p value, or -1 if null-terminated.
 * @param[in] destructor SQLITE_STATIC or SQLITE_TRANSIENT.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_bind_blob(sqlite3_stmt *stmt,
                                    int index,
                                    const void *value,
                                    int nbytes,
                                    void (*destructor)(void *ptr)){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_bind_blob_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_bind_blob(stmt, index, value, nbytes, destructor);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_bind_int64() fails.
 *
 * @param[in] stmt  SQLite statement handle.
 * @param[in] index Column index.
 * @param[in] value Value to bind.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_bind_int64(sqlite3_stmt *stmt,
                                     int index,
                                     sqlite3_int64 value){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_bind_int64_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_bind_int64(stmt, index, value);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_bind_text() fails.
 *
 * @param[in] stmt       SQLite statement handle.
 * @param[in] index      Column index.
 * @param[in] value      Value to bind.
 * @param[in] nbytes     Number of bytes in @p value, or -1 if null-terminated.
 * @param[in] destructor SQLITE_STATIC or SQLITE_TRANSIENT.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_bind_text(sqlite3_stmt *stmt,
                                    int index,
                                    const char *value,
                                    int nbytes,
                                    void (*destructor)(void *ptr)){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_bind_text_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_bind_text(stmt, index, value, nbytes, destructor);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_bind_null() fails.
 *
 * @param[in] stmt  SQLite statement handle.
 * @param[in] index Column index.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_bind_null(sqlite3_stmt *stmt,
                                    int index){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_bind_null_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_bind_null(stmt, index);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_close_v2() fails.
 *
 * @param[in] db SQLite database handle.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_close_v2(sqlite3 *db){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_close_v2_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_close_v2(db);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_column_blob() fails.
 *
 * @param[in] stmt SQLite statement handle.
 * @param[in] iCol Column index.
 * @return Blob data.
 */
const void *
sqldbal_test_seam_sqlite3_column_blob(sqlite3_stmt *stmt,
                                      int iCol){
  const void *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_column_blob_ctr)){
    result = NULL;
  }
  else{
    result = sqlite3_column_blob(stmt, iCol);
  }
  return result;
}

/**
 * Allows the test harness to control when sqlite3_column_text() fails.
 *
 * @param[in] stmt SQLite statement handle.
 * @param[in] iCol Column index.
 * @return Text data.
 */
const unsigned char *
sqldbal_test_seam_sqlite3_column_text(sqlite3_stmt *stmt,
                                      int iCol){
  const unsigned char *result;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_column_text_ctr)){
    result = NULL;
  }
  else{
    result = sqlite3_column_text(stmt, iCol);
  }
  return result;
}

/**
 * Allows the test harness to control when sqlite3_errcode() fails.
 *
 * @param[in] db SQLite database handle.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_errcode(sqlite3 *db){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_errcode_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_errcode(db);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_exec() fails.
 *
 * @param[in]  db        SQLite database handle.
 * @param[in]  sql       SQL statement to execute.
 * @param[in]  callback  Callback function to execute for each returned row.
 * @param[in]  user_data Data to pass to callback function.
 * @param[out] errmsg    Error message.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_exec(sqlite3 *db,
                               const char *sql,
                               int (*callback)(void *, int, char **, char **),
                               void *user_data,
                               char **errmsg){
  char *err_seam;
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_exec_ctr)){
    rc = SQLITE_NOMEM;

    err_seam = sqlite3_malloc(100);
    assert(err_seam);
    strcpy(err_seam, "test seam error");
    *errmsg = err_seam;
  }
  else{
    rc = sqlite3_exec(db, sql, callback, user_data, errmsg);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_finalize() fails.
 *
 * @param[in] stmt SQLite statement handle.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_finalize(sqlite3_stmt *stmt){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_finalize_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_finalize(stmt);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_last_insert_rowid()
 * returns a negative value.
 *
 * @param[in] db SQLite database handle.
 * @return Row id of last insert statement.
 */
sqlite3_int64
sqldbal_test_seam_sqlite3_last_insert_rowid(sqlite3 *db){
  sqlite3_int64 i64;

  if(sqldbal_test_seam_dec_err_ctr(
       &g_sqldbal_err_sqlite3_last_insert_rowid_ctr)){
    i64 = -1;
  }
  else{
    i64 = sqlite3_last_insert_rowid(db);
  }
  return i64;
}

/**
 * Allows the test harness to control when sqlite3_reset() fails.
 *
 * @param[in] stmt SQLite statement handle.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_reset(sqlite3_stmt *stmt){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_reset_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_reset(stmt);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_step() fails.
 *
 * @param[in] stmt SQLite statement handle.
 * @return SQLite status code.
 */
int
sqldbal_test_seam_sqlite3_step(sqlite3_stmt *stmt){
  int rc;

  sqldbal_test_seam_dec_err_ctr(&g_sqldbal_busy_sqlite3_step_ctr);
  if(g_sqldbal_busy_sqlite3_step_ctr != -1){
    rc = SQLITE_BUSY;
  }
  else if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_step_ctr)){
    rc = SQLITE_NOMEM;
  }
  else{
    rc = sqlite3_step(stmt);
  }
  return rc;
}

/**
 * Allows the test harness to control when sqlite3_trace_v2() fails.
 *
 * @param[in] db        SQLite database handle.
 * @param[in] uMask     Events to trace on.
 * @param[in] xCallback Called whenever @p uMask events occur.
 * @param[in] pCtx      User data passed to the @p xCallback function.
 * @retval  0 Success.
 * @retval !0 Error occurred.
 */
int
sqldbal_test_seam_sqlite3_trace_v2(sqlite3 *db,
                                   unsigned uMask,
                                   int (*xCallback)(unsigned,
                                                    void *,
                                                    void *,
                                                    void *),
                                   void *pCtx){
  int rc;

  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_sqlite3_trace_v2_ctr)){
    rc = -1;
  }
  else{
    rc = sqlite3_trace_v2(db, uMask, xCallback, pCtx);
  }
  return rc;
}

