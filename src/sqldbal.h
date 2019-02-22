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
#ifndef SQLDBAL_H
#define SQLDBAL_H

#include <stdint.h>

#ifdef SQLDBAL_MYSQL
# define SQLDBAL_MARIADB
#endif /* SQLDBAL_MYSQL */

#ifdef SQLDBAL_PQ
# define SQLDBAL_POSTGRESQL
#endif /* SQLDBAL_PQ */

#if !defined(SQLDBAL_MARIADB   ) && \
    !defined(SQLDBAL_POSTGRESQL) && \
    !defined(SQLDBAL_SQLITE)
# error "must define at least one driver: "
        "SQLDBAL_MARIADB, "
        "SQLDBAL_POSTGRESQL, "
        "SQLDBAL_SQLITE"
#endif /* no drivers defined */

/**
 * Special flags for the @ref sqldbal_db context.
 */
enum sqldbal_flag{
  /**
   * No flag.
   */
  SQLDBAL_FLAG_NONE                  = 0,

  /**
   * Print debug/tracing information to stderr.
   */
  SQLDBAL_FLAG_DEBUG                 = 1 << 0,

#ifdef SQLDBAL_SQLITE
  /**
   * Open the SQLite database in read mode.
   */
  SQLDBAL_FLAG_SQLITE_OPEN_READONLY  = 1 << 16,

  /**
   * Open the SQLite database in read/write mode.
   */
  SQLDBAL_FLAG_SQLITE_OPEN_READWRITE = 1 << 17,

  /**
   * Create the SQLite database if it does not exist yet.
   */
  SQLDBAL_FLAG_SQLITE_OPEN_CREATE    = 1 << 18,
#endif /* SQLDBAL_SQLITE */

  /**
   * Special flag for the database context used to determine if the
   * initial memory allocation failed.
   */
  SQLDBAL_FLAG_INVALID_MEMORY        = 1 << 30
};

/**
 * SQL drivers available to use in this library.
 */
enum sqldbal_driver{
#ifdef SQLDBAL_MARIADB
  /**
   * MariaDB/MySQL driver.
   */
  SQLDBAL_DRIVER_MARIADB      = 3,

  /**
   * Same driver as @ref SQLDBAL_DRIVER_MARIADB.
   *
   * This has a different value than @ref SQLDBAL_DRIVER_MARIADB
   * because the application may need to distinguish between the
   * two driver names.
   */
  SQLDBAL_DRIVER_MYSQL        = 4,
#endif /* SQLDBAL_MARIADB */

#ifdef SQLDBAL_POSTGRESQL
  /**
   * PostgreSQL driver using the pq library.
   */
  SQLDBAL_DRIVER_POSTGRESQL   = 7,
#endif /* SQLDBAL_POSTGRESQL */

#ifdef SQLDBAL_SQLITE
  /**
   * SQLite driver using SQLite3 library.
   */
  SQLDBAL_DRIVER_SQLITE       = 12,
#endif /* SQLDBAL_SQLITE */

  /**
   * Unknown driver or invalid database driver context.
   */
  SQLDBAL_DRIVER_INVALID      = 100
};

/**
 * Status codes indicating success or failure after calling any of the
 * sqldbal functions.
 *
 * Use @ref sqldbal_errstr to get more detailed error information.
 */
enum sqldbal_status_code{
  /**
   * Successful operation completed.
   */
  SQLDBAL_STATUS_OK,

  /**
   * Invalid parameter.
   */
  SQLDBAL_STATUS_PARAM,

  /**
   * Memory allocation failed.
   */
  SQLDBAL_STATUS_NOMEM,

  /**
   * One of the following problems has occurred:
   *   - Overflow
   *   - Wrap
   *   - Conversion issue
   */
  SQLDBAL_STATUS_OVERFLOW,

  /**
   * Failed to execute SQL statement.
   */
  SQLDBAL_STATUS_EXEC,

  /**
   * Failed to prepare statement.
   *
   * This could get set because of syntax error, communication error, etc.
   */
  SQLDBAL_STATUS_PREPARE,

  /**
   * Failed to bind parameter.
   */
  SQLDBAL_STATUS_BIND,

  /**
   * Failed to fetch the next result from the executed prepared statement.
   */
  SQLDBAL_STATUS_FETCH,

  /**
   * Error occurred while trying to coerce the requested column value.
   */
  SQLDBAL_STATUS_COLUMN_COERCE,

  /**
   * Driver not supported or not linked in with library.
   */
  SQLDBAL_STATUS_DRIVER_NOSUPPORT,

  /**
   * Failed to open the database handle or connection to the database server.
   */
  SQLDBAL_STATUS_OPEN,

  /**
   * Failed to close or free resources associated with the database.
   */
  SQLDBAL_STATUS_CLOSE,

  /**
   * Indicates the last status code in the enumeration, useful for
   * bounds checking.
   *
   * Not a valid status code.
   */
  SQLDBAL_STATUS__LAST
};

/**
 * Returned by the @ref sqldbal_stmt_fetch function.
 *
 * The application can use this result code to determine if more rows exist
 * in the result set.
 */
enum sqldbal_fetch_result{
  /**
   * The next row has been returned in the result set.
   *
   * The application can call the sqldbal_stmt_column_* routines to get the
   * columns for the currently fetched row.
   */
  SQLDBAL_FETCH_ROW,

  /**
   * No more selected rows exist in the result set.
   */
  SQLDBAL_FETCH_DONE,

  /**
   * An error occurred while fetching the next result.
   */
  SQLDBAL_FETCH_ERROR
};

/**
 * Column data type in the result set of a prepared statement.
 *
 * Applications can determine data types by calling
 * @ref sqldbal_stmt_column_type.
 *
 * Some drivers may consider text and integers as blobs. However, applications
 * can still use this to check if the result has a NULL value.
 */
enum sqldbal_column_type{
  /**
   * Integer.
   */
  SQLDBAL_TYPE_INT,

  /**
   * Text string.
   */
  SQLDBAL_TYPE_TEXT,

  /**
   * Blob/binary data.
   */
  SQLDBAL_TYPE_BLOB,

  /**
   * NULL value.
   */
  SQLDBAL_TYPE_NULL,

  /**
   * Non-standard data type.
   */
  SQLDBAL_TYPE_OTHER,

  /**
   * Error occurred while trying to determine the data type.
   */
  SQLDBAL_TYPE_ERROR
};

/**
 * Driver-specific options to pass to the driver when creating the database
 * connection.
 */
struct sqldbal_driver_option{
  /**
   * Unique identifier naming the option.
   */
  const char *key;

  /**
   * Value corresponding to @ref key.
   */
  const char *value;
};

/**
 * Callback function type used to process returned SQL results.
 */
typedef int
(*sqldbal_exec_callback_fp)(void *user_data,
                            size_t num_cols,
                            char **col_result_list,
                            size_t *col_length_list);

struct sqldbal_db;

struct sqldbal_stmt;

enum sqldbal_status_code
sqldbal_status_code_get(const struct sqldbal_db *const db);

enum sqldbal_status_code
sqldbal_status_code_clear(struct sqldbal_db *const db);

enum sqldbal_driver
sqldbal_driver_type(const struct sqldbal_db *const db);

enum sqldbal_status_code
sqldbal_errstr(const struct sqldbal_db *const db,
               const char **errstr);

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
             struct sqldbal_db **db);

enum sqldbal_status_code
sqldbal_close(struct sqldbal_db *db);

void *
sqldbal_db_handle(const struct sqldbal_db *const db);

void *
sqldbal_stmt_handle(const struct sqldbal_stmt *const stmt);

enum sqldbal_status_code
sqldbal_begin_transaction(struct sqldbal_db *const db);

enum sqldbal_status_code
sqldbal_commit(struct sqldbal_db *const db);

enum sqldbal_status_code
sqldbal_rollback(struct sqldbal_db *const db);

enum sqldbal_status_code
sqldbal_exec(struct sqldbal_db *const db,
             const char *const sql,
             sqldbal_exec_callback_fp callback,
             void *user_data);

enum sqldbal_status_code
sqldbal_last_insert_id(struct sqldbal_db *const db,
                       const char *const name,
                       uint64_t *insert_id);

enum sqldbal_status_code
sqldbal_stmt_prepare(struct sqldbal_db *const db,
                     const char *const sql,
                     size_t sql_len,
                     struct sqldbal_stmt **stmt);

enum sqldbal_status_code
sqldbal_stmt_bind_blob(struct sqldbal_stmt *const stmt,
                       size_t col_idx,
                       const void *const blob,
                       size_t blobsz);

enum sqldbal_status_code
sqldbal_stmt_bind_int64(struct sqldbal_stmt *const stmt,
                        size_t col_idx,
                        int64_t i64);

enum sqldbal_status_code
sqldbal_stmt_bind_text(struct sqldbal_stmt *const stmt,
                       size_t col_idx,
                       const char *const s,
                       size_t slen);

enum sqldbal_status_code
sqldbal_stmt_bind_null(struct sqldbal_stmt *const stmt,
                       size_t col_idx);

enum sqldbal_status_code
sqldbal_stmt_execute(struct sqldbal_stmt *const stmt);

enum sqldbal_fetch_result
sqldbal_stmt_fetch(struct sqldbal_stmt *const stmt);

enum sqldbal_status_code
sqldbal_stmt_column_blob(struct sqldbal_stmt *const stmt,
                         size_t col_idx,
                         const void **blob,
                         size_t *blobsz);

enum sqldbal_status_code
sqldbal_stmt_column_int64(struct sqldbal_stmt *const stmt,
                          size_t col_idx,
                          int64_t *i64);

enum sqldbal_status_code
sqldbal_stmt_column_text(struct sqldbal_stmt *const stmt,
                         size_t col_idx,
                         const char **text,
                         size_t *textsz);

enum sqldbal_column_type
sqldbal_stmt_column_type(struct sqldbal_stmt *const stmt,
                         size_t col_idx);

enum sqldbal_status_code
sqldbal_stmt_close(struct sqldbal_stmt *const stmt);

#endif /* SQLDBAL_H */

