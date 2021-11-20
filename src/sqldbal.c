/**
 * @file
 * @brief SQLDBAL - SQL Database Abstraction Library.
 * @author James Humphrey (mail@somnisoft.com)
 * @version 0.99
 *
 * SQL Database Abstraction Library that provides a high-level interface
 * for multiple database engines.
 *
 * This software has been placed into the public domain using CC0.
 */
#if defined(_WIN32) || defined(WIN32)
# define SQLDBAL_IS_WINDOWS
#endif /* SQLDBAL_IS_WINDOWS */

#ifdef SQLDBAL_IS_WINDOWS
# include <winsock2.h>
#else /* POSIX */
# include <sys/select.h>
#endif /* SQLDBAL_IS_WINDOWS */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqldbal.h"

#ifdef SQLDBAL_TEST
/**
 * Some functions need to have extern linkage so that we can redefine their
 * behavior in the external test suite.
 */
# define SQLDBAL_LINKAGE extern
# include "../test/seams.h"
#else /* !(SQLDBAL_TEST) */
/**
 * When not testing, all functions should have static linkage, except
 * for those in the header.
 */
# define SQLDBAL_LINKAGE static
#endif /* SQLDBAL_TEST */

/**
 * Highest port number available.
 */
#define SQLDBAL_MAX_PORT_NUMBER 65535

/**
 * Maximum buffer size for 64-bit signed integer.
 *
 * Minimum 64-bit integer:
 * -9223372036854775808
 * 123456789012345678901
 *         10        20 -> 21 bytes
 */
#define MAX_I64_STR_SZ 21

/**
 * Prepared statement compiled by the driver.
 */
struct sqldbal_stmt{
  /**
   * Database context.
   */
  struct sqldbal_db *db;

  /**
   * Number of parameters to bind in the statement.
   */
  size_t num_params;

  /**
   * Number of columns in the result.
   */
  size_t num_cols_result;

  /**
   * Driver-specific statement context.
   */
  void *handle;

  /**
   * Set to 1 if statement has been allocated and valid.
   */
  int valid;

  /**
   * Padding structure to align.
   */
  char pad[4];
};

/**
 * Driver-specific functions that all drivers must implement.
 */
struct sqldbal_driver_functions{
  /**
   * Open connection to database/server.
   *
   * SQLite opens a file handle whereas the other database engines will
   * connect to the database server.
   */
  void
  (*sqldbal_fp_open)(struct sqldbal_db *const db,
                     const char *const location,
                     const char *const port,
                     const char *const username,
                     const char *const password,
                     const char *const database,
                     const struct sqldbal_driver_option *const option_list,
                     size_t num_options);

  /**
   * Close connection to database/server.
   */
  void
  (*sqldbal_fp_close)(struct sqldbal_db *const db);

  /**
   * Get the driver database handle.
   */
  void *
  (*sqldbal_fp_db_handle)(const struct sqldbal_db *const db);

  /**
   * Get the driver compiled statement handle.
   */
  void *
  (*sqldbal_fp_stmt_handle)(const struct sqldbal_stmt *const stmt);

  /**
   * Begin database transaction.
   */
  void
  (*sqldbal_fp_begin_transaction)(struct sqldbal_db *const db);

  /**
   * Commit a database transaction started with
   * @ref sqldbal_fp_begin_transaction.
   */
  void
  (*sqldbal_fp_commit)(struct sqldbal_db *const db);

  /**
   * Rollback a transaction started with @ref sqldbal_fp_begin_transaction.
   */
  void
  (*sqldbal_fp_rollback)(struct sqldbal_db *const db);

  /**
   * Directly execute a SQL statement, skipping the separate statement
   * compilation steps.
   */
  void
  (*sqldbal_fp_exec)(struct sqldbal_db *const db,
                     const char *const sql,
                     sqldbal_exec_callback_fp callback,
                     void *user_data);

  /**
   * Get the insert id from the last SQL insert statement.
   */
  void
  (*sqldbal_fp_last_insert_id)(struct sqldbal_db *const db,
                               const char *const name,
                               uint64_t *insert_id);

  /**
   * Compile a SQL statement.
   */
  void
  (*sqldbal_fp_stmt_prepare)(struct sqldbal_db *const db,
                             const char *const sql,
                             size_t sql_len,
                             struct sqldbal_stmt *const stmt);

  /**
   * Assign binary data to the compiled statement.
   */
  void
  (*sqldbal_fp_stmt_bind_blob)(struct sqldbal_stmt *const stmt,
                               size_t col_idx,
                               const void *const blob,
                               size_t blobsz);

  /**
   * Assign an integer to the compiled statement.
   */
  void
  (*sqldbal_fp_stmt_bind_int64)(struct sqldbal_stmt *const stmt,
                                size_t col_idx,
                                int64_t i64);

  /**
   * Assign a string to the compiled statement.
   */
  void
  (*sqldbal_fp_stmt_bind_text)(struct sqldbal_stmt *const stmt,
                               size_t col_idx,
                               const char *const s,
                               size_t slen);

  /**
   * Assign a NULL value to the compiled statement.
   */
  void
  (*sqldbal_fp_stmt_bind_null)(struct sqldbal_stmt *const stmt,
                               size_t col_idx);

  /**
   * Execute a compiled statement with the previously bound parameters.
   */
  void
  (*sqldbal_fp_stmt_execute)(struct sqldbal_stmt *const stmt);

  /**
   * Get the next row in the result.
   */
  enum sqldbal_fetch_result
  (*sqldbal_fp_stmt_fetch)(struct sqldbal_stmt *const stmt);

  /**
   * Get the result column as a blob.
   */
  void
  (*sqldbal_fp_stmt_column_blob)(struct sqldbal_stmt *const stmt,
                                 size_t col_idx,
                                 const void **blob,
                                 size_t *blobsz);

  /**
   * Get the result column as a 64-bit integer.
   */
  void
  (*sqldbal_fp_stmt_column_int64)(struct sqldbal_stmt *const stmt,
                                  size_t col_idx,
                                  int64_t *i64);

  /**
   * Get the result column as UTF-8 text.
   */
  void
  (*sqldbal_fp_stmt_column_text)(struct sqldbal_stmt *const stmt,
                                 size_t col_idx,
                                 const char **text,
                                 size_t *textsz);

  /**
   * Get the column data type.
   */
  enum sqldbal_column_type
  (*sqldbal_fp_stmt_column_type)(struct sqldbal_stmt *const stmt,
                                 size_t col_idx);

  /**
   * Free resources associated with a prepared statement.
   */
  void
  (*sqldbal_fp_stmt_close)(struct sqldbal_stmt *const stmt);
};

/**
 * SQL database connection/handle.
 */
struct sqldbal_db{
  /**
   * String describing the last error that occurred.
   *
   * The error will get set by the database driver when possible. If not
   * available, a generic error string will get returned based on the
   * code in @ref sqldbal_status_code.
   */
  char *errstr;

  /**
   * Driver-specific database context.
   */
  void *handle;

  /**
   * List of functions SQL drivers must implement.
   *
   * See @ref sqldbal_driver_functions.
   */
  struct sqldbal_driver_functions functions;

  /**
   * Previous error set by the library or database driver.
   *
   * See @ref sqldbal_status_code.
   */
  enum sqldbal_status_code status_code;

  /**
   * See @ref sqldbal_flag.
   */
  enum sqldbal_flag flags;

  /**
   * SQL database driver in use.
   *
   * See @ref sqldbal_driver.
   */
  enum sqldbal_driver type;

  /**
   * Padding structure to align.
   */
  char pad[4];
};

/**
 * Add two size_t values and check for wrap.
 *
 * @param[in]  a      Add this value with @p b.
 * @param[in]  b      Add this value with @p a.
 * @param[out] result Save the addition to this memory location.
 * @retval 1 Value wrapped.
 * @retval 0 Value did not wrap.
 */
SQLDBAL_LINKAGE int
si_add_size_t(size_t a,
              size_t b,
              size_t *const result){
  int wraps;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_add_size_t_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  *result = a + b;
  if(*result < a){
    wraps = 1;
  }
  else{
    wraps = 0;
  }
  return wraps;
}

/**
 * Multiply two size_t values and check for wrap.
 *
 * @param[in]  a      Multiply this value with @p b.
 * @param[in]  b      Multiply this value with @p a.
 * @param[out] result Save the multiplication to this buffer.
 * @retval 1 Value wrapped.
 * @retval 0 Value did not wrap.
 */
SQLDBAL_LINKAGE int
si_mul_size_t(const size_t a,
              const size_t b,
              size_t *const result){
  int wraps;

  *result = a * b;
  if(b != 0 && a > SIZE_MAX / b){
    wraps = 1;
  }
  else{
    wraps = 0;
  }
  return wraps;
}

#ifdef SQLDBAL_MARIADB
/**
 * Convert a size_t to an unsigned int unless the value would truncate.
 *
 * @param[in]  size Convert this to unsigned int and store result in @p ui.
 * @param[out] ui   Converted from @p size.
 * @retval 1 Value would have truncated.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_size_to_uint(const size_t size,
                unsigned int *const ui){
  int truncated;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_size_to_uint_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(size > UINT_MAX){
    truncated = 1;
  }
  else{
    truncated = 0;
    *ui = (unsigned int)size;
  }
  return truncated;
}

/**
 * Convert an int64_t to a long long unless the value would overflow.
 *
 * @param[in]  i64 Convert this to a long long and store result in @p ll.
 * @param[out] ll  Converted from @p i64.
 * @retval 1 Value would have overflowed.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_int64_to_llong(const int64_t i64,
                  long long *ll){
  int overflow;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_int64_to_llong_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(i64 > LLONG_MAX){
    overflow = 1;
  }
  else{
    overflow = 0;
    *ll = (long long)i64;
  }
  return overflow;
}

/**
 * Convert an unsigned long to size_t unless the value would wrap.
 *
 * @param[in]  ul   Convert this to size_t and store result in @p size.
 * @param[out] size Converted from @p ul.
 * @retval 1 Value would have wrapped.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_ulong_to_size(unsigned long ul,
                 size_t *const size){
  int wraps;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_ulong_to_size_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(ul > SIZE_MAX){
    wraps = 1;
  }
  else{
    wraps = 0;
    *size = (size_t)ul;
  }
  return wraps;
}
#endif /* SQLDBAL_MARIADB */

#if defined(SQLDBAL_POSTGRESQL) || defined(SQLDBAL_SQLITE)
/**
 * Convert a size_t to an int unless the value would overflow.
 *
 * @param[in]  size Convert this to an int and store result in @p i.
 * @param[out] i    Converted from @p size.
 * @retval 1 Value would have overflowed.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_size_to_int(const size_t size,
               int *const i){
  int overflow;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_size_to_int_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(size > INT_MAX){
    overflow = 1;
  }
  else{
    overflow = 0;
    *i = (int)size;
  }
  return overflow;
}

/**
 * Convert an int to size_t unless the value would wrap.
 *
 * @param[in]  i    Convert this to size_t and store result in @p size.
 * @param[out] size Converted from @p i.
 * @retval 1 Value would have wrapped.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_int_to_size(const int i,
               size_t *const size){
  int wraps;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_int_to_size_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(i < 0){
    wraps = 1;
  }
  else{
    wraps = 0;
    *size = (size_t)i;
  }
  return wraps;
}

/**
 * Convert a 64 bit integer to a 64 bit unsigned integer unless the value
 * would overflow.
 *
 * @param[in]  i64  Convert this to uint64_t and store result in @p ui64.
 * @param[out] ui64 Converted from @p i64.
 * @retval 1 Value would have overflowed.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_int64_to_uint64(const int64_t i64,
                   uint64_t *const ui64){
  int overflow;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_int64_to_uint64_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(i64 < 0){
    overflow = 1;
  }
  else{
    overflow = 0;
    *ui64 = (uint64_t)i64;
  }
  return overflow;
}
#endif /* SQLDBAL_POSTGRESQL || SQLDBAL_SQLITE */

/**
 * Reallocate memory array.
 *
 * Similar to the following function call:
 * void *mem = realloc(ptr, nelem * size);
 *
 * However, unlike realloc, this will check for wrap when multiplying
 * @p nelem with @p size.
 *
 * @param[in] ptr   Pointer to a previous memory allocation, or NULL to
 *                  allocate a new memory region.
 * @param[in] nelem Number of elements in the array.
 * @param[in] size  Number of bytes in each @p nelem.
 * @retval void* Pointer to allocated memory region.
 * @retval NULL  Memory allocation failure.
 */
SQLDBAL_LINKAGE void *
sqldbal_reallocarray(void *ptr,
                     size_t nelem,
                     size_t size){
  void *alloc;
  size_t size_mul;

  if(si_mul_size_t(nelem, size, &size_mul)){
    errno = ENOMEM;
    alloc = NULL;
  }
  else{
    alloc = realloc(ptr, size_mul);
  }
  return alloc;
}

/**
 * Copy a string into a new dynamically allocated buffer.
 *
 * Returns a dynamically allocated string, with the same contents as the
 * input string. The caller must free the returned string when finished.
 *
 * @param[in] s String to duplicate.
 * @retval char* Pointer to a new dynamically allocated string duplicated
 *               from @p s.
 * @retval NULL  Failed to allocate memory for the new string.
 */
SQLDBAL_LINKAGE char *
sqldbal_strdup(const char *s){
  char *dup;
  size_t dup_len;

  dup_len = strlen(s);
  if(dup_len == SIZE_MAX){
    errno = ENOMEM;
    dup = NULL;
  }
  else{
    dup_len += 1;
    dup = malloc(dup_len);
    if(dup){
      memcpy(dup, s, dup_len);
    }
  }
  return dup;
}

/**
 * Set the internal error code to a new code.
 *
 * @param[in] db     See @ref sqldbal_db.
 * @param[in] status New error code to set to. See @ref sqldbal_status_code.
 * @return See @ref sqldbal_status_code.
 */
SQLDBAL_LINKAGE enum sqldbal_status_code
sqldbal_status_code_set(struct sqldbal_db *const db,
                        enum sqldbal_status_code status){
  if((unsigned)status >= SQLDBAL_STATUS__LAST){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
  }
  db->status_code = status;
  return status;
}

#if defined(SQLDBAL_MARIADB) || defined(SQLDBAL_POSTGRESQL)
/**
 * Convert a long long to int64_t unless the value would overflow.
 *
 * @param[in]  i64 Convert this to a long long and store result in @p ll.
 * @param[out] lli Converted from @p i64.
 * @retval 1 Value would have overflowed.
 * @retval 0 Value converted.
 */
SQLDBAL_LINKAGE int
si_llong_to_int64(const long long int lli,
                  int64_t *i64){
  int overflow;

#ifdef SQLDBAL_TEST
  if(sqldbal_test_seam_dec_err_ctr(&g_sqldbal_err_si_llong_to_int64_ctr)){
    return 1;
  }
#endif /* SQLDBAL_TEST */

  if(lli > INT64_MAX || lli < INT64_MIN){
    overflow = 1;
  }
  else{
    overflow = 0;
    *i64 = lli;
  }
  return overflow;
}

/**
 * Safely convert a string to 64-bit integer and verify the result.
 *
 * @param[in]  db   See @ref sqldbal_db.
 * @param[in]  text String to convert.
 * @param[out] i64  Converted value.
 */
SQLDBAL_LINKAGE void
sqldbal_strtoi64(struct sqldbal_db *const db,
                 const char *const text,
                 int64_t *const i64){
  char *ep;
  long long int lli;

  errno = 0;
  lli = strtoll(text, &ep, 10);
  if(text[0] == '\0' || *ep != '\0' ||
     (errno == ERANGE && (lli == LLONG_MAX || lli == LLONG_MIN)) ||
     si_llong_to_int64(lli, i64)){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_COLUMN_COERCE);
    *i64 = 0;
  }
}

/**
 * Convert a string into unsigned integer and set status code if error.
 *
 * @param[in]  db     See @ref sqldbal_db.
 * @param[in]  str    String containing unsigned integer [0,@p maxval].
 *                    Setting this paramater to NULL will set @p ui to 0.
 * @param[in]  maxval Any value above this will result in error.
 * @param[out] ui     Converted value gets stored here.
 * @return See @ref sqldbal_status_code.
 */
SQLDBAL_LINKAGE enum sqldbal_status_code
sqldbal_strtoui(struct sqldbal_db *const db,
                const char *const str,
                unsigned int maxval,
                unsigned int *const ui){
  unsigned long ul;
  char *ep;

  *ui = 0;
  if(str){
    ul = strtoul(str, &ep, 10);
    if(str[0] == '\0' || *ep != '\0' || ul > maxval){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
    }
    else{
      /* ul <= UINT_MAX as proven above. */
      *ui = (unsigned int)ul;
    }
  }
  return sqldbal_status_code_get(db);
}
#endif /* defined(SQLDBAL_MARIADB) || defined(SQLDBAL_POSTGRESQL) */

/**
 * Free the existing error string and replace with a new error string.
 *
 * @param[in] db     See @ref sqldbal_db.
 * @param[in] errstr Error string to set in the database context. This
 *                   function makes a copy of the string.
 */
SQLDBAL_LINKAGE void
sqldbal_errstr_set(struct sqldbal_db *const db,
                   const char *const errstr){
  free(db->errstr);
  db->errstr = sqldbal_strdup(errstr);
  if(db->errstr == NULL){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
}

/**
 * Convenience function that sets both the error status code and error string.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] status_code See @ref sqldbal_status_code.
 * @param[in] errstr      Error string to set in the database context.
 */
static void
sqldbal_err_set(struct sqldbal_db *const db,
                enum sqldbal_status_code status_code,
                const char *const errstr){
  sqldbal_status_code_set(db, status_code);
  sqldbal_errstr_set(db, errstr);
}

#ifdef SQLDBAL_MARIADB

#include <mysql.h>

#if MYSQL_VERSION_ID >= 50600
/**
 * MariaDB did not add the MYSQL_OPT_SSL_* options until version 5.6.
 */
# define SQLDBAL_MARIADB_HAS_SSL
#endif /* MYSQL_VERSION_ID >= 50600 */

/**
 * Maximum number of seconds allowed in the CONNECT_TIMEOUT option.
 */
#define SQLDBAL_MARIADB_MAX_CONNECT_TIMEOUT 1000

/**
 * Driver-specific compiled statement handle for MariaDB.
 */
struct sqldbal_mariadb_stmt{
  /**
   * MariaDB statement handle.
   */
  MYSQL_STMT *stmt;

  /**
   * Parameters to bind for outgoing statement.
   */
  MYSQL_BIND *bind_out;

  /**
   * Store the fetched row into this bind list.
   */
  MYSQL_BIND *bind_in_list;

  /**
   * Length of stored result for corresponding entry in @ref bind_in_list.
   */
  unsigned long *bind_in_length_list;

  /**
   * Null value flag for corresponding entry in @ref bind_in_list.
   */
  char *bind_in_null_list;
};

/**
 * Convenience function that sets the status code and the error string
 * generated by the database function.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] mysql_db    MariaDB database handle.
 * @param[in] status_code See @ref sqldbal_status_code.
 */
static void
sqldbal_mariadb_error(struct sqldbal_db *const db,
                      MYSQL *const mysql_db,
                      enum sqldbal_status_code status_code){
  const char *error;

  /* https://mariadb.com/kb/en/library/mysql_error */
  error = mysql_error(mysql_db);
  sqldbal_err_set(db, status_code, error);
}

/**
 * Convenience function that sets the status code and the error string
 * generated by the statement.
 *
 * @param[in] stmt        See @ref sqldbal_stmt.
 * @param[in] status_code See @ref sqldbal_status_code.
 */
static void
sqldbal_mariadb_stmt_error(const struct sqldbal_stmt *const stmt,
                           enum sqldbal_status_code status_code){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  const char *stmt_error;

  mariadb_stmt = stmt->handle;

  /* https://mariadb.com/kb/en/mysql_stmt_error */
  stmt_error = mysql_stmt_error(mariadb_stmt->stmt);
  sqldbal_err_set(stmt->db, status_code, stmt_error);
}

/**
 * Wrapper for the mysql_options function.
 *
 * @param[in] db     See @ref sqldbal_db.
 * @param[in] option One of the available options in MariaDB.
 * @param[in] arg    Option value.
 */
static void
sqldbal_mariadb_mysql_options(struct sqldbal_db *const db,
                              enum mysql_option option,
                              const void *arg){
  MYSQL *mysql;

  mysql = db->handle;

  /* https://mariadb.com/kb/en/mysql_options */
  if(mysql_options(mysql, option, arg) != 0){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
  }
}

/**
 * Handle driver-specific options for MariaDB.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] num_options Number of entries in @p opt_list.
 * @param[in] opt_list    See @ref sqldbal_driver_option.
 * @return sqldbal_status_code.
 */
static enum sqldbal_status_code
sqldbal_mariadb_set_options(struct sqldbal_db *const db,
                            size_t num_options,
                            const struct sqldbal_driver_option *const opt_list){
  size_t i;
  const struct sqldbal_driver_option *option;
  unsigned int timeout;

  for(i = 0; i < num_options; i++){
    option = &opt_list[i];
    if(strcmp(option->key, "CONNECT_TIMEOUT") == 0){
      if(sqldbal_strtoui(db,
                         option->value,
                         SQLDBAL_MARIADB_MAX_CONNECT_TIMEOUT,
                         &timeout) == SQLDBAL_STATUS_OK){
        sqldbal_mariadb_mysql_options(db,
                                      MYSQL_OPT_CONNECT_TIMEOUT,
                                      (void*)&timeout);
      }
    }
#ifdef SQLDBAL_MARIADB_HAS_SSL
    else if(strcmp(option->key, "TLS_KEY") == 0){
      sqldbal_mariadb_mysql_options(db, MYSQL_OPT_SSL_KEY, option->value);
    }
    else if(strcmp(option->key, "TLS_CERT") == 0){
      sqldbal_mariadb_mysql_options(db, MYSQL_OPT_SSL_CERT, option->value);
    }
    else if(strcmp(option->key, "TLS_CA") == 0){
      sqldbal_mariadb_mysql_options(db, MYSQL_OPT_SSL_CA, option->value);
    }
    else if(strcmp(option->key, "TLS_CAPATH") == 0){
      sqldbal_mariadb_mysql_options(db, MYSQL_OPT_SSL_CAPATH, option->value);
    }
    else if(strcmp(option->key, "TLS_CIPHER") == 0){
      sqldbal_mariadb_mysql_options(db, MYSQL_OPT_SSL_CIPHER, option->value);
    }
#endif /* SQLDBAL_MARIADB_HAS_SSL */
    else{
      sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
    }
  }

  return sqldbal_status_code_get(db);
}

/**
 * Connect to MariaDB server.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] location    IP address or hostname.
 * @param[in] port        Server port number to connect to.
 * @param[in] username    User name to connect with.
 * @param[in] password    Optional password for @p username. Set to NULL if
 *                        user does not have a password.
 * @param[in] database    Optional database to open after connecting to the
 *                        server. Set this to NULL to prevent opening a
 *                        database after initially connecting.
 * @param[in] option_list See @ref sqldbal_driver_option.
 * @param[in] num_options Number of entries in @p option_list.
 */
static void
sqldbal_mariadb_open(struct sqldbal_db *const db,
                     const char *const location,
                     const char *const port,
                     const char *const username,
                     const char *const password,
                     const char *const database,
                     const struct sqldbal_driver_option *const option_list,
                     size_t num_options){
  MYSQL *mysql_db;
  unsigned int port_i;

  if(sqldbal_strtoui(db,
                     port,
                     SQLDBAL_MAX_PORT_NUMBER,
                     &port_i) != SQLDBAL_STATUS_OK){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
  }
  else{
    if(db->flags & SQLDBAL_FLAG_DEBUG){
      /* https://mariadb.com/kb/en/mysql_debug */
      mysql_debug("d");
    }

    /* https://mariadb.com/kb/en/mysql_init */
    mysql_db = mysql_init(NULL);
    if(mysql_db == NULL){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
    }
    else{
      db->handle = mysql_db;

      if(sqldbal_mariadb_set_options(db,
                                     num_options,
                                     option_list) == SQLDBAL_STATUS_OK){
        /* https://mariadb.com/kb/en/mysql_real_connect */
        if(mysql_real_connect(mysql_db,
                              location,
                              username,
                              password,
                              database,
                              port_i,
                              NULL,
                              0) == NULL){
          sqldbal_mariadb_error(db, mysql_db, SQLDBAL_STATUS_OPEN);
        }
      }
    }
  }
}

/**
 * Close the database connection.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_mariadb_close(struct sqldbal_db *const db){
  MYSQL *mysql_db;

  mysql_db = db->handle;

  /* https://mariadb.com/kb/en/mysql_close */
  mysql_close(mysql_db);
}

/**
 * Get the MariaDB database handle.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return MYSQL*.
 */
static void *
sqldbal_mariadb_db_handle(const struct sqldbal_db *const db){
  MYSQL *mysql_db;

  mysql_db = db->handle;
  return mysql_db;
}

/**
 * Get the MariaDB statement handle.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return MYSQL_STMT*.
 */
static void *
sqldbal_mariadb_stmt_handle(const struct sqldbal_stmt *const stmt){
  struct sqldbal_mariadb_stmt *mariadb_stmt;

  mariadb_stmt = stmt->handle;
  return mariadb_stmt->stmt;
}

/**
 * Begin MariaDB transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_mariadb_begin_transaction(struct sqldbal_db *const db){
  MYSQL *mysql_db;

  mysql_db = db->handle;

  /* https://mariadb.com/kb/en/mysql_autocommit */
  if(mysql_autocommit(mysql_db, 0)){
    sqldbal_mariadb_error(db, mysql_db, SQLDBAL_STATUS_EXEC);
  }
}

/**
 * Commit a MariaDB transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_mariadb_commit(struct sqldbal_db *const db){
  MYSQL *mysql_db;

  mysql_db = db->handle;

  /* https://mariadb.com/kb/en/mysql_commit     */
  /* https://mariadb.com/kb/en/mysql_autocommit */
  if(mysql_commit(mysql_db) ||
     mysql_autocommit(mysql_db, 1)){
    sqldbal_mariadb_error(db, mysql_db, SQLDBAL_STATUS_EXEC);
  }
}

/**
 * Rollback a MariaDB transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_mariadb_rollback(struct sqldbal_db *const db){
  MYSQL *mysql_db;

  mysql_db = db->handle;

  /* https://mariadb.com/kb/en/mysql_rollback   */
  /* https://mariadb.com/kb/en/mysql_autocommit */
  if(mysql_rollback(mysql_db) ||
     mysql_autocommit(mysql_db, 1)){
    sqldbal_mariadb_error(db, mysql_db, SQLDBAL_STATUS_EXEC);
  }
}

/**
 * Convert unsigned long values returned by mysql_fetch_lengths() into
 * a size_t list while checking for wrapping.
 *
 * @param[in]  lengths         List of unsigned long values to convert
 *                             to size_t.
 * @param[out] col_length_list Store the converted values in this list.
 * @param[in]  num_fields      Number of entries in @p lengths.
 * @retval 0 All length values converted to a size_t.
 * @retval 1 At least one length value did not fit into a size_t.
 */
static int
sqldbal_mariadb_fetch_lengths_conv_size(const unsigned long *const lengths,
                                        size_t *const col_length_list,
                                        unsigned int num_fields){
  unsigned int i;
  int rc;
  size_t size_conv;

  rc = 0;
  for(i = 0; i < num_fields; i++){
    if(si_ulong_to_size(lengths[i], &size_conv)){
      rc = 1;
      break;
    }
    col_length_list[i] = size_conv;
  }
  return rc;
}

/**
 * Execute a direct SQL statement.
 *
 * @param[in] db        See @ref sqldbal_db.
 * @param[in] sql       SQL command to execute.
 * @param[in] callback  Invokes this callback function for every returned row
 *                      in the query result set. Set this to NULL if query
 *                      results not needed.
 * @param[in] user_data Pass this to the first argument in the
 *                      @p callback function.
 */
static void
sqldbal_mariadb_exec(struct sqldbal_db *const db,
                     const char *const sql,
                     sqldbal_exec_callback_fp callback,
                     void *user_data){
  MYSQL *mysql_db;
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned int num_fields;
  size_t num_rows;
  size_t i;
  unsigned long *lengths;
  size_t *col_length_list;

  mysql_db = db->handle;

  /* https://mariadb.com/kb/en/mysql_real_query */
  if(mysql_real_query(mysql_db, sql, strlen(sql))){
    sqldbal_mariadb_error(db, mysql_db, SQLDBAL_STATUS_EXEC);
  }
  else{
    /* https://mariadb.com/kb/en/mysql_store_result */
    result = mysql_store_result(mysql_db);
    if(result){
      if(callback){
        /* https://mariadb.com/kb/en/mysql_num_fields */
        num_fields = mysql_num_fields(result);

        col_length_list = sqldbal_reallocarray(NULL,
                                               num_fields,
                                               sizeof(*col_length_list));
        if(col_length_list == NULL){
          sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
        }
        else{
          /* https://mariadb.com/kb/en/mysql_num_rows */
          num_rows = mysql_num_rows(result);
          for(i = 0; i < num_rows; i++){
            /* https://mariadb.com/kb/en/mysql_fetch_row */
            row = mysql_fetch_row(result);
            /* https://mariadb.com/kb/en/mysql_fetch_lengths */
            lengths = mysql_fetch_lengths(result);
            if(sqldbal_mariadb_fetch_lengths_conv_size(lengths, col_length_list, num_fields)){
              sqldbal_status_code_set(db, SQLDBAL_STATUS_OVERFLOW);
            }
            else{
              if(callback(user_data,
                          num_fields,
                          row,
                          col_length_list)){
                sqldbal_status_code_set(db, SQLDBAL_STATUS_EXEC);
                break;
              }
            }
          }
          free(col_length_list);
        }
      }
      /* https://mariadb.com/kb/en/mysql_free_result */
      mysql_free_result(result);
    }
    else if(mysql_errno(mysql_db)){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_EXEC);
    }
  }
}

/**
 * Get the insert id from the last SQL insert statement.
 *
 * @param[in]  db        See @ref sqldbal_db.
 * @param[in]  name      Unused.
 * @param[out] insert_id Last insert id.
 */
static void
sqldbal_mariadb_last_insert_id(struct sqldbal_db *const db,
                               const char *const name,
                               uint64_t *insert_id){
  MYSQL *mysql_db;
  my_ulonglong ull;

  (void)name;

  mysql_db = db->handle;

  /* https://mariadb.com/kb/en/mysql_insert_id */
  ull = mysql_insert_id(mysql_db);
  *insert_id = ull;
}

/**
 * Query prepared statement properties (column count, blob size, ...)
 * and allocate memory resources based on that information.
 *
 * @param[in] db   See @ref sqldbal_db.
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_mariadb_stmt_get_num_cols(struct sqldbal_db *const db,
                                  struct sqldbal_stmt *const stmt){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  MYSQL_RES *metadata;

  mariadb_stmt = stmt->handle;

  /* https://mariadb.com/kb/en/mysql_stmt_result_metadata */
  metadata = mysql_stmt_result_metadata(mariadb_stmt->stmt);
  if(metadata == NULL){
    stmt->num_cols_result = 0;
    /* https://mariadb.com/kb/en/mysql_stmt_errno */
    if(mysql_stmt_errno(mariadb_stmt->stmt)){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
    }
  }
  else{
    /* https://mariadb.com/kb/en/mysql_num_fields */
    stmt->num_cols_result = mysql_num_fields(metadata);

    /* https://mariadb.com/kb/en/mysql_free_result */
    mysql_free_result(metadata);
  }
}

/**
 * Prepare a database statement.
 *
 * @param[in] db      See @ref sqldbal_db.
 * @param[in] sql     SQL query to prepare.
 * @param[in] sql_len Length in bytes of @p sql, or -1 if null-terminated.
 * @param[in] stmt    See @ref sqldbal_stmt.
 */
static void
sqldbal_mariadb_stmt_prepare(struct sqldbal_db *const db,
                             const char *const sql,
                             size_t sql_len,
                             struct sqldbal_stmt *const stmt){
  MYSQL *mysql_db;
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  unsigned long num_bind_out;

  mysql_db = db->handle;

  mariadb_stmt = malloc(sizeof(*mariadb_stmt));
  if(mariadb_stmt == NULL){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    stmt->handle = mariadb_stmt;
    mariadb_stmt->bind_out            = NULL;
    mariadb_stmt->bind_in_list        = NULL;
    mariadb_stmt->bind_in_length_list = NULL;
    mariadb_stmt->bind_in_null_list   = NULL;

    /* https://mariadb.com/kb/en/mysql_stmt_init */
    mariadb_stmt->stmt = mysql_stmt_init(mysql_db);
    if(mariadb_stmt->stmt == NULL){
      sqldbal_mariadb_error(db, mysql_db, SQLDBAL_STATUS_PREPARE);
    }
    else{
      if(sql_len == SIZE_MAX){
        sql_len = strlen(sql);
      }

      /* https://mariadb.com/kb/en/mysql_stmt_prepare */
      if(mysql_stmt_prepare(mariadb_stmt->stmt, sql, sql_len) != 0){
        sqldbal_mariadb_stmt_error(stmt, SQLDBAL_STATUS_PREPARE);
      }
      else{
        /* https://mariadb.com/kb/en/mysql_stmt_param_count */
        num_bind_out = mysql_stmt_param_count(mariadb_stmt->stmt);
        stmt->num_params = (size_t)num_bind_out;
        mariadb_stmt->bind_out =
          sqldbal_reallocarray(NULL,
                               stmt->num_params,
                               sizeof(*mariadb_stmt->bind_out));
        if(mariadb_stmt->bind_out == NULL){
          sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);

          /* https://mariadb.com/kb/en/mysql_stmt_close */
          mysql_stmt_close(mariadb_stmt->stmt);
          mariadb_stmt->stmt = NULL;
        }
        else{
          sqldbal_mariadb_stmt_get_num_cols(db, stmt);
          memset(mariadb_stmt->bind_out,
                 0,
                 stmt->num_params * sizeof(*mariadb_stmt->bind_out));
        }
      }
    }
  }
}

/**
 * Free the MariaDB bind buffer if it has already been allocated.
 *
 * The bind buffer can get re-used across statement executions.
 *
 * @param[in] bind MariaDB bind structure.
 */
static void
sqldbal_mariadb_stmt_bind_free_buf(MYSQL_BIND *bind){
  if(bind->buffer){
    free(bind->buffer);
    bind->buffer = NULL;
  }
}

/**
 * Assign binary data to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] blob    Binary data saved to BLOB or BYTEA types.
 * @param[in] blobsz  Length of @p blob in bytes.
 */
static void
sqldbal_mariadb_stmt_bind_blob(struct sqldbal_stmt *const stmt,
                               size_t col_idx,
                               const void *const blob,
                               size_t blobsz){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  MYSQL_BIND *bind_col;
  void *blob_copy;

  blob_copy = malloc(blobsz);
  if(blob_copy == NULL){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    memcpy(blob_copy, blob, blobsz);

    mariadb_stmt = stmt->handle;
    bind_col = &mariadb_stmt->bind_out[col_idx];
    sqldbal_mariadb_stmt_bind_free_buf(bind_col);

    bind_col->buffer_type   = MYSQL_TYPE_BLOB;
    bind_col->buffer        = blob_copy;
    bind_col->buffer_length = blobsz;
    bind_col->length        = &bind_col->buffer_length;
    bind_col->is_null       = NULL;
    bind_col->is_unsigned   = 0;
    bind_col->error         = NULL;
  }
}

/**
 * Assign a 64-bit integer to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] i64     Integer to bind.
 */
static void
sqldbal_mariadb_stmt_bind_int64(struct sqldbal_stmt *const stmt,
                                size_t col_idx,
                                int64_t i64){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  MYSQL_BIND *bind_col;
  long long ll;
  long long *ill;

  if(si_int64_to_llong(i64, &ll)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    ill = malloc(sizeof(*ill));
    if(ill == NULL){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
    }
    else{
      *ill = ll;

      mariadb_stmt = stmt->handle;
      bind_col = &mariadb_stmt->bind_out[col_idx];
      sqldbal_mariadb_stmt_bind_free_buf(bind_col);

      bind_col->buffer_type   = MYSQL_TYPE_LONGLONG;
      bind_col->buffer        = ill;
      bind_col->buffer_length = sizeof(*ill);
      bind_col->length        = &bind_col->buffer_length;
      bind_col->is_null       = NULL;
      bind_col->is_unsigned   = 0;
      bind_col->error         = NULL;
    }
  }
}

/**
 * Assign a text string to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] s       Text string saved to a SQL text type.
 * @param[in] slen    Length of @p s in bytes.
 */
static void
sqldbal_mariadb_stmt_bind_text(struct sqldbal_stmt *const stmt,
                               size_t col_idx,
                               const char *const s,
                               size_t slen){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  MYSQL_BIND *bind_col;
  char *s_copy;

  s_copy = malloc(slen);
  if(s_copy == NULL){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    strcpy(s_copy, s);

    mariadb_stmt = stmt->handle;
    bind_col = &mariadb_stmt->bind_out[col_idx];
    sqldbal_mariadb_stmt_bind_free_buf(bind_col);

    bind_col->buffer_type   = MYSQL_TYPE_STRING;
    bind_col->buffer        = s_copy;
    bind_col->buffer_length = slen;
    bind_col->length        = &bind_col->buffer_length;
    bind_col->is_null       = NULL;
    bind_col->is_unsigned   = 0;
    bind_col->error         = NULL;
  }
}

/**
 * Assign a NULL value to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 */
static void
sqldbal_mariadb_stmt_bind_null(struct sqldbal_stmt *const stmt,
                               size_t col_idx){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  MYSQL_BIND *bind_col;

  mariadb_stmt = stmt->handle;
  bind_col = &mariadb_stmt->bind_out[col_idx];
  sqldbal_mariadb_stmt_bind_free_buf(bind_col);

  bind_col->buffer_type   = MYSQL_TYPE_NULL;
  bind_col->buffer        = NULL;
  bind_col->buffer_length = 0;
  bind_col->length        = NULL;
  bind_col->is_null       = NULL;
  bind_col->is_unsigned   = 0;
  bind_col->error         = NULL;
}

/**
 * Allocate memory for binding variables used when fetching data.
 *
 * @param[in] stmt     See @ref sqldbal_stmt.
 * @param[in] metadata Statement metadata from mysql_stmt_result_metadata().
 * @return See @ref sqldbal_status_code.
 */
static enum sqldbal_status_code
sqldbal_mariadb_stmt_allocate_bind_in_list(struct sqldbal_stmt *const stmt,
                                           MYSQL_RES *const metadata){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  size_t i;
  char *blob_buf;
  MYSQL_BIND *bind;
  MYSQL_FIELD *field_blob;
  unsigned int fieldnr;

  mariadb_stmt = stmt->handle;

  mariadb_stmt->bind_in_list = calloc(stmt->num_cols_result,
                                      sizeof(*mariadb_stmt->bind_in_list));

  mariadb_stmt->bind_in_length_list = calloc(
    stmt->num_cols_result,
    sizeof(*mariadb_stmt->bind_in_length_list));

  mariadb_stmt->bind_in_null_list = calloc(
    stmt->num_cols_result,
    sizeof(*mariadb_stmt->bind_in_null_list));

  if(mariadb_stmt->bind_in_list        == NULL ||
     mariadb_stmt->bind_in_length_list == NULL ||
     mariadb_stmt->bind_in_null_list   == NULL){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    for(i = 0; i < stmt->num_cols_result; i++){
      if(si_size_to_uint(i, &fieldnr)){
        sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
        break;
      }
      else{
        /* https://mariadb.com/kb/en/mysql_fetch_field_direct */
        field_blob = mysql_fetch_field_direct(metadata, fieldnr);
        blob_buf = malloc(field_blob->max_length);
        if(blob_buf == NULL){
          sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
          break;
        }
        else{
          bind = &mariadb_stmt->bind_in_list[i];
          bind->buffer_type   = MYSQL_TYPE_BLOB;
          bind->buffer        = blob_buf;
          bind->buffer_length = field_blob->max_length;
          bind->length        = &mariadb_stmt->bind_in_length_list[i];
          bind->error         = NULL;
          bind->is_null       = &mariadb_stmt->bind_in_null_list[i];
        }
      }
    }
  }
  return sqldbal_status_code_get(stmt->db);
}

/**
 * Execute a compiled statement with bound parameters.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_mariadb_stmt_execute(struct sqldbal_stmt *const stmt){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  char update_max_length;
  MYSQL_RES *metadata;

  mariadb_stmt = stmt->handle;

  update_max_length = 1;
  /* https://mariadb.com/kb/en/mysql_stmt_attr_set */
  /* https://mariadb.com/kb/en/mysql_stmt_bind_param   */
  /* https://mariadb.com/kb/en/mysql_stmt_execute      */
  /* https://mariadb.com/kb/en/mysql_stmt_store_result */
  if(mysql_stmt_attr_set(mariadb_stmt->stmt,
                         STMT_ATTR_UPDATE_MAX_LENGTH,
                         &update_max_length) ||
     mysql_stmt_bind_param  (mariadb_stmt->stmt, mariadb_stmt->bind_out) ||
     mysql_stmt_execute     (mariadb_stmt->stmt) ||
     mysql_stmt_store_result(mariadb_stmt->stmt)){
    sqldbal_mariadb_stmt_error(stmt, SQLDBAL_STATUS_EXEC);
  }
  else{
    if(stmt->num_cols_result){
      /* https://mariadb.com/kb/en/mysql_stmt_result_metadata */
      metadata = mysql_stmt_result_metadata(mariadb_stmt->stmt);

      if(metadata){
        if(sqldbal_mariadb_stmt_allocate_bind_in_list(
                                              stmt,
                                              metadata) == SQLDBAL_STATUS_OK){
          /* https://mariadb.com/kb/en/mysql_stmt_bind_result */
          if(mysql_stmt_bind_result(mariadb_stmt->stmt,
                                    mariadb_stmt->bind_in_list)){
            sqldbal_mariadb_stmt_error(stmt, SQLDBAL_STATUS_EXEC);
          }
        }
        mysql_free_result(metadata);
      }
      else{
        sqldbal_mariadb_stmt_error(stmt, SQLDBAL_STATUS_NOMEM);
      }
    }
  }
}

/**
 * Get the next row in the result set.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return See @ref sqldbal_fetch_result.
 */
static enum sqldbal_fetch_result
sqldbal_mariadb_stmt_fetch(struct sqldbal_stmt *const stmt){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  enum sqldbal_fetch_result fetch_result;

  mariadb_stmt = stmt->handle;

  /* https://mariadb.com/kb/en/mysql_stmt_fetch */
  switch(mysql_stmt_fetch(mariadb_stmt->stmt)){
    case 0:
      fetch_result = SQLDBAL_FETCH_ROW;
      break;
    case MYSQL_NO_DATA:
      fetch_result = SQLDBAL_FETCH_DONE;
      break;
    default:
      sqldbal_mariadb_stmt_error(stmt, SQLDBAL_STATUS_FETCH);
      fetch_result = SQLDBAL_FETCH_ERROR;
      break;
  }

  return fetch_result;
}

/**
 * Get the column result as blob/binary data.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] blob    Binary data.
 * @param[out] blobsz  Number of bytes in @p blob.
 */
static void
sqldbal_mariadb_stmt_column_blob(struct sqldbal_stmt *const stmt,
                                 size_t col_idx,
                                 const void **blob,
                                 size_t *blobsz){
  struct sqldbal_mariadb_stmt *mariadb_stmt;

  mariadb_stmt = stmt->handle;

  if(mariadb_stmt->bind_in_null_list[col_idx]){
    *blob   = NULL;
    *blobsz = 0;
  }
  else{
    *blob   = mariadb_stmt->bind_in_list[col_idx].buffer;
    *blobsz = mariadb_stmt->bind_in_length_list[col_idx];
  }
}

/**
 * Get the column result as a 64-bit integer.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] i64     64-bit integer.
 */
static void
sqldbal_mariadb_stmt_column_int64(struct sqldbal_stmt *const stmt,
                                  size_t col_idx,
                                  int64_t *i64){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  const char *buf;

  mariadb_stmt = stmt->handle;

  if(mariadb_stmt->bind_in_null_list[col_idx]){
    *i64 = 0;
  }
  else{
    buf = mariadb_stmt->bind_in_list[col_idx].buffer;
    sqldbal_strtoi64(stmt->db, buf, i64);
  }
}

/**
 * Get the column result as a string.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] text    Null-terminated string.
 * @param[out] textsz  Number of bytes in @p text.
 */
static void
sqldbal_mariadb_stmt_column_text(struct sqldbal_stmt *const stmt,
                                 size_t col_idx,
                                 const char **text,
                                 size_t *textsz){
  struct sqldbal_mariadb_stmt *mariadb_stmt;

  mariadb_stmt = stmt->handle;

  if(mariadb_stmt->bind_in_null_list[col_idx]){
    *text   = NULL;
    *textsz = 0;
  }
  else{
    *text   = mariadb_stmt->bind_in_list[col_idx].buffer;
    *textsz = mariadb_stmt->bind_in_length_list[col_idx] - 1;
  }
}

/**
 * Get the column data type.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @return See @ref sqldbal_column_type.
 */
static enum sqldbal_column_type
sqldbal_mariadb_stmt_column_type(struct sqldbal_stmt *const stmt,
                                 size_t col_idx){
  enum sqldbal_column_type type;
  struct sqldbal_mariadb_stmt *mariadb_stmt;

  mariadb_stmt = stmt->handle;
  if(mariadb_stmt->bind_in_null_list[col_idx]){
    type = SQLDBAL_TYPE_NULL;
  }
  else{
    type = SQLDBAL_TYPE_BLOB;
  }
  return type;
}

/**
 * Free a prepared statement resource.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_mariadb_stmt_close(struct sqldbal_stmt *const stmt){
  struct sqldbal_mariadb_stmt *mariadb_stmt;
  MYSQL_BIND *bind_col;
  size_t i;

  mariadb_stmt = stmt->handle;

  if(mariadb_stmt){
    for(i = 0; i < stmt->num_params; i++){
      bind_col = &mariadb_stmt->bind_out[i];
      sqldbal_mariadb_stmt_bind_free_buf(bind_col);
    }

    free(mariadb_stmt->bind_out);

    if(mariadb_stmt->bind_in_list){
      for(i = 0; i < stmt->num_cols_result; i++){
        free(mariadb_stmt->bind_in_list[i].buffer);
      }
      free(mariadb_stmt->bind_in_list);
    }
    free(mariadb_stmt->bind_in_length_list);
    free(mariadb_stmt->bind_in_null_list);

    /* https://mariadb.com/kb/en/mysql_stmt_close */
    if(mariadb_stmt->stmt){
      mysql_stmt_close(mariadb_stmt->stmt);
    }
    free(mariadb_stmt);
  }
}

#endif /* SQLDBAL_MARIADB */

#ifdef SQLDBAL_POSTGRESQL

#include <libpq-fe.h>
#include <pg_config.h>

#if PG_VERSION_NUM >= 90600
/**
 * PostgreSQL did not add the PQsetErrorContextVisibility function until
 * version 9.6.
 */
# define SQLDBAL_PQ_HAS_ERROR_CONTEXT_VISIBILITY
#endif /* PG_VERSION_NUM >= 90600 */

/**
 * Oid to data type mapping table.
 *
 * Each data type in PostgreSQL has a unique integer (oid). We need to
 * query the server to get these values.
 */
struct sqldbal_pq_oid_typname{
  /**
   * Unique data type id number.
   */
  Oid oid;

  /**
   * Data type.
   *
   * The returned typname will get cut off if larger than this buffer.
   * This value gets used internally and we only need to check a few
   * typnames.
   */
  char typname[48];
};

/**
 * Driver-specific database handle for PostgreSQL.
 */
struct sqldbal_pq_db{
  /**
   * PostgreSQL database connection object.
   */
  PGconn *db;

  /**
   * Increments when a new SQL statement gets prepared.
   */
  unsigned long stmt_counter;

  /**
   * Map oid values to data types.
   */
  struct sqldbal_pq_oid_typname *oid_typname_list;

  /**
   * Number of elements in @ref oid_typname_list.
   */
  size_t num_oid_typnames;
};

/**
 * Driver-specific compiled statement handle for PostgreSQL.
 */
struct sqldbal_pq_stmt{
  /**
   * Unique statement name in the current session.
   */
  char *name;

  /**
   * Parameter values to send with the query.
   */
  char **param_value_list;

  /**
   * Length of binary data in @ref param_value_list.
   */
  int *param_length_list;

  /**
   * Indicates if the corresponding parameter in @ref param_value_list has
   * a text (0) or binary (1) format.
   */
  int *param_format_list;

  /**
   * Stores the column values in the prepared statement results.
   */
  char **column_value_list;

  /**
   * Store the result of the prepared statement.
   */
  PGresult *exec_result;

  /**
   * Number of rows returned in @ref exec_result.
   */
  int exec_row_count;

  /**
   * Current row to fetch from @ref exec_result.
   */
  int fetch_row_index;
};

/**
 * Attributes for columns returned in @ref sqldbal_pq_exec.
 */
struct sqldbal_pq_col_attribute{
  /**
   * Indicates if memory allocated by library.
   */
  int mem_allocated;
};

/**
 * Convenience function that sets the status code and the error string
 * generated by the database function.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] status_code See @ref sqldbal_status_code.
 */
static void
sqldbal_pq_error(struct sqldbal_db *const db,
                 enum sqldbal_status_code status_code){
  const char *error;
  struct sqldbal_pq_db *pq_db;

  pq_db = db->handle;
  error = PQerrorMessage(pq_db->db);
  sqldbal_err_set(db, status_code, error);
}

/**
 * Copy a string and get the pointer to the end of the copied buffer.
 *
 * This function behaves similar to POSIX stpcpy(), useful for
 * concatenating multiple strings onto a buffer. It always adds a
 * null-terminated byte at the end of the string.
 *
 * @param[in] s1 Destination buffer.
 * @param[in] s2 Null-terminated source string to copy to @p s1.
 * @return Pointer to location in @p s1 after the last copied byte.
 */
SQLDBAL_LINKAGE char *
sqldbal_stpcpy(char *s1,
               const char *s2){
  size_t i;

  i = 0;
  do{
    s1[i] = s2[i];
  } while(s2[i++] != '\0');
  return &s1[i-1];
}

/**
 * Generate a unique statement name for use by PostgreSQL and store
 * it in @ref sqldbal_pq_stmt::name.
 *
 * @param[in] db      See @ref sqldbal_db.
 * @param[in] pq_stmt See @ref sqldbal_pq_stmt.
 * @return See @ref sqldbal_status_code.
 */
static enum sqldbal_status_code
sqldbal_pq_gen_stmt_name(struct sqldbal_db *const db,
                         struct sqldbal_pq_stmt *const pq_stmt){
  const char *const prefix_str = "pqs";
  struct sqldbal_pq_db *pq_db;
  size_t namesz;
  size_t int_count;
  size_t i;
  int name_len;

  pq_db = db->handle;

  /*
   * Get the number of characters in the counter.
   * For example, the longest 64-bit unsigned number:
   * 18446744073709551615
   * 12345678901234567890
   *         10        20
   * This means int_count has a maximum of 20 digits.
   */
  int_count = 1;
  for(i = pq_db->stmt_counter; i >= 10; i /= 10){
    int_count += 1;
  }

  /*
   * Does not wrap.
   * 3 + [1-20] + 1
   */
  namesz = strlen(prefix_str) + int_count + 1;
  pq_stmt->name = malloc(namesz);
  if(pq_stmt->name == NULL){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    name_len = sprintf(pq_stmt->name,
                       "%s%lu",
                       prefix_str,
                       pq_db->stmt_counter);
    if(name_len < 0){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
    }
    else{
      pq_db->stmt_counter += 1;
    }
  }

  return sqldbal_status_code_get(db);
}

/**
 * Get the length of key=value string to add to the conninfo parameter.
 *
 * The following shows an example key=value pair inside the quotes.
 * " key=value"
 *
 * Each key=value sequence gets preceded by a space.
 * See @ref sqldbal_pq_append_conninfo_param.
 *
 * @param[in]  key   Parameter key.
 * @param[in]  value Parameter value, NULL if this parameter does not exist.
 * @param[out] len   String length of key=value pair.
 * @retval 0 Length calculation did not wrap.
 * @retval 1 Length calculation wrapped.
 */
static int
sqldbal_pq_conninfo_param_len(const char *const key,
                              const char *const value,
                              size_t *const len){
  size_t key_len;
  size_t value_len;

  *len = 0;
  if(value){
    key_len = strlen(key);
    value_len = strlen(value);
    /*
     *        SPACE     key     =     value
     * len +=   1   + key_len + 1 + value_len;
     */
    if(si_add_size_t(key_len, value_len, len) ||
       si_add_size_t(*len, 2, len)){
      return 1;
    }
  }
  return 0;
}

/**
 * Append a conninfo paramater if the value exists.
 *
 * @param[in] conninfo Pointer to the first unwritten byte in conninfo string.
 * @param[in] key      Paramater key.
 * @param[in] value    Paramater value, NULL if this parameter does not exist.
 * @return Pointer to the next unwritten byte in @p conninfo.
 */
static char *
sqldbal_pq_append_conninfo_param(char *const conninfo,
                                 const char *const key,
                                 const char *const value){
  char *cip;

  cip = conninfo;
  if(value){
    cip = sqldbal_stpcpy(conninfo, " ");
    cip = sqldbal_stpcpy(cip, key);
    cip = sqldbal_stpcpy(cip, "=");
    cip = sqldbal_stpcpy(cip, value);
  }
  return cip;
}

/**
 * Generate the conninfo parameter to send to libpq when opening a
 * database connection.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] location    IP address or hostname.
 * @param[in] port        Server port number to connect to.
 * @param[in] username    Username to connect with.
 * @param[in] password    Password corresponding to @p username.
 * @param[in] database    Optional database to open after connecting to the
 *                        server. Set this to NULL to prevent opening a
 *                        database after initially connecting.
 * @param[in] num_options Number of entries in @p option_list.
 * @param[in] option_list See @ref sqldbal_driver_option.
 * @retval char* The conninfo parameter to use in PQconnectdb().
 *               The caller must free() this memory after use.
 * @retval NULL  Memory allocation failed. This function will also set the
 *               status in @ref sqldbal_db to @ref SQLDBAL_STATUS_NOMEM.
 */
static char *
sqldbal_pq_conninfo(struct sqldbal_db *const db,
                    const char *const location,
                    const char *const port,
                    const char *const username,
                    const char *const password,
                    const char *const database,
                    size_t num_options,
                    const struct sqldbal_driver_option *const option_list){
  size_t conninfo_sz;
  size_t inc_sz;
  char *conninfo;
  size_t i;
  const struct sqldbal_driver_option *option;
  char *cip;
  struct sqldbal_driver_option param_list[] = {
    {"host"                     , location}, /* 0  */
    {"hostaddr"                 , NULL    }, /* 1  */
    {"port"                     , port    }, /* 2  */
    {"dbname"                   , database}, /* 3  */
    {"user"                     , username}, /* 4  */
    {"password"                 , password}, /* 5  */
    {"passfile"                 , NULL    }, /* 6  */
    {"connect_timeout"          , NULL    }, /* 7  */
    {"client_encoding"          , NULL    }, /* 8  */
    {"options"                  , NULL    }, /* 9  */
    {"application_name"         , NULL    }, /* 10 */
    {"fallback_application_name", NULL    }, /* 11 */
    {"keepalives"               , NULL    }, /* 12 */
    {"keepalives_idle"          , NULL    }, /* 13 */
    {"keepalives_interval"      , NULL    }, /* 14 */
    {"keepalives_count"         , NULL    }, /* 15 */
    {"tty"                      , NULL    }, /* 16 */
    {"replication"              , NULL    }, /* 17 */
    {"sslmode"                  , NULL    }, /* 18 */
    {"requiressl"               , NULL    }, /* 19 */
    {"sslcompression"           , NULL    }, /* 20 */
    {"sslcert"                  , NULL    }, /* 21 */
    {"sslkey"                   , NULL    }, /* 22 */
    {"sslrootcert"              , NULL    }, /* 23 */
    {"sslcrl"                   , NULL    }, /* 24 */
    {"requirepeer"              , NULL    }, /* 25 */
    {"krbsrvname"               , NULL    }, /* 26 */
    {"gsslib"                   , NULL    }, /* 27 */
    {"service"                  , NULL    }, /* 28 */
    {"target_session_attrs"     , NULL    }  /* 29 */
  };
  const size_t num_possible_params = sizeof(param_list) / sizeof(param_list[0]);

  for(i = 0; i < num_options; i++){
    option = &option_list[i];
    if(strcmp(option->key, "CONNECT_TIMEOUT") == 0){
      param_list[7].value = option->value;
    }
    else if(strcmp(option->key, "TLS_MODE") == 0){
      param_list[18].value = option->value;
    }
    else if(strcmp(option->key, "TLS_CERT") == 0){
      param_list[21].value = option->value;
    }
    else if(strcmp(option->key, "TLS_KEY") == 0){
      param_list[22].value = option->value;
    }
    else if(strcmp(option->key, "TLS_CA") == 0){
      param_list[23].value = option->value;
    }
    else{
      sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
    }
  }

  conninfo_sz = 0;
  for(i = 0; i < num_possible_params; i++){
    if(sqldbal_pq_conninfo_param_len(param_list[i].key,
                                     param_list[i].value,
                                     &inc_sz) ||
       si_add_size_t(conninfo_sz, inc_sz, &conninfo_sz)){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
    }
  }
  if(si_add_size_t(conninfo_sz, 1, &conninfo_sz)){ /* null-terminator */
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }

  if(sqldbal_status_code_get(db) != SQLDBAL_STATUS_OK){
    conninfo = NULL;
  }
  else{
    conninfo = malloc(conninfo_sz);
    if(conninfo == NULL){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
    }
    else{
      conninfo[0] = '\0';

      cip = conninfo;
      for(i = 0; i < num_possible_params; i++){
        cip = sqldbal_pq_append_conninfo_param(cip,
                                               param_list[i].key,
                                               param_list[i].value);
      }
    }
  }

  return conninfo;
}

/**
 * Query the database to get the Oid values and corresponding typname.
 *
 * PostgreSQL has the PQftype function to get the column type of a PGresult.
 * However, we need to query the pg_type table to get the mapping of types.
 *
 * @param[in] db    See @ref sqldbal_db.
 * @param[in] pq_db See @ref sqldbal_pq_db.
 * @return See @ref sqldbal_status_code.
 */
static enum sqldbal_status_code
sqldbal_pq_query_oid_list(struct sqldbal_db *const db,
                          struct sqldbal_pq_db *const pq_db){
  PGresult *result_list;
  int nrows;
  size_t nrows_size_t;
  size_t size;
  int i;
  struct sqldbal_pq_oid_typname *oid_typname;
  const char *oid_str;
  Oid oid_conv;
  enum sqldbal_status_code status_code;

  const char *const sql =
  "SELECT oid, typname FROM pg_type ORDER BY oid ASC";

  status_code = SQLDBAL_STATUS_OK;
  result_list = PQexec(pq_db->db, sql);
  if(PQresultStatus(result_list) != PGRES_TUPLES_OK){
    status_code = SQLDBAL_STATUS_EXEC;
  }
  else{
    nrows = PQntuples(result_list);

    /* Number of rows limited to INT_MAX. */
    nrows_size_t = (size_t)nrows;

    size = sizeof(*pq_db->oid_typname_list);
    pq_db->oid_typname_list = sqldbal_reallocarray(NULL, nrows_size_t, size);
    if(pq_db->oid_typname_list == NULL){
      status_code = SQLDBAL_STATUS_NOMEM;
    }
    else{
      for(i = 0; i < nrows; i++){
        oid_typname = &pq_db->oid_typname_list[i];

        oid_str = PQgetvalue(result_list, i, 0);

        if(sqldbal_strtoui(db,
                           oid_str,
                           OID_MAX,
                           &oid_conv) != SQLDBAL_STATUS_OK){
          status_code = SQLDBAL_STATUS_COLUMN_COERCE;
          free(pq_db->oid_typname_list);
          break;
        }
        else{
          oid_typname->oid = oid_conv;
          oid_typname->typname[0] = '\0';
          strncat(oid_typname->typname,
                  PQgetvalue(result_list, i, 1),
                  sizeof(oid_typname->typname) - 1);
        }
      }
      pq_db->num_oid_typnames = nrows_size_t;
    }
    PQclear(result_list);
  }
  return status_code;
}

#ifdef SQLDBAL_TEST
/**
 * Get the PostgreSQL database structure, used by test suite.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return See @ref sqldbal_pq_db.
 */
SQLDBAL_LINKAGE const struct sqldbal_pq_db *
sqldbal_pq_get_pq_db_handle(const struct sqldbal_db *const db){
  return db->handle;
}
#endif /* SQLDBAL_TEST */

/**
 * Check if the oid corresponds to the given data type.
 *
 * Uses a linear search instead of bsearch because the only oid we
 * care about (bytea) exists close to the beginning of the list.
 *
 * @param[in] pq_db   See @ref sqldbal_pq_db.
 * @param[in] oid     Unique data type id in PostgreSQL.
 * @param[in] typname Name of column data type.
 * @retval 0 The @p oid corresponds to @p typname.
 * @retval 1 The @p oid does not correspond to @p typname, or it does not
 *           exist in the Oid list.
 */
SQLDBAL_LINKAGE int
sqldbal_pq_is_oid(const struct sqldbal_pq_db *const pq_db,
                  Oid oid,
                  const char *const typname){
  size_t i;

  for(i = 0; i < pq_db->num_oid_typnames; i++){
    if(oid == pq_db->oid_typname_list[i].oid){
      return strcmp(pq_db->oid_typname_list[i].typname, typname);
    }
  }
  return 1;
}

/**
 * Convert hexadecimal string sequence to binary data.
 *
 * The hex string must have a multiple of 2 characters.
 *
 * @param[in]  hex_str Hexadecimal string to convert to binary.
 * @param[out] binlen  Number of bytes in the returned binary data.
 * @retval char* Pointer to binary data converted from @p hex_str. This
 *               memory must get free'd by the caller.
 * @retval NULL  Error occurred.
 */
SQLDBAL_LINKAGE char *
sqldbal_str_hex2bin(const char *const hex_str,
                    size_t *const binlen){
  char *snew;
  size_t slen;
  size_t i;
  size_t j;
  unsigned long ulval;
  char hexbuf[3];
  char *ep;

  snew = NULL;
  *binlen = 0;

  slen = strlen(hex_str);

  if(slen % 2 != 1){
    snew = malloc(slen / 2 + 1);
    if(snew){
      snew[0] = '\0';
      *binlen = slen / 2;

      hexbuf[2] = '\0';
      for(i = 0, j = 0; hex_str[i]; i += 2, j++){
        hexbuf[0] = hex_str[i];
        hexbuf[1] = hex_str[i+1];
        ulval = strtoul(hexbuf, &ep, 16);
        if(*ep != 0){
          free(snew);
          snew = NULL;
          *binlen = 0;
          break;
        }
        /* ulval within range of char because hexbuf has only 2 hex chars. */
        snew[j] = (char)ulval;
      }
    }
  }

  return snew;
}

/**
 * Open database connection to server.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] location    IP address or hostname.
 * @param[in] port        Server port number to connect to.
 * @param[in] username    Username to connect with.
 * @param[in] password    Password corresponding to @p username.
 * @param[in] database    Optional database to open after connecting to the
 *                        server. Set this to NULL to prevent opening a
 *                        database after initially connecting.
 * @param[in] option_list See @ref sqldbal_driver_option.
 * @param[in] num_options Number of entries in @p option_list.
 */
static void
sqldbal_pq_open(struct sqldbal_db *const db,
                const char *const location,
                const char *const port,
                const char *const username,
                const char *const password,
                const char *const database,
                const struct sqldbal_driver_option *const option_list,
                size_t num_options){
  struct sqldbal_pq_db *pq_db;
  char *conninfo;
  enum sqldbal_status_code status_code;

  pq_db = malloc(sizeof(*pq_db));
  if(pq_db == NULL){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    db->handle = NULL;
    pq_db->stmt_counter = 1;

    conninfo = sqldbal_pq_conninfo(db,
                                   location,
                                   port,
                                   username,
                                   password,
                                   database,
                                   num_options,
                                   option_list);
    if(conninfo == NULL){
      free(pq_db);
    }
    else{
      pq_db->db = PQconnectdb(conninfo);
      if(pq_db->db == NULL){
        sqldbal_status_code_set(db, SQLDBAL_STATUS_OPEN);
        free(pq_db);
      }
      else{
        if(PQstatus(pq_db->db) != CONNECTION_OK){
          sqldbal_status_code_set(db, SQLDBAL_STATUS_OPEN);
          PQfinish(pq_db->db);
          free(pq_db);
        }
        else{
          if(db->flags & SQLDBAL_FLAG_DEBUG){
            PQsetErrorVerbosity(pq_db->db, PQERRORS_VERBOSE);
#ifdef SQLDBAL_PQ_HAS_ERROR_CONTEXT_VISIBILITY
            PQsetErrorContextVisibility(pq_db->db, PQSHOW_CONTEXT_ALWAYS);
#endif /* SQLDBAL_PQ_HAS_ERROR_CONTEXT_VISIBILITY */
            PQtrace(pq_db->db, stderr);
          }

          status_code = sqldbal_pq_query_oid_list(db, pq_db);
          if(status_code != SQLDBAL_STATUS_OK){
            PQfinish(pq_db->db);
            free(pq_db);
            sqldbal_status_code_set(db, status_code);
          }
          else{
            db->handle = pq_db;
          }
        }
      }
      free(conninfo);
    }
  }
}

/**
 * Close database connection.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_pq_close(struct sqldbal_db *const db){
  struct sqldbal_pq_db *pq_db;

  pq_db = db->handle;
  if(pq_db){
    PQfinish(pq_db->db);
    free(pq_db->oid_typname_list);
    free(pq_db);
  }
}

/**
 * Get the PostgreSQL database handle.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return PGconn*.
 */
static void *
sqldbal_pq_db_handle(const struct sqldbal_db *const db){
  struct sqldbal_pq_db *pq_db;

  pq_db = db->handle;
  return pq_db->db;
}

/**
 * Get the PostgreSQL statement handle.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return char* (statement name).
 */
static void *
sqldbal_pq_stmt_handle(const struct sqldbal_stmt *const stmt){
  struct sqldbal_pq_stmt *pq_stmt;

  pq_stmt = stmt->handle;
  return pq_stmt->name;
}

/**
 * Directly execute SQL statement that does not return any data.
 *
 * @param[in] db  See @ref sqldbal_db.
 * @param[in] sql Database statement to execute.
 */
static void
sqldbal_pq_exec_noresult(struct sqldbal_db *const db,
                         const char *const sql){
  struct sqldbal_pq_db *pq_db;
  PGresult *result;

  pq_db = db->handle;
  result = PQexec(pq_db->db, sql);
  if(result == NULL){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    if(PQresultStatus(result) != PGRES_COMMAND_OK){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_EXEC);
    }
    PQclear(result);
  }
}

/**
 * Begin a PostgreSQL transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_pq_begin_transaction(struct sqldbal_db *const db){
  sqldbal_pq_exec_noresult(db, "BEGIN");
}

/**
 * Commit a PostgreSQL transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_pq_commit(struct sqldbal_db *const db){
  sqldbal_pq_exec_noresult(db, "COMMIT");
}

/**
 * Rollback a PostgreSQL transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_pq_rollback(struct sqldbal_db *const db){
  sqldbal_pq_exec_noresult(db, "ROLLBACK");
}

/**
 * Execute a direct SQL statement.
 *
 * @param[in] db        See @ref sqldbal_db.
 * @param[in] sql       SQL command to execute.
 * @param[in] callback  Invokes this callback function for every returned row
 *                      in the query result set. Set this to NULL if query
 *                      results not needed.
 * @param[in] user_data Pass this to the first argument in the
 *                      @p callback function.
 */
static void
sqldbal_pq_exec(struct sqldbal_db *const db,
                const char *const sql,
                sqldbal_exec_callback_fp callback,
                void *user_data){
  struct sqldbal_pq_db *pq_db;
  PGresult *result;
  ExecStatusType result_status;
  int pq_nfields;
  size_t num_cols;
  int num_rows;
  int row_i;
  size_t col_i;
  int pq_column_number_i;
  char *col_value;
  int pq_length;
  size_t col_length;
  int mem_allocated;
  size_t binlen;
  char **col_result_list;
  size_t *col_length_list;
  struct sqldbal_pq_col_attribute *col_attribute_list;
  Oid data_type_id;

  pq_db = db->handle;
  result = PQexecParams(pq_db->db, sql, 0, NULL, NULL, NULL, NULL, 0);
  if(result == NULL){
    sqldbal_pq_error(db, SQLDBAL_STATUS_EXEC);
  }
  else{
    result_status = PQresultStatus(result);
    if(result_status == PGRES_COMMAND_OK){
      /* Insert/update does not have any rows returned to the caller. */
    }
    else if(result_status == PGRES_TUPLES_OK){
      if(callback){
        pq_nfields = PQnfields(result);
        num_rows = PQntuples(result);

        if(si_int_to_size(pq_nfields, &num_cols)){
          sqldbal_pq_error(db, SQLDBAL_STATUS_NOMEM);
        }
        else{
          col_result_list = sqldbal_reallocarray(NULL,
                                                 num_cols,
                                                 sizeof(*col_result_list));
          col_length_list = sqldbal_reallocarray(NULL,
                                                 num_cols,
                                                 sizeof(*col_length_list));
          col_attribute_list = sqldbal_reallocarray(
                                 NULL,
                                 num_cols,
                                 sizeof(*col_attribute_list));
          if(col_result_list == NULL ||
             col_length_list == NULL ||
             col_attribute_list == NULL){
            sqldbal_pq_error(db, SQLDBAL_STATUS_NOMEM);
          }
          else{
            for(row_i = 0; row_i < num_rows; row_i++){
              for(col_i = 0; col_i < num_cols; col_i++){
                col_attribute_list[col_i].mem_allocated = 0;
                if(si_size_to_int(col_i, &pq_column_number_i)){
                  sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
                }
                else{
                  pq_length = PQgetlength(result, row_i, pq_column_number_i);
                  if(si_int_to_size(pq_length, &col_length)){
                    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
                  }
                  else{
                    mem_allocated = 0;
                    if(PQgetisnull(result, row_i, pq_column_number_i)){
                      col_value = NULL;
                    }
                    else{
                      col_value = PQgetvalue(result, row_i, pq_column_number_i);
                      data_type_id = PQftype(result, pq_column_number_i);
                      if(sqldbal_pq_is_oid(pq_db, data_type_id, "bytea") == 0){
                        /*
                         * The bytea type should begin with "\\x", so
                         * start the conversion at the 3rd character.
                         */
                        col_value = sqldbal_str_hex2bin(&col_value[2],
                                                        &binlen);
                        if(col_value){
                          col_length = binlen;
                          mem_allocated = 1;
                        }
                        else{
                          col_length = 0;
                          sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
                        }
                      }
                    }
                    col_result_list[col_i] = col_value;
                    col_length_list[col_i] = col_length;
                    col_attribute_list[col_i].mem_allocated = mem_allocated;
                  }
                }
              }
              if(sqldbal_status_code_get(db) == SQLDBAL_STATUS_OK){
                if(callback(user_data,
                            num_cols,
                            col_result_list,
                            col_length_list)){
                  sqldbal_status_code_set(db, SQLDBAL_STATUS_EXEC);
                }
              }
              for(col_i = 0; col_i < num_cols; col_i++){
                if(col_attribute_list[col_i].mem_allocated){
                  free(col_result_list[col_i]);
                }
              }
            }
          }
          free(col_result_list);
          free(col_length_list);
          free(col_attribute_list);
        }
      }
    }
    else{
      sqldbal_pq_error(db, SQLDBAL_STATUS_EXEC);
    }
    PQclear(result);
  }
}

/**
 * Get the insert id from the last SQL insert statement.
 *
 * @param[in]  db        See @ref sqldbal_db.
 * @param[in]  name      Sequence name.
 * @param[out] insert_id Last insert id.
 */
static void
sqldbal_pq_last_insert_id(struct sqldbal_db *const db,
                          const char *const name,
                          uint64_t *insert_id){
  const char *const sql = "SELECT currval($1)";
  struct sqldbal_stmt *stmt;
  int64_t i64_insert_id;

  /*
   * Static analyzer complains that this unset when calling
   * @ref si_int64_to_uint64.
   */
  i64_insert_id = 0;

  sqldbal_stmt_prepare(db, sql, (size_t)-1, &stmt);
  sqldbal_stmt_bind_text(stmt, 0, name, (size_t)-1);
  sqldbal_stmt_execute(stmt);
  sqldbal_stmt_fetch(stmt);
  sqldbal_stmt_column_int64(stmt, 0, &i64_insert_id);
  if(sqldbal_status_code_get(db) == SQLDBAL_STATUS_OK){
    if(si_int64_to_uint64(i64_insert_id, insert_id)){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_COLUMN_COERCE);
    }
  }
  sqldbal_stmt_close(stmt);
}

/**
 * Preallocate memory used to bind parameters.
 *
 * @param[in] db      See @ref sqldbal_db.
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] pq_stmt See @ref sqldbal_pq_stmt.
 * @retval  0 Success.
 * @retval -1 Error occurred.
 */
static int
sqldbal_pq_stmt_allocate_param_list(struct sqldbal_db *const db,
                                    struct sqldbal_stmt *const stmt,
                                    struct sqldbal_pq_stmt *const pq_stmt){
  struct sqldbal_pq_db *pq_db;
  PGresult *stmt_describe;
  int pq_nparams;
  int rc;

  rc = 0;
  pq_db = db->handle;
  stmt_describe = PQdescribePrepared(pq_db->db, pq_stmt->name);
  if(PQresultStatus(stmt_describe) != PGRES_COMMAND_OK){
    rc = -1;
    sqldbal_status_code_set(db, SQLDBAL_STATUS_PREPARE);
  }
  else{
    pq_nparams = PQnparams(stmt_describe);
    if(si_int_to_size(pq_nparams, &stmt->num_params)){
      sqldbal_status_code_set(db, SQLDBAL_STATUS_OVERFLOW);
    }
    else{
      pq_stmt->param_value_list = calloc(stmt->num_params,
                                         sizeof(*pq_stmt->param_value_list));
      pq_stmt->param_length_list = sqldbal_reallocarray(NULL,
                                                        stmt->num_params,
                                          sizeof(*pq_stmt->param_length_list));
      pq_stmt->param_format_list = sqldbal_reallocarray(
                                     NULL,
                                     stmt->num_params,
                                     sizeof(*pq_stmt->param_format_list));
      if(pq_stmt->param_value_list == NULL  ||
         pq_stmt->param_length_list == NULL ||
         pq_stmt->param_format_list == NULL){
        rc = -1;
        sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
        free(pq_stmt->param_value_list);
        free(pq_stmt->param_length_list);
        free(pq_stmt->param_format_list);
      }
    }
  }
  PQclear(stmt_describe);
  return rc;
}

/**
 * Compile statement in PostgreSQL.
 *
 * @param[in] db      See @ref sqldbal_db.
 * @param[in] sql     SQL statement to compile.
 * @param[in] sql_len Unused.
 * @param[in] stmt    See @ref sqldbal_stmt.
 */
static void
sqldbal_pq_stmt_prepare(struct sqldbal_db *const db,
                        const char *const sql,
                        size_t sql_len,
                        struct sqldbal_stmt *stmt){
  struct sqldbal_pq_db *pq_db;
  struct sqldbal_pq_stmt *pq_stmt;
  PGresult *stmt_result;

  (void)sql_len;

  pq_db = db->handle;

  pq_stmt = malloc(sizeof(*pq_stmt));
  if(pq_stmt == NULL){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    stmt->handle = NULL;

    pq_stmt->param_value_list  = NULL;
    pq_stmt->param_length_list = NULL;
    pq_stmt->param_format_list = NULL;
    pq_stmt->exec_result       = NULL;
    pq_stmt->column_value_list = NULL;

    if(sqldbal_pq_gen_stmt_name(db, pq_stmt) == SQLDBAL_STATUS_OK){
      stmt_result = PQprepare(pq_db->db, pq_stmt->name, sql, 0, NULL);
      if(stmt_result == NULL){
        sqldbal_pq_error(db, SQLDBAL_STATUS_PREPARE);
        free(pq_stmt->name);
        free(pq_stmt);
      }
      else{
        if(PQresultStatus(stmt_result) != PGRES_COMMAND_OK){
          sqldbal_status_code_set(db, SQLDBAL_STATUS_PREPARE);
          sqldbal_errstr_set(db, PQresultErrorMessage(stmt_result));
          free(pq_stmt->name);
          free(pq_stmt);
        }
        else if(sqldbal_pq_stmt_allocate_param_list(db, stmt, pq_stmt) < 0){
          free(pq_stmt->name);
          free(pq_stmt);
        }
        else{
          stmt->handle = pq_stmt;
        }
        PQclear(stmt_result);
      }
    }
    else{
      free(pq_stmt->name);
      free(pq_stmt);
    }
  }
}

/**
 * Assign binary data to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] blob    Binary data saved to BLOB or BYTEA types.
 * @param[in] blobsz  Length of @p blob in bytes.
 */
static void
sqldbal_pq_stmt_bind_blob(struct sqldbal_stmt *const stmt,
                          size_t col_idx,
                          const void *const blob,
                          size_t blobsz){
  char *blob_copy;
  struct sqldbal_pq_stmt *pq_stmt;
  int blobsz_i;

  if(si_size_to_int(blobsz, &blobsz_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    blob_copy = malloc(blobsz);
    if(blob_copy == NULL){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
    }
    else{
      pq_stmt = stmt->handle;
      memcpy(blob_copy, blob, blobsz);
      if(pq_stmt->param_value_list[col_idx]){
        free(pq_stmt->param_value_list[col_idx]);
        pq_stmt->param_value_list[col_idx] = NULL;
      }
      pq_stmt->param_value_list [col_idx] = blob_copy;
      pq_stmt->param_length_list[col_idx] = blobsz_i;
      pq_stmt->param_format_list[col_idx] = 1;
    }
  }
}

/**
 * Assign a 64-bit integer to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] i64     Integer to bind.
 */
static void
sqldbal_pq_stmt_bind_int64(struct sqldbal_stmt *const stmt,
                           size_t col_idx,
                           int64_t i64){
  char i64_str[MAX_I64_STR_SZ];
  int slen;
  size_t slen_sz;
  struct sqldbal_pq_stmt *pq_stmt;

  pq_stmt = stmt->handle;

  slen = sprintf(i64_str, "%" PRIi64, i64);
  if(slen < 0){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_BIND);
  }
  else{
    slen_sz = (size_t)slen;
    slen_sz += 1;
    i64_str[slen_sz] = '\0';
    sqldbal_pq_stmt_bind_blob(stmt, col_idx, i64_str, slen_sz);
    pq_stmt->param_format_list[col_idx] = 0;
  }
}

/**
 * Assign a text string to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] s       Text string saved to a SQL text type.
 * @param[in] slen    Length of @p s in bytes.
 */
static void
sqldbal_pq_stmt_bind_text(struct sqldbal_stmt *const stmt,
                          size_t col_idx,
                          const char *const s,
                          size_t slen){
  struct sqldbal_pq_stmt *pq_stmt;

  pq_stmt = stmt->handle;
  sqldbal_pq_stmt_bind_blob(stmt, col_idx, s, slen);
  pq_stmt->param_format_list[col_idx] = 0;
}

/**
 * Assign a NULL value to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 */
static void
sqldbal_pq_stmt_bind_null(struct sqldbal_stmt *const stmt,
                          size_t col_idx){
  struct sqldbal_pq_stmt *pq_stmt;

  pq_stmt = stmt->handle;
  pq_stmt->param_value_list [col_idx] = NULL;
  pq_stmt->param_length_list[col_idx] = 0;
  pq_stmt->param_format_list[col_idx] = 0;
}

/**
 * Execute a compiled statement with bound parameters.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_pq_stmt_execute(struct sqldbal_stmt *const stmt){
  struct sqldbal_pq_db *pq_db;
  struct sqldbal_pq_stmt *pq_stmt;
  int pq_num_param_list;
  const char *const *const_param_value_list;
  ExecStatusType pq_status;
  int pq_nfields;

  pq_db = stmt->db->handle;
  pq_stmt = stmt->handle;

  if(pq_stmt->exec_result){
    free(pq_stmt->exec_result);
    pq_stmt->exec_result = NULL;
  }

  /*
   * Already compatible with int because of conversion in
   * @ref sqldbal_pq_stmt_allocate_param_list.
   */
  pq_num_param_list = (int)stmt->num_params;

  const_param_value_list = (const char *const *)pq_stmt->param_value_list;
  pq_stmt->exec_result = PQexecPrepared(pq_db->db,
                                        pq_stmt->name,
                                        pq_num_param_list,
                                        const_param_value_list,
                                        pq_stmt->param_length_list,
                                        pq_stmt->param_format_list,
                                        0);
  pq_status = PQresultStatus(pq_stmt->exec_result);
  if(pq_status != PGRES_COMMAND_OK && pq_status != PGRES_TUPLES_OK){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_EXEC);
    sqldbal_errstr_set(stmt->db, PQresultErrorMessage(pq_stmt->exec_result));
  }
  else{
    pq_stmt->exec_row_count = PQntuples(pq_stmt->exec_result);
    pq_nfields = PQnfields(pq_stmt->exec_result);
    if(si_int_to_size(pq_nfields, &stmt->num_cols_result)){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
    }
    else if(stmt->num_cols_result){
      pq_stmt->column_value_list = calloc(stmt->num_cols_result,
                                          sizeof(*pq_stmt->column_value_list));
      if(pq_stmt->column_value_list == NULL){
        sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
      }
    }
  }
  pq_stmt->fetch_row_index = 0;
}

/**
 * Free the column values held in the @ref sqldbal_pq_stmt::column_value_list.
 *
 * This does not free the actual list. It will eventually get free'd in
 * @ref sqldbal_pq_stmt_close
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_pq_stmt_free_column_values(struct sqldbal_stmt *const stmt){
  struct sqldbal_pq_stmt *pq_stmt;
  size_t i;

  pq_stmt = stmt->handle;
  if(pq_stmt->column_value_list){
    for(i = 0; i < stmt->num_cols_result; i++){
      free(pq_stmt->column_value_list[i]);
      pq_stmt->column_value_list[i] = NULL;
    }
  }
}

/**
 * Get the next row in the result set.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return See @ref sqldbal_fetch_result.
 */
static enum sqldbal_fetch_result
sqldbal_pq_stmt_fetch(struct sqldbal_stmt *const stmt){
  enum sqldbal_fetch_result fetch_result;
  struct sqldbal_pq_stmt *pq_stmt;

  sqldbal_pq_stmt_free_column_values(stmt);

  pq_stmt = stmt->handle;
  if(pq_stmt->fetch_row_index >= pq_stmt->exec_row_count){
    fetch_result = SQLDBAL_FETCH_DONE;
  }
  else{
    pq_stmt->fetch_row_index += 1;
    fetch_result = SQLDBAL_FETCH_ROW;
  }
  return fetch_result;
}

/**
 * Get the column result as blob/binary data.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] blob    Binary data.
 * @param[out] blobsz  Number of bytes in @p blob.
 */
static void
sqldbal_pq_stmt_column_blob(struct sqldbal_stmt *const stmt,
                            size_t col_idx,
                            const void **blob,
                            size_t *blobsz){
  struct sqldbal_pq_stmt *pq_stmt;
  int row_number;
  int col_no_i;
  const char *blob_offset;
  char *hex2bin;
  int pq_length;

  pq_stmt = stmt->handle;
  row_number = pq_stmt->fetch_row_index - 1;

  if(si_size_to_int(col_idx, &col_no_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
    *blob = NULL;
  }
  else{
    *blob = PQgetvalue(pq_stmt->exec_result,
                       row_number,
                       col_no_i);
    pq_length = PQgetlength(pq_stmt->exec_result,
                            row_number,
                            col_no_i);
    if(si_int_to_size(pq_length, blobsz)){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
    }
    else if(*blobsz == 0 &&
       PQgetisnull(pq_stmt->exec_result, row_number, col_no_i)){
      *blob = NULL;
    }
    else if(strncmp(*blob, "\\x", 2) == 0){
      blob_offset = *blob;
      blob_offset += 2;

      hex2bin = sqldbal_str_hex2bin(blob_offset, blobsz);
      if(hex2bin == NULL){
        sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
      }
      *blob = hex2bin;
      pq_stmt->column_value_list[col_idx] = hex2bin;
    }
  }
}

/**
 * Get the column result as a string.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] text    Null-terminated string.
 * @param[out] textsz  Number of bytes in @p text.
 */
static void
sqldbal_pq_stmt_column_text(struct sqldbal_stmt *const stmt,
                            size_t col_idx,
                            const char **text,
                            size_t *textsz){
  const void *blob;

  sqldbal_pq_stmt_column_blob(stmt, col_idx, &blob, textsz);
  *text = blob;
}

/**
 * Get the column data type.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @return See @ref sqldbal_column_type.
 */
static enum sqldbal_column_type
sqldbal_pq_stmt_column_type(struct sqldbal_stmt *const stmt,
                            size_t col_idx){
  enum sqldbal_column_type type;
  struct sqldbal_pq_stmt *pq_stmt;
  int col_no_i;
  int row_number;

  if(si_size_to_int(col_idx, &col_no_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
    type = SQLDBAL_TYPE_ERROR;
  }
  else{
    pq_stmt = stmt->handle;
    row_number = pq_stmt->fetch_row_index - 1;
    if(PQgetisnull(pq_stmt->exec_result, row_number, col_no_i)){
      type = SQLDBAL_TYPE_NULL;
    }
    else{
      type = SQLDBAL_TYPE_BLOB;
    }
  }
  return type;
}

/**
 * Get the column result as a 64-bit integer.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] i64     64-bit integer.
 */
static void
sqldbal_pq_stmt_column_int64(struct sqldbal_stmt *const stmt,
                             size_t col_idx,
                             int64_t *i64){
  const char *text;
  size_t textsz;

  sqldbal_pq_stmt_column_text(stmt, col_idx, &text, &textsz);

  if(text){
    sqldbal_strtoi64(stmt->db, text, i64);
  }
  else{
    *i64 = 0;
  }
}

/**
 * Delete a pq prepared statement.
 *
 * pq does not have an explicit function for closing a prepared statement.
 * Instead, we have to use the DEALLOCATE SQL statement with the name of
 * the prepared statement.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_pq_stmt_close(struct sqldbal_stmt *const stmt){
  const char *const DEALLOCATE_PREFIX_STR = "DEALLOCATE ";
  /*
   * Allocate the SQL buffer which must hold the longest statement id.
   *
   * For example, the longest 64-bit unsigned number:
   * 18446744073709551615
   * 12345678901234567890
   *         10        20
   * This means int_count has a maximum of 20 digits.
   *                11               +        20         +       1
   *   strlen(DEALLOCATE_PREFIX_STR) + max 64-bit length + null-terminator
   */
  char sql[11 + 20 + 1];
  struct sqldbal_pq_stmt *pq_stmt;
  size_t i;

  pq_stmt = stmt->handle;

  if(pq_stmt){
    PQclear(pq_stmt->exec_result);

    strcpy(sql, DEALLOCATE_PREFIX_STR);
    strcat(sql, pq_stmt->name);
    sqldbal_exec(stmt->db, sql, NULL, NULL);

    if(pq_stmt->param_value_list){
      for(i = 0; i < stmt->num_params; i++){
        free(pq_stmt->param_value_list[i]);
      }
      free(pq_stmt->param_value_list);
    }

    if(pq_stmt->param_length_list){
      free(pq_stmt->param_length_list);
    }

    if(pq_stmt->param_format_list){
      free(pq_stmt->param_format_list);
    }

    sqldbal_pq_stmt_free_column_values(stmt);

    free(pq_stmt->column_value_list);

    free(pq_stmt->name);

    free(pq_stmt);
  }
}

#endif /* SQLDBAL_POSTGRESQL */

#ifdef SQLDBAL_SQLITE

#include <sqlite3.h>

#if SQLITE_VERSION_NUMBER >= 3014000
/**
 * SQLite has the sqlite3_trace_v2 api since version 3.14.0.
 */
# define SQLDBAL_SQLITE_HAS_TRACE_V2
#endif /* SQLITE_VERSION_NUMBER >= 3014000 */

/**
 * Maximum number of retries to do when SQLITE_BUSY gets returned.
 */
#define SQLDBAL_SQLITE_MAX_NUM_RETRIES 10

/**
 * This data gets passed to the sqite3_exec function which allows the SQLite
 * callback function to invoke the SQLDBAL callback function.
 */
struct sqldbal_sqlite_cb_data{
  /**
   * See @ref sqldbal_db.
   */
  struct sqldbal_db *db;

  /**
   * Application callback function which sends rows from a SELECT query.
   */
  sqldbal_exec_callback_fp callback;

  /**
   * This gets passed to the application-provided callback function
   * in @ref callback.
   */
  void *user_data;
};

/**
 * Convenience function that sets the status code and the error string
 * generated by the database function.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] sqlite_rc   Return code from SQLite function call. Only used if
 *                        the sqlite3* database handle has not been created.
 * @param[in] status_code See @ref sqldbal_status_code.
 */
static void
sqldbal_sqlite_error(struct sqldbal_db *const db,
                     int sqlite_rc,
                     enum sqldbal_status_code status_code){
  sqlite3 *sqlite_db;
  const char *errstr;

  sqlite_db = db->handle;

  /* https://www.sqlite.org/c3ref/errcode.html */
  if(sqlite_db){
    errstr = sqlite3_errmsg(sqlite_db);
  }
  else{
    errstr = sqlite3_errstr(sqlite_rc);
  }
  sqldbal_err_set(db, status_code, errstr);
}

#ifdef SQLDBAL_SQLITE_HAS_TRACE_V2
/**
 * Trace mask used in sqlite3_trace_v2.
 */
#define SQLDBAL_SQLITE_TRACE_ALL SQLITE_TRACE_STMT    | \
                                 SQLITE_TRACE_PROFILE | \
                                 SQLITE_TRACE_ROW     | \
                                 SQLITE_TRACE_CLOSE

/**
 * Print trace information to stderr.
 *
 * @param[in] mask    One of the values in @ref SQLDBAL_SQLITE_TRACE_ALL.
 * @param[in] context User parameter provided to the callback - unused.
 * @param[in] P       SQLite prepared statement or database handle, depending
 *                    on the value of @p mask.
 * @param[in] X       Parameter type set depending on value of @p mask.
 * @retval 0 Return value currently unused.
 */
SQLDBAL_LINKAGE int
sqldbal_sqlite_trace_hook(unsigned mask,
                          void *context,
                          void *P,
                          void *X){
  const char *trace_stmt;
  int64_t i64;
  long l;
  const char *more_than;

  (void)context;
  (void)P;

  switch(mask){
    case SQLITE_TRACE_STMT:
      trace_stmt = X;
      fprintf(stderr, "SQLITE_TRACE_STMT: %s\n", trace_stmt);
      break;
    case SQLITE_TRACE_PROFILE:
      i64 = *(int64_t*)X;
      l = i64;
      if(i64 > LONG_MAX){
        more_than = "more than ";
      }
      else{
        more_than = "";
      }
      fprintf(stderr,
              "SQLITE_TRACE_PROFILE: query took %s%ld us\n",
              more_than,
              l);
      break;
    case SQLITE_TRACE_ROW:
      fputs("SQLITE_TRACE_ROW: statement generated new row\n", stderr);
      break;
    case SQLITE_TRACE_CLOSE:
      fputs("SQLITE_TRACE_CLOSE: database connection closed\n", stderr);
      break;
    default:
      fputs("SQLITE_TRACE_UNKNOWN: unknown trace mask\n", stderr);
      break;
  }

  return 0;
}
#endif /* SQLDBAL_SQLITE_HAS_TRACE_V2 */

/**
 * Open a SQLite database file.
 *
 * @param[in] db          See @ref sqldbal_db.
 * @param[in] location    Path to database file.
 * @param[in] port        Unused.
 * @param[in] username    Unused.
 * @param[in] password    Unused.
 * @param[in] database    Unused.
 * @param[in] option_list Driver-specific options to pass to SQLite.
 * @param[in] num_options Number of entries in @p option_list.
 */
static void
sqldbal_sqlite_open(struct sqldbal_db *const db,
                    const char *const location,
                    const char *const port,
                    const char *const username,
                    const char *const password,
                    const char *const database,
                    const struct sqldbal_driver_option *const option_list,
                    size_t num_options){
  sqlite3 *sqlite_db;
  const struct sqldbal_driver_option *option;
  size_t i;
  int flags;
  int sqlite_err;
  const char *vfs;

  (void)port;
  (void)username;
  (void)password;
  (void)database;

  flags = 0;
  vfs = NULL;

  for(i = 0; i < num_options; i++){
    option = &option_list[i];
    if(strcmp(option->key, "VFS") == 0){
      vfs = option->value;
    }
    else{
      sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
    }
  }

  if(sqldbal_status_code_get(db) == SQLDBAL_STATUS_OK){
    if(db->flags & SQLDBAL_FLAG_SQLITE_OPEN_READONLY){
      flags |= SQLITE_OPEN_READONLY;
    }

    if(db->flags & SQLDBAL_FLAG_SQLITE_OPEN_READWRITE){
      flags |= SQLITE_OPEN_READWRITE;
    }

    if(db->flags & SQLDBAL_FLAG_SQLITE_OPEN_CREATE){
      flags |= SQLITE_OPEN_CREATE;
    }

    if(flags == 0){
      flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    }

    /* https://www.sqlite.org/c3ref/open.html */
    sqlite_err = sqlite3_open_v2(location, &sqlite_db, flags, vfs);
    if(sqlite_err != SQLITE_OK){
      sqldbal_sqlite_error(db, sqlite_err, SQLDBAL_STATUS_OPEN);
    }
    else{
      if(db->flags & SQLDBAL_FLAG_DEBUG){
#ifdef SQLDBAL_SQLITE_HAS_TRACE_V2
        /* https://www.sqlite.org/c3ref/trace_v2.html */
        if(sqlite3_trace_v2(sqlite_db,
                            SQLDBAL_SQLITE_TRACE_ALL,
                            sqldbal_sqlite_trace_hook,
                            NULL) != SQLITE_OK){
          sqldbal_sqlite_error(db, 0, SQLDBAL_STATUS_OPEN);
        }
#endif /* SQLDBAL_SQLITE_HAS_TRACE_V2 */
      }
    }

    db->handle = sqlite_db;
  }
}

/**
 * Close SQLite database handle.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_sqlite_close(struct sqldbal_db *const db){
  sqlite3 *sqlite_db;

  sqlite_db = db->handle;

  /* https://www.sqlite.org/c3ref/close.html */
  if(sqlite3_close_v2(sqlite_db) != SQLITE_OK){
    sqldbal_sqlite_error(db, 0, SQLDBAL_STATUS_CLOSE);
  }
}

/**
 * Get the SQLite3 database handle.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return SQLite3 database handle.
 */
static void *
sqldbal_sqlite_db_handle(const struct sqldbal_db *const db){
  sqlite3 *sqlite_db;

  sqlite_db = db->handle;
  return sqlite_db;
}

/**
 * Get the SQLite3 statement handle.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return SQLite statement handle.
 */
static void *
sqldbal_sqlite_stmt_handle(const struct sqldbal_stmt *const stmt){
  sqlite3_stmt *sqlite_stmt;

  sqlite_stmt = stmt->handle;
  return sqlite_stmt;
}

/**
 * Execute a database statement that does not have a result set.
 *
 * @param[in] db  See @ref sqldbal_db.
 * @param[in] sql SQL statement to execute.
 */
static void
sqldbal_sqlite_noresult(struct sqldbal_db *const db,
                        const char *const sql){
  sqlite3 *sqlite_db;
  char *errmsg;

  sqlite_db = db->handle;
  /* https://www.sqlite.org/c3ref/exec.html */
  if(sqlite3_exec(sqlite_db, sql, NULL, NULL, &errmsg) != SQLITE_OK){
    sqldbal_err_set(db, SQLDBAL_STATUS_EXEC, errmsg);
    /* https://www.sqlite.org/c3ref/free.html */
    sqlite3_free(errmsg);
  }
}

/**
 * Begin a SQLite transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_sqlite_begin_transaction(struct sqldbal_db *const db){
  sqldbal_sqlite_noresult(db, "BEGIN");
}

/**
 * Commit a SQLite transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_sqlite_commit(struct sqldbal_db *const db){
  sqldbal_sqlite_noresult(db, "COMMIT");
}

/**
 * Rollback a SQLite transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 */
static void
sqldbal_sqlite_rollback(struct sqldbal_db *const db){
  sqldbal_sqlite_noresult(db, "ROLLBACK");
}

/**
 * Called by sqlite3_exec to process each row in the result set.
 *
 * This function copies the row data to the callback provided by the
 * application.
 *
 * @param[in] cb_argument     See @ref sqldbal_sqlite_cb_data.
 * @param[in] num_cols        Number of entries in @p col_result_list
 *                            and @p col_name_list.
 * @param[in] col_result_list Column values that have been coerced to strings.
 * @param[in] col_name_list   Column names - not used.
 * @retval SQLITE_OK    Success.
 * @retval SQLITE_ABORT An error occurred.
 */
static int
sqldbal_sqlite_exec_callback(void *cb_argument,
                             int num_cols,
                             char **col_result_list,
                             char **col_name_list){
  struct sqldbal_sqlite_cb_data *cb_data;
  int rc;
  int col_i;
  size_t num_cols_size_t;
  size_t *col_length_list;

  (void)col_name_list;

  cb_data = cb_argument;

  /* The callback does not provide a negative number for column count. */
  num_cols_size_t = (size_t)num_cols;

  col_length_list = sqldbal_reallocarray(NULL,
                                         num_cols_size_t,
                                         sizeof(*col_length_list));
  if(col_length_list == NULL){
    sqldbal_status_code_set(cb_data->db, SQLDBAL_STATUS_NOMEM);
    rc = SQLITE_ABORT;
  }
  else{
    for(col_i = 0; col_i < num_cols; col_i++){
      if(col_result_list[col_i]){
        col_length_list[col_i] = strlen(col_result_list[col_i]);
      }
      else{
        col_length_list[col_i] = 0;
      }
    }

    rc = cb_data->callback(cb_data->user_data,
                           num_cols_size_t,
                           col_result_list,
                           col_length_list);
    if(rc == 0){
      rc = SQLITE_OK;
    }
    else{
      sqldbal_status_code_set(cb_data->db, SQLDBAL_STATUS_EXEC);
      rc = SQLITE_ABORT;
    }
    free(col_length_list);
  }

  return rc;
}

/**
 * Execute a direct SQL statement.
 *
 * @param[in] db        See @ref sqldbal_db.
 * @param[in] sql       SQL command to execute.
 * @param[in] callback  Invokes this callback function for every returned row
 *                      in the query result set. Set this to NULL if query
 *                      results not needed.
 * @param[in] user_data Pass this to the first argument in the
 *                      @p callback function.
 */
static void
sqldbal_sqlite_exec(struct sqldbal_db *const db,
                    const char *const sql,
                    sqldbal_exec_callback_fp callback,
                    void *user_data){
  sqlite3 *sqlite_db;
  char *errmsg;
  struct sqldbal_sqlite_cb_data cb_data;
  int (*sqlite_exec_fp)(void *, int, char **, char **);

  sqlite_db = db->handle;

  cb_data.db = db;
  cb_data.callback = callback;
  cb_data.user_data = user_data;

  if(callback == NULL){
    sqlite_exec_fp = NULL;
  }
  else{
    sqlite_exec_fp = sqldbal_sqlite_exec_callback;
  }

  /* https://www.sqlite.org/c3ref/exec.html */
  sqlite3_exec(sqlite_db, sql, sqlite_exec_fp, &cb_data, &errmsg);
  if(errmsg){
    sqldbal_err_set(db, SQLDBAL_STATUS_EXEC, errmsg);
    /* https://www.sqlite.org/c3ref/free.html */
    sqlite3_free(errmsg);
  }
}

/**
 * Get the insert id from the last SQL insert statement.
 *
 * @param[in]  db        See @ref sqldbal_db.
 * @param[in]  name      Unused.
 * @param[out] insert_id Last insert id.
 */
static void
sqldbal_sqlite_last_insert_id(struct sqldbal_db *const db,
                              const char *const name,
                              uint64_t *insert_id){
  sqlite3 *sqlite_db;
  sqlite3_int64 sqlite_rowid;

  (void)name;

  sqlite_db = db->handle;

  /* https://www.sqlite.org/c3ref/last_insert_rowid.html */
  sqlite_rowid = sqlite3_last_insert_rowid(sqlite_db);

  /*
   * SQLite normally wouldn't return a negative value, but a virtual table
   * implementation might.
   */
  if(si_int64_to_uint64(sqlite_rowid, insert_id)){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_OVERFLOW);
  }
}

/**
 * Prepare SQLite statement.
 *
 * @param[in] db      See @ref sqldbal_db.
 * @param[in] sql     SQL query to compile.
 * @param[in] sql_len Length of @p sql in bytes, or -1 if null-terminated.
 * @param[in] stmt    See @ref sqldbal_stmt.
 */
static void
sqldbal_sqlite_stmt_prepare(struct sqldbal_db *const db,
                            const char *const sql,
                            size_t sql_len,
                            struct sqldbal_stmt *stmt){
  sqlite3 *sqlite_db;
  sqlite3_stmt *sqlite_stmt;
  int nbyte;
  int col_count;
  int param_count;

  sqlite_db = db->handle;
  if(sql_len == SIZE_MAX){
    nbyte = -1;
  }
  else if(sql_len > INT_MAX){
    sqldbal_status_code_set(db, SQLDBAL_STATUS_PARAM);
    nbyte = -2;
  }
  else{
    /* Within range because above condition ensures sql_len <= INT_MAX. */
    nbyte = (int)sql_len;
  }

  if(nbyte != -2){
    /* https://www.sqlite.org/c3ref/prepare.html */
    if(sqlite3_prepare_v2(sqlite_db,
                          sql,
                          nbyte,
                          &sqlite_stmt,
                          NULL) != SQLITE_OK){
      sqldbal_sqlite_error(db, 0, SQLDBAL_STATUS_PREPARE);
    }
    stmt->handle = sqlite_stmt;

    /*
     * https://www.sqlite.org/c3ref/column_count.html
     * This function should always return a positive value.
     */
    col_count = sqlite3_column_count(sqlite_stmt);
    stmt->num_cols_result = (size_t)col_count;

    /*
     * https://www.sqlite.org/c3ref/bind_parameter_count.html
     * This function should always return a positive value.
     */
    param_count = sqlite3_bind_parameter_count(sqlite_stmt);
    stmt->num_params = (size_t)param_count;
  }
}

/**
 * Convert the bind index to a 1-based index system required by SQLite.
 *
 * The driver interface expects a 0-based column index provided by
 * the application, but SQLite uses a 1-based colum index for the
 * bind_* functions.
 *
 * @param[in]  col_idx_in  The 0-based column index provided by the driver.
 * @param[out] col_idx_out The converted 1-based column index.
 * @retval  0 Successfully converted column index.
 * @retval !0 Error occurred when converting column index.
 */
static int
sqldbal_sqlite_get_col_idx(size_t col_idx_in,
                           int *col_idx_out){
  size_t col_idx_sum;

  if(si_add_size_t(col_idx_in, 1, &col_idx_sum) ||
     si_size_to_int(col_idx_sum, col_idx_out)){
    return 1;
  }
  return 0;
}

/**
 * Assign binary data to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] blob    Binary data saved to BLOB or BYTEA types.
 * @param[in] blobsz  Length of @p blob in bytes.
 */
static void
sqldbal_sqlite_stmt_bind_blob(struct sqldbal_stmt *const stmt,
                              size_t col_idx,
                              const void *const blob,
                              size_t blobsz){
  sqlite3_stmt *sqlite_stmt;
  int blobsz_int;
  int col_idx_i;

  if(si_size_to_int(blobsz, &blobsz_int) ||
     sqldbal_sqlite_get_col_idx(col_idx, &col_idx_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    sqlite_stmt = stmt->handle;

    /* https://www.sqlite.org/c3ref/bind_blob.html */
    if(sqlite3_bind_blob(sqlite_stmt,
                         col_idx_i,
                         blob,
                         blobsz_int,
                         SQLITE_TRANSIENT) != SQLITE_OK){
      sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_BIND);
    }
  }
}

/**
 * Assign a 64-bit integer to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] i64     Integer to bind.
 */
static void
sqldbal_sqlite_stmt_bind_int64(struct sqldbal_stmt *const stmt,
                               size_t col_idx,
                               int64_t i64){
  sqlite3_stmt *sqlite_stmt;
  int col_idx_i;

  sqlite_stmt = stmt->handle;

  if(sqldbal_sqlite_get_col_idx(col_idx, &col_idx_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    /* https://www.sqlite.org/c3ref/bind_blob.html */
    if(sqlite3_bind_int64(sqlite_stmt,
                          col_idx_i,
                          i64) != SQLITE_OK){
      sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_BIND);
    }
  }
}

/**
 * Assign a text string to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 * @param[in] s       Text string saved to a SQL text type.
 * @param[in] slen    Length of @p s in bytes.
 */
static void
sqldbal_sqlite_stmt_bind_text(struct sqldbal_stmt *const stmt,
                              size_t col_idx,
                              const char *const s,
                              size_t slen){
  sqlite3_stmt *sqlite_stmt;
  int bind_len;
  int col_idx_i;

  if(si_size_to_int(slen, &bind_len) ||
     sqldbal_sqlite_get_col_idx(col_idx, &col_idx_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    sqlite_stmt = stmt->handle;

    /* https://www.sqlite.org/c3ref/bind_blob.html */
    if(sqlite3_bind_text(sqlite_stmt,
                         col_idx_i,
                         s,
                         bind_len,
                         SQLITE_TRANSIENT) != SQLITE_OK){
      sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_BIND);
    }
  }
}

/**
 * Assign a NULL value to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index referenced in prepared statement.
 */
static void
sqldbal_sqlite_stmt_bind_null(struct sqldbal_stmt *const stmt,
                              size_t col_idx){
  sqlite3_stmt *sqlite_stmt;
  int col_idx_i;

  sqlite_stmt = stmt->handle;

  if(sqldbal_sqlite_get_col_idx(col_idx, &col_idx_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    /* https://www.sqlite.org/c3ref/bind_blob.html */
    if(sqlite3_bind_null(sqlite_stmt,
                         col_idx_i) != SQLITE_OK){
      sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_BIND);
    }
  }
}

/**
 * Pause between SQLITE_BUSY results.
 */
static void
sqldbal_sqlite_busy_sleep(void){
  struct timeval timeout;

  timeout.tv_sec = 0;
  timeout.tv_usec = 10000;
  select(0, NULL, NULL, NULL, &timeout);
}

/**
 * Execute a compiled statement with bound parameters.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_sqlite_stmt_execute(struct sqldbal_stmt *const stmt){
  sqlite3_stmt *sqlite_stmt;
  int step_rc;
  unsigned int num_retries;
  unsigned int retry_execute;

  sqlite_stmt = stmt->handle;

  num_retries = 0;
  do{
    retry_execute = 0;

    /* https://www.sqlite.org/c3ref/step.html */
    step_rc = sqlite3_step(sqlite_stmt);
    if(step_rc == SQLITE_DONE || step_rc == SQLITE_ROW){
      /* https://www.sqlite.org/c3ref/reset.html */
      if(sqlite3_reset(sqlite_stmt) != SQLITE_OK){
        sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_EXEC);
      }
    }
    else if(step_rc == SQLITE_BUSY){
      if(num_retries < SQLDBAL_SQLITE_MAX_NUM_RETRIES){
        sqldbal_sqlite_busy_sleep();
        num_retries += 1;
        retry_execute = 1;
      }
      else{
        sqldbal_sqlite_error(stmt->db, step_rc, SQLDBAL_STATUS_EXEC);
      }
    }
    else{
      sqldbal_sqlite_error(stmt->db, step_rc, SQLDBAL_STATUS_EXEC);
    }
  } while(retry_execute);
}

/**
 * Get the next row in the result set.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return See @ref sqldbal_fetch_result.
 */
static enum sqldbal_fetch_result
sqldbal_sqlite_stmt_fetch(struct sqldbal_stmt *const stmt){
  int step_rc;
  sqlite3_stmt *sqlite_stmt;
  unsigned int num_retries;
  unsigned int retry_execute;
  enum sqldbal_fetch_result fetch_result;

  sqlite_stmt = stmt->handle;
  num_retries = 0;

  fetch_result = SQLDBAL_FETCH_ERROR;
  do{
    retry_execute = 0;

    /* https://www.sqlite.org/c3ref/step.html */
    step_rc = sqlite3_step(sqlite_stmt);
    if(step_rc == SQLITE_ROW){
      fetch_result = SQLDBAL_FETCH_ROW;
    }
    else if(step_rc == SQLITE_DONE){
      fetch_result = SQLDBAL_FETCH_DONE;
    }
    else if(step_rc == SQLITE_BUSY){
      if(num_retries < SQLDBAL_SQLITE_MAX_NUM_RETRIES){
        sqldbal_sqlite_busy_sleep();
        num_retries += 1;
        retry_execute = 1;
      }
      else{
        sqldbal_sqlite_error(stmt->db, step_rc, SQLDBAL_STATUS_FETCH);
      }
    }
    else{
      sqldbal_sqlite_error(stmt->db, step_rc, SQLDBAL_STATUS_FETCH);
    }
  } while(retry_execute);

  return fetch_result;
}

/**
 * Get the column result as blob/binary data.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] blob    Binary data.
 * @param[out] blobsz  Number of bytes in @p blob.
 */
static void
sqldbal_sqlite_stmt_column_blob(struct sqldbal_stmt *const stmt,
                                size_t col_idx,
                                const void **blob,
                                size_t *blobsz){
  sqlite3_stmt *sqlite_stmt;
  int col_no_i;
  int col_bytes;

  sqlite_stmt = stmt->handle;

  if(si_size_to_int(col_idx, &col_no_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    /* https://www.sqlite.org/c3ref/column_blob.html */
    col_bytes = sqlite3_column_bytes(sqlite_stmt, col_no_i);
    *blob = sqlite3_column_blob(sqlite_stmt, col_no_i);

    if(si_int_to_size(col_bytes, blobsz)){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
    }
    else{
      if(*blobsz && *blob == NULL){
        sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_NOMEM);
      }
    }
  }
}

/**
 * Get the column result as a 64-bit integer.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] i64     64-bit integer.
 */
static void
sqldbal_sqlite_stmt_column_int64(struct sqldbal_stmt *const stmt,
                                 size_t col_idx,
                                 int64_t *i64){
  sqlite3_stmt *sqlite_stmt;
  int col_no_i;

  if(si_size_to_int(col_idx, &col_no_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    sqlite_stmt = stmt->handle;

    /* https://www.sqlite.org/c3ref/column_blob.html */
    *i64 = sqlite3_column_int64(sqlite_stmt, col_no_i);
  }
}

/**
 * Get the column result as a string.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @param[out] text    Null-terminated string.
 * @param[out] textsz  Number of bytes in @p text.
 */
static void
sqldbal_sqlite_stmt_column_text(struct sqldbal_stmt *const stmt,
                                size_t col_idx,
                                const char **text,
                                size_t *textsz){
  sqlite3_stmt *sqlite_stmt;
  int col_no_i;
  int col_bytes;

  if(si_size_to_int(col_idx, &col_no_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
  }
  else{
    sqlite_stmt = stmt->handle;

    /* https://www.sqlite.org/c3ref/column_blob.html */
    col_bytes = sqlite3_column_bytes(sqlite_stmt, col_no_i);
    *text = (const char*)sqlite3_column_text(sqlite_stmt, col_no_i);

    if(si_int_to_size(col_bytes, textsz)){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
    }
    else{
      if(*textsz){
        *textsz -= 1;
        if(*text == NULL){
          sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_NOMEM);
        }
      }
    }
  }
}

/**
 * Get the column data type.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index.
 * @return See @ref sqldbal_column_type.
 */
static enum sqldbal_column_type
sqldbal_sqlite_stmt_column_type(struct sqldbal_stmt *const stmt,
                                size_t col_idx){
  sqlite3_stmt *sqlite_stmt;
  enum sqldbal_column_type col_type;
  int sqlite_col_type;
  int col_no_i;

  if(si_size_to_int(col_idx, &col_no_i)){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_OVERFLOW);
    col_type = SQLDBAL_TYPE_ERROR;
  }
  else{
    sqlite_stmt = stmt->handle;

    /* https://www.sqlite.org/c3ref/column_blob.html */
    sqlite_col_type = sqlite3_column_type(sqlite_stmt, col_no_i);

    switch(sqlite_col_type){
      case SQLITE_INTEGER:
        col_type = SQLDBAL_TYPE_INT;
        break;
      case SQLITE_TEXT:
        col_type = SQLDBAL_TYPE_TEXT;
        break;
      case SQLITE_BLOB:
        col_type = SQLDBAL_TYPE_BLOB;
        break;
      case SQLITE_NULL:
        col_type = SQLDBAL_TYPE_NULL;
        break;
      default:
        col_type = SQLDBAL_TYPE_OTHER;
        break;
    }
  }
  return col_type;
}

/**
 * Free prepared statement resources.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 */
static void
sqldbal_sqlite_stmt_close(struct sqldbal_stmt *const stmt){
  sqlite3_stmt *sqlite_stmt;

  sqlite_stmt = stmt->handle;

  /* https://www.sqlite.org/c3ref/finalize.html */
  if(sqlite3_finalize(sqlite_stmt) != SQLITE_OK){
    sqldbal_sqlite_error(stmt->db, 0, SQLDBAL_STATUS_CLOSE);
  }
}

#endif /* SQLDBAL_SQLITE */

enum sqldbal_status_code
sqldbal_status_code_get(const struct sqldbal_db *const db){
  return db->status_code;
}

enum sqldbal_status_code
sqldbal_status_code_clear(struct sqldbal_db *const db){
  enum sqldbal_status_code status;

  status = sqldbal_status_code_get(db);
  sqldbal_status_code_set(db, SQLDBAL_STATUS_OK);
  return status;
}

enum sqldbal_driver
sqldbal_driver_type(const struct sqldbal_db *const db){
  return db->type;
}

enum sqldbal_status_code
sqldbal_errstr(const struct sqldbal_db *const db,
               const char **errstr){
  enum sqldbal_status_code status;
  const char *const sqldbal_errstr_list[] = {
    /* SQLDBAL_STATUS_OK */
    "Success",
    /* SQLDBAL_STATUS_PARAM */
    "Invalid parameter",
    /* SQLDBAL_STATUS_NOMEM */
    "Memory allocation failed",
    /* SQLDBAL_STATUS_OVERFLOW */
    "Overflow/wrap/conversion",
    /* SQLDBAL_STATUS_EXEC */
    "Failed to execute SQL statement",
    /* SQLDBAL_STATUS_PREPARE */
    "Failed to prepare statement",
    /* SQLDBAL_STATUS_BIND */
    "Failed to bind parameter",
    /* SQLDBAL_STATUS_FETCH */
    "Failed to fetch next statement result",
    /* SQLDBAL_STATUS_COLUMN_COERCE */
    "Error coercing the requested column value",
    /* SQLDBAL_STATUS_DRIVER_NOSUPPORT */
    "Driver not supported",
    /* SQLDBAL_STATUS_OPEN */
    "Failed to open database context",
    /* SQLDBAL_STATUS_CLOSE */
    "Failed to close database context",
    /* SQLDBAL_STATUS__LAST */
    "Unknown error"
  };

  status = sqldbal_status_code_get(db);
  if(db->errstr){
    *errstr = db->errstr;
  }
  else{
    if((unsigned)status >= SQLDBAL_STATUS__LAST){
      status = SQLDBAL_STATUS__LAST;
    }
    *errstr = sqldbal_errstr_list[status];
  }
  return status;
}

/**
 * This error structure used for the single error case where we cannot
 * initially allocate memory.
 *
 * This makes it easier to propagate any error codes when calling
 * other external header functions because the caller will always
 * get a valid SQLDBAL structure returned.
 */
static struct sqldbal_db g_db_error = {
  NULL,                        /* errstr                       */
  NULL,                        /* handle                       */
  {                            /* functions                    */
    NULL,                      /* sqldbal_fp_open              */
    NULL,                      /* sqldbal_fp_close             */
    NULL,                      /* sqldbal_fp_db_handle         */
    NULL,                      /* sqldbal_fp_stmt_handle       */
    NULL,                      /* sqldbal_fp_begin_transaction */
    NULL,                      /* sqldbal_fp_commit            */
    NULL,                      /* sqldbal_fp_rollback          */
    NULL,                      /* sqldbal_fp_exec              */
    NULL,                      /* sqldbal_fp_last_insert_id    */
    NULL,                      /* sqldbal_fp_stmt_prepare      */
    NULL,                      /* sqldbal_fp_stmt_bind_blob    */
    NULL,                      /* sqldbal_fp_stmt_bind_int64   */
    NULL,                      /* sqldbal_fp_stmt_bind_text    */
    NULL,                      /* sqldbal_fp_stmt_bind_null    */
    NULL,                      /* sqldbal_fp_stmt_execute      */
    NULL,                      /* sqldbal_fp_stmt_fetch        */
    NULL,                      /* sqldbal_fp_stmt_column_blob  */
    NULL,                      /* sqldbal_fp_stmt_column_int64 */
    NULL,                      /* sqldbal_fp_stmt_column_text  */
    NULL,                      /* sqldbal_fp_stmt_column_type  */
    NULL,                      /* sqldbal_fp_stmt_close        */
  },                           /* functions                    */
  SQLDBAL_STATUS_NOMEM,        /* status_code                  */
  SQLDBAL_FLAG_INVALID_MEMORY, /* flags                        */
  SQLDBAL_DRIVER_INVALID,      /* type                         */
  {0}                          /* pad                          */
};

enum sqldbal_status_code
sqldbal_open(enum sqldbal_driver driver,
             const char *const location,
             const char *const port,
             const char *const username,
             const char *const password,
             const char *const database,
             enum sqldbal_flag flags,
             const struct sqldbal_driver_option *const option_list,
             size_t num_options,
             struct sqldbal_db **db){
  struct sqldbal_db *new_db;
  struct sqldbal_driver_functions *func;

  new_db = malloc(sizeof(*new_db));
  if(new_db == NULL){
    *db = &g_db_error;
    sqldbal_status_code_set(*db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    *db = new_db;

    sqldbal_status_code_set(new_db, SQLDBAL_STATUS_OK);
    new_db->errstr = NULL;
    new_db->flags = flags;
    new_db->type = driver;
    new_db->handle = NULL;

    func = &new_db->functions;
    switch(driver){
#ifdef SQLDBAL_MARIADB
    case SQLDBAL_DRIVER_MARIADB:
    case SQLDBAL_DRIVER_MYSQL:
      func->sqldbal_fp_open              = sqldbal_mariadb_open;
      func->sqldbal_fp_close             = sqldbal_mariadb_close;
      func->sqldbal_fp_db_handle         = sqldbal_mariadb_db_handle;
      func->sqldbal_fp_stmt_handle       = sqldbal_mariadb_stmt_handle;
      func->sqldbal_fp_begin_transaction = sqldbal_mariadb_begin_transaction;
      func->sqldbal_fp_commit            = sqldbal_mariadb_commit;
      func->sqldbal_fp_rollback          = sqldbal_mariadb_rollback;
      func->sqldbal_fp_exec              = sqldbal_mariadb_exec;
      func->sqldbal_fp_last_insert_id    = sqldbal_mariadb_last_insert_id;
      func->sqldbal_fp_stmt_prepare      = sqldbal_mariadb_stmt_prepare;
      func->sqldbal_fp_stmt_bind_blob    = sqldbal_mariadb_stmt_bind_blob;
      func->sqldbal_fp_stmt_bind_int64   = sqldbal_mariadb_stmt_bind_int64;
      func->sqldbal_fp_stmt_bind_text    = sqldbal_mariadb_stmt_bind_text;
      func->sqldbal_fp_stmt_bind_null    = sqldbal_mariadb_stmt_bind_null;
      func->sqldbal_fp_stmt_execute      = sqldbal_mariadb_stmt_execute;
      func->sqldbal_fp_stmt_fetch        = sqldbal_mariadb_stmt_fetch;
      func->sqldbal_fp_stmt_column_blob  = sqldbal_mariadb_stmt_column_blob;
      func->sqldbal_fp_stmt_column_int64 = sqldbal_mariadb_stmt_column_int64;
      func->sqldbal_fp_stmt_column_text  = sqldbal_mariadb_stmt_column_text;
      func->sqldbal_fp_stmt_column_type  = sqldbal_mariadb_stmt_column_type;
      func->sqldbal_fp_stmt_close        = sqldbal_mariadb_stmt_close;
      break;
#endif /* SQLDBAL_MARIADB */
#ifdef SQLDBAL_POSTGRESQL
    case SQLDBAL_DRIVER_POSTGRESQL:
      func->sqldbal_fp_open              = sqldbal_pq_open;
      func->sqldbal_fp_close             = sqldbal_pq_close;
      func->sqldbal_fp_db_handle         = sqldbal_pq_db_handle;
      func->sqldbal_fp_stmt_handle       = sqldbal_pq_stmt_handle;
      func->sqldbal_fp_begin_transaction = sqldbal_pq_begin_transaction;
      func->sqldbal_fp_commit            = sqldbal_pq_commit;
      func->sqldbal_fp_rollback          = sqldbal_pq_rollback;
      func->sqldbal_fp_exec              = sqldbal_pq_exec;
      func->sqldbal_fp_last_insert_id    = sqldbal_pq_last_insert_id;
      func->sqldbal_fp_stmt_prepare      = sqldbal_pq_stmt_prepare;
      func->sqldbal_fp_stmt_bind_blob    = sqldbal_pq_stmt_bind_blob;
      func->sqldbal_fp_stmt_bind_int64   = sqldbal_pq_stmt_bind_int64;
      func->sqldbal_fp_stmt_bind_text    = sqldbal_pq_stmt_bind_text;
      func->sqldbal_fp_stmt_bind_null    = sqldbal_pq_stmt_bind_null;
      func->sqldbal_fp_stmt_execute      = sqldbal_pq_stmt_execute;
      func->sqldbal_fp_stmt_fetch        = sqldbal_pq_stmt_fetch;
      func->sqldbal_fp_stmt_column_blob  = sqldbal_pq_stmt_column_blob;
      func->sqldbal_fp_stmt_column_int64 = sqldbal_pq_stmt_column_int64;
      func->sqldbal_fp_stmt_column_text  = sqldbal_pq_stmt_column_text;
      func->sqldbal_fp_stmt_column_type  = sqldbal_pq_stmt_column_type;
      func->sqldbal_fp_stmt_close        = sqldbal_pq_stmt_close;
      break;
#endif /* SQLDBAL_POSTGRESQL */
#ifdef SQLDBAL_SQLITE
    case SQLDBAL_DRIVER_SQLITE:
      func->sqldbal_fp_open              = sqldbal_sqlite_open;
      func->sqldbal_fp_close             = sqldbal_sqlite_close;
      func->sqldbal_fp_db_handle         = sqldbal_sqlite_db_handle;
      func->sqldbal_fp_stmt_handle       = sqldbal_sqlite_stmt_handle;
      func->sqldbal_fp_begin_transaction = sqldbal_sqlite_begin_transaction;
      func->sqldbal_fp_commit            = sqldbal_sqlite_commit;
      func->sqldbal_fp_rollback          = sqldbal_sqlite_rollback;
      func->sqldbal_fp_exec              = sqldbal_sqlite_exec;
      func->sqldbal_fp_last_insert_id    = sqldbal_sqlite_last_insert_id;
      func->sqldbal_fp_stmt_prepare      = sqldbal_sqlite_stmt_prepare;
      func->sqldbal_fp_stmt_bind_blob    = sqldbal_sqlite_stmt_bind_blob;
      func->sqldbal_fp_stmt_bind_int64   = sqldbal_sqlite_stmt_bind_int64;
      func->sqldbal_fp_stmt_bind_text    = sqldbal_sqlite_stmt_bind_text;
      func->sqldbal_fp_stmt_bind_null    = sqldbal_sqlite_stmt_bind_null;
      func->sqldbal_fp_stmt_execute      = sqldbal_sqlite_stmt_execute;
      func->sqldbal_fp_stmt_fetch        = sqldbal_sqlite_stmt_fetch;
      func->sqldbal_fp_stmt_column_blob  = sqldbal_sqlite_stmt_column_blob;
      func->sqldbal_fp_stmt_column_int64 = sqldbal_sqlite_stmt_column_int64;
      func->sqldbal_fp_stmt_column_text  = sqldbal_sqlite_stmt_column_text;
      func->sqldbal_fp_stmt_column_type  = sqldbal_sqlite_stmt_column_type;
      func->sqldbal_fp_stmt_close        = sqldbal_sqlite_stmt_close;
      break;
#endif /* SQLDBAL_SQLITE */
    case SQLDBAL_DRIVER_INVALID:
    default:
      sqldbal_status_code_set(new_db, SQLDBAL_STATUS_DRIVER_NOSUPPORT);
      break;
    }

    if(sqldbal_status_code_get(new_db) == SQLDBAL_STATUS_OK){
      new_db->functions.sqldbal_fp_open(new_db,
                                        location,
                                        port,
                                        username,
                                        password,
                                        database,
                                        option_list,
                                        num_options);
    }
  }
  return sqldbal_status_code_get(*db);
}

enum sqldbal_status_code
sqldbal_close(struct sqldbal_db *db){
  enum sqldbal_status_code status;

  status = sqldbal_status_code_get(db);
  if((db->flags & SQLDBAL_FLAG_INVALID_MEMORY) == 0){
    if(status != SQLDBAL_STATUS_DRIVER_NOSUPPORT){
      db->functions.sqldbal_fp_close(db);
      if(status == SQLDBAL_STATUS_OK){
        status = sqldbal_status_code_get(db);
      }
    }
    if(status != SQLDBAL_STATUS_CLOSE){
      free(db->errstr);
      free(db);
    }
  }
  return status;
}

void *
sqldbal_db_handle(const struct sqldbal_db *const db){
  return db->functions.sqldbal_fp_db_handle(db);
}

void *
sqldbal_stmt_handle(const struct sqldbal_stmt *const stmt){
  return stmt->db->functions.sqldbal_fp_stmt_handle(stmt);
}

enum sqldbal_status_code
sqldbal_begin_transaction(struct sqldbal_db *const db){
  db->functions.sqldbal_fp_begin_transaction(db);
  return sqldbal_status_code_get(db);
}

enum sqldbal_status_code
sqldbal_commit(struct sqldbal_db *const db){
  db->functions.sqldbal_fp_commit(db);
  return sqldbal_status_code_get(db);
}

enum sqldbal_status_code
sqldbal_rollback(struct sqldbal_db *const db){
  db->functions.sqldbal_fp_rollback(db);
  return sqldbal_status_code_get(db);
}

enum sqldbal_status_code
sqldbal_exec(struct sqldbal_db *const db,
             const char *const sql,
             sqldbal_exec_callback_fp callback,
             void *user_data){
  db->functions.sqldbal_fp_exec(db, sql, callback, user_data);
  return sqldbal_status_code_get(db);
}

enum sqldbal_status_code
sqldbal_last_insert_id(struct sqldbal_db *const db,
                       const char *const name,
                       uint64_t *insert_id){
  db->functions.sqldbal_fp_last_insert_id(db, name, insert_id);
  return sqldbal_status_code_get(db);
}

/**
 * This error structure used for the single error case where we cannot
 * initially allocate memory for the @ref sqldbal_stmt.
 */
static struct sqldbal_stmt
g_stmt_error = {
  &g_db_error,                      /* db              */
  0   ,                             /* num_params      */
  0   ,                             /* num_cols_result */
  NULL,                             /* handle          */
  0   ,                             /* valid           */
  {0}                               /* pad             */
};

enum sqldbal_status_code
sqldbal_stmt_prepare(struct sqldbal_db *const db,
                     const char *const sql,
                     size_t sql_len,
                     struct sqldbal_stmt **stmt){
  struct sqldbal_stmt *new_stmt;

  new_stmt = malloc(sizeof(*new_stmt));
  if(new_stmt == NULL){
    *stmt = &g_stmt_error;
    sqldbal_status_code_set(db, SQLDBAL_STATUS_NOMEM);
  }
  else{
    *stmt = new_stmt;
    new_stmt->db              = db;
    new_stmt->num_params      = 0;
    new_stmt->num_cols_result = 0;
    new_stmt->handle          = NULL;
    new_stmt->valid           = 1;
    db->functions.sqldbal_fp_stmt_prepare(db, sql, sql_len, new_stmt);
  }
  return sqldbal_status_code_get(db);
}

/**
 * Ensures the bind index provided by the application stays within bounds.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Column index.
 * @retval 1 Column index is valid.
 * @retval 0 Column index is not valid.
 */
static int
sqldbal_stmt_bind_in_range(const struct sqldbal_stmt *const stmt,
                           size_t col_idx){
  int rc;

  rc = 1;
  if(col_idx >= stmt->num_params){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_PARAM);
    rc = 0;
  }
  return rc;
}

enum sqldbal_status_code
sqldbal_stmt_bind_blob(struct sqldbal_stmt *const stmt,
                       size_t col_idx,
                       const void *const blob,
                       size_t blobsz){
  if(sqldbal_stmt_bind_in_range(stmt, col_idx)){
    stmt->db->functions.sqldbal_fp_stmt_bind_blob(stmt,
                                                  col_idx,
                                                  blob,
                                                  blobsz);
  }
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_status_code
sqldbal_stmt_bind_int64(struct sqldbal_stmt *const stmt,
                        size_t col_idx,
                        int64_t i64){
  if(sqldbal_stmt_bind_in_range(stmt, col_idx)){
    stmt->db->functions.sqldbal_fp_stmt_bind_int64(stmt, col_idx, i64);
  }
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_status_code
sqldbal_stmt_bind_text(struct sqldbal_stmt *const stmt,
                       size_t col_idx,
                       const char *const s,
                       size_t slen){
  if(sqldbal_stmt_bind_in_range(stmt, col_idx)){
    if(slen == (size_t)-1){
      slen = strlen(s);
    }

    /* Add one more byte to include null-terminator character. */
    if(si_add_size_t(slen, 1, &slen)){
      sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_NOMEM);
    }
    else{
      stmt->db->functions.sqldbal_fp_stmt_bind_text(stmt, col_idx, s, slen);
    }
  }
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_status_code
sqldbal_stmt_bind_null(struct sqldbal_stmt *const stmt,
                       size_t col_idx){
  if(sqldbal_stmt_bind_in_range(stmt, col_idx)){
    stmt->db->functions.sqldbal_fp_stmt_bind_null(stmt, col_idx);
  }
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_status_code
sqldbal_stmt_execute(struct sqldbal_stmt *const stmt){
  stmt->db->functions.sqldbal_fp_stmt_execute(stmt);
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_fetch_result
sqldbal_stmt_fetch(struct sqldbal_stmt *const stmt){
  return stmt->db->functions.sqldbal_fp_stmt_fetch(stmt);
}

/**
 * Ensures the column index provided by the application stays within bounds.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Column index.
 * @retval 0 Column index not valid.
 * @retval 1 Column index valid.
 */
static int
sqldbal_stmt_column_in_range(const struct sqldbal_stmt *const stmt,
                             size_t col_idx){
  int rc;

  rc = 1;
  if(col_idx >= stmt->num_cols_result){
    sqldbal_status_code_set(stmt->db, SQLDBAL_STATUS_PARAM);
    rc = 0;
  }
  return rc;
}

enum sqldbal_status_code
sqldbal_stmt_column_blob(struct sqldbal_stmt *const stmt,
                         size_t col_idx,
                         const void **blob,
                         size_t *blobsz){
  if(sqldbal_stmt_column_in_range(stmt, col_idx)){
    stmt->db->functions.sqldbal_fp_stmt_column_blob(stmt,
                                                    col_idx,
                                                    blob,
                                                    blobsz);
  }
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_status_code
sqldbal_stmt_column_int64(struct sqldbal_stmt *const stmt,
                          size_t col_idx,
                          int64_t *i64){
  if(sqldbal_stmt_column_in_range(stmt, col_idx)){
    stmt->db->functions.sqldbal_fp_stmt_column_int64(stmt, col_idx, i64);
  }
  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_status_code
sqldbal_stmt_column_text(struct sqldbal_stmt *const stmt,
                         size_t col_idx,
                         const char **text,
                         size_t *textsz){
  size_t text_len;

  if(sqldbal_stmt_column_in_range(stmt, col_idx)){
    stmt->db->functions.sqldbal_fp_stmt_column_text(stmt,
                                                    col_idx,
                                                    text,
                                                    &text_len);
    if(textsz){
      *textsz = text_len;
    }
  }

  return sqldbal_status_code_get(stmt->db);
}

enum sqldbal_column_type
sqldbal_stmt_column_type(struct sqldbal_stmt *const stmt,
                         size_t col_idx){
  enum sqldbal_column_type type;

  if(sqldbal_stmt_column_in_range(stmt, col_idx)){
    type = stmt->db->functions.sqldbal_fp_stmt_column_type(stmt, col_idx);
  }
  else{
    type = SQLDBAL_TYPE_ERROR;
  }
  return type;
}

enum sqldbal_status_code
sqldbal_stmt_close(struct sqldbal_stmt *const stmt){
  const struct sqldbal_db *db;

  db = stmt->db;
  if(stmt != &g_stmt_error){
    db->functions.sqldbal_fp_stmt_close(stmt);
    free(stmt);
  }
  return sqldbal_status_code_get(db);
}

