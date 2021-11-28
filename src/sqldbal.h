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
 * @defgroup sqldbal_flag
 *
 * Special flags for the @ref sqldbal_db context.
 */

/**
 * @ingroup sqldbal_flag
 *
 * No flag.
 */
#define SQLDBAL_FLAG_NONE                  (0)

/**
 * @ingroup sqldbal_flag
 *
 * Print debug/tracing information to stderr.
 */
#define SQLDBAL_FLAG_DEBUG                 (1 << 0)

#ifdef SQLDBAL_SQLITE
/**
 * @ingroup sqldbal_flag
 *
 * Open the SQLite database in read mode.
 */
#define SQLDBAL_FLAG_SQLITE_OPEN_READONLY  (1 << 16)

/**
 * @ingroup sqldbal_flag
 *
 * Open the SQLite database in read/write mode.
 */
#define SQLDBAL_FLAG_SQLITE_OPEN_READWRITE (1 << 17)

/**
 * @ingroup sqldbal_flag
 *
 * Create the SQLite database if it does not exist yet.
 */
#define SQLDBAL_FLAG_SQLITE_OPEN_CREATE    (1 << 18)
#endif /* SQLDBAL_SQLITE */

/**
 * @ingroup sqldbal_flag
 *
 * Special flag for the database context used to determine if the
 * initial memory allocation failed.
 */
#define SQLDBAL_FLAG_INVALID_MEMORY        (1 << 30)

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

/**
 * Get the last error code set by the library.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_status_code_get(const struct sqldbal_db *const db);

/**
 * Clear the error code set in the database handle.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       Previous error code before clearing.
 */
enum sqldbal_status_code
sqldbal_status_code_clear(struct sqldbal_db *const db);

/**
 * Get the current driver type used by the database handle.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       See @ref sqldbal_driver.
 */
enum sqldbal_driver
sqldbal_driver_type(const struct sqldbal_db *const db);

/**
 * Get a string describing the previous library function error.
 *
 * @param[in]  db     See @ref sqldbal_db.
 * @param[out] errstr String describing the previous error. Do not
 *                    modify this string.
 * @return            See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_errstr(const struct sqldbal_db *const db,
               const char **errstr);

/**
 * Open a new connection to the database.
 *
 * If successful, this will return a valid database context in @p db.
 * Applications must call @ref sqldbal_close after finished with its
 * database operations.
 *
 * MariaDB/MySQL
 * =============
 * Supports the following options in @p option_list. Review the
 * mysql_optionsv documentation for more information on the meaning of each
 * parameter.
 *   - CONNECT_TIMEOUT (MYSQL_OPT_CONNECT_TIMEOUT - timeout in seconds       )
 *   - TLS_KEY         (MYSQL_OPT_SSL_KEY         - path to private key file )
 *   - TLS_CERT        (MYSQL_OPT_SSL_CERT        - path to certificate file )
 *   - TLS_CA          (MYSQL_OPT_SSL_CA          - path to CA file          )
 *   - TLS_CAPATH      (MYSQL_OPT_SSL_CAPATH      - path directory CA files  )
 *   - TLS_CIPHER      (MYSQL_OPT_SSL_CIPHER      - list of permitted ciphers)
 *
 * PostgreSQL
 * ==========
 * Supports the following options in @p option_list. Review the
 * PQconnectdb documentation for more information on the meaning of each
 * parameter.
 *   - CONNECT_TIMEOUT (connect_timeout - timeout in seconds      )
 *   - TLS_MODE        (sslmode         - see below               )
 *   - TLS_CERT        (sslcert         - path to certificate file)
 *   - TLS_KEY         (sslkey          - path to private key file)
 *   - TLS_CA          (sslrootcert     - path to CA file         )
 *
 * The TLS_MODE parameter for PostgreSQL can have one of the following values:
 *   - disable     - Use unencrypted connection.
 *   - allow       - Use unencrypted if available, otherwise use encrypted.
 *   - prefer      - Use TLS if available, otherwise use unencrypted.
 *   - require     - Use TLS and verify certificate issued by trusted CA if
 *                   a CA file has been provided.
 *   - verify-ca   - Use TLS and verify certificate issued by trusted CA.
 *   - verify-full - Use TLS and verify certificate issued by trusted CA. Also
 *                   check host name matches the name in the certificate.
 *
 * SQLite
 * ======
 * Provide the file path in the @p location parameter.
 * Ignores the @p port, @p username, @p password, and @p database parameters.
 * Supports the following options in @p option_list:
 *   - VFS (Name of Virtual File System to use)
 *
 * @param[in]  driver      See @ref sqldbal_driver.
 * @param[in]  location    File path, host name, or IP address.
 * @param[in]  port        Server port number to connect to.
 * @param[in]  username    Username to use when connecting to the database.
 * @param[in]  password    Password corresponding to @p username.
 * @param[in]  database    Name of database to use after connecting to the
 *                         server, or NULL if application does not
 *                         need to initially connect to a database.
 * @param[in]  flags       See @ref sqldbal_flag.
 * @param[in]  option_list Pass driver-specific parameters desribed above.
 * @param[in]  num_options Number of entries in @p option_list.
 * @param[out] db          See @ref sqldbal_db.
 * @return                 See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_open(enum sqldbal_driver driver,
             const char *const location,
             const char *const port,
             const char *const username,
             const char *const password,
             const char *const database,
             const unsigned long flags,
             const struct sqldbal_driver_option *const option_list,
             size_t num_options,
             struct sqldbal_db **db);

/**
 * Close the database handle previously opened by @ref sqldbal_open.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_close(struct sqldbal_db *db);

/**
 * Get the driver database handle.
 *
 * The caller can use the returned handle in driver-specific code.
 * Do not free the database handle directly and instead use the
 * @ref sqldbal_close function to free the database resources.
 *
 * The returned type will need to get cast based on the driver type:
 *   - MariaDB   : MYSQL *
 *   - PostgreSQL: PGconn *
 *   - SQLite    : sqlite3 *
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       Driver database handle.
 */
void *
sqldbal_db_handle(const struct sqldbal_db *const db);

/**
 * Get the driver statement handle.
 *
 * The caller can use the returned handle in driver-specific code.
 * Do not free the statement handle directly and instead use the
 * @ref sqldbal_stmt_close function to free the statement resources.
 *
 * The returned type will need to get cast based on the driver type:
 *   - MariaDB   : MYSQL_STMT *
 *   - PostgreSQL: const char *
 *   - SQLite    : sqlite3_stmt *
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return         Driver statement handle.
 */
void *
sqldbal_stmt_handle(const struct sqldbal_stmt *const stmt);

/**
 * Start a new database transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_begin_transaction(struct sqldbal_db *const db);

/**
 * End a transaction previously started by @ref sqldbal_begin_transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_commit(struct sqldbal_db *const db);

/**
 * Rollback a transaction previously started by
 * @ref sqldbal_begin_transaction.
 *
 * @param[in] db See @ref sqldbal_db.
 * @return       See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_rollback(struct sqldbal_db *const db);

/**
 * Execute a SQL query directly without preparing statements.
 *
 * Invokes the application @p callback function for each row in the result set.
 *
 * @param[in] db        See @ref sqldbal_db.
 * @param[in] sql       SQL query to execute.
 * @param[in] callback  Function called for each result row. The application
 *                      can set this to NULL if it does not need to process
 *                      the results.
 * @param[in] user_data Application data to send to @p callback.
 * @return              See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_exec(struct sqldbal_db *const db,
             const char *const sql,
             sqldbal_exec_callback_fp callback,
             void *user_data);

/**
 * Get the insert id from the last SQL insert statement.
 *
 * The PostgreSQL driver uses the @p name parameter. When creating tables
 * in PostgreSQL, use the SERIAL data type and then concatenate the table name
 * with the sequence name and add '_seq' to the end to use as the @p name
 * parameter. For example, if you have table name 'table' and primary key
 * sequence named 'id', then the @p name parameter would get set to
 * "table_id_seq".
 *
 * @param[in]  db        See @ref sqldbal_db.
 * @param[in]  name      Sequence name.
 * @param[out] insert_id Last insert id.
 * @return               See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_last_insert_id(struct sqldbal_db *const db,
                       const char *const name,
                       uint64_t *insert_id);

/**
 * Compile a SQL query and return a statement handle.
 *
 * @param[in]  db      See @ref sqldbal_db.
 * @param[in]  sql     Null-terminated SQL string.
 * @param[in]  sql_len Length of @p sql in bytes up to the null-terminator,
 *                     or -1 to have the library compute the string length.
 * @param[out] stmt    See @ref sqldbal_stmt.
 * @return             See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_prepare(struct sqldbal_db *const db,
                     const char *const sql,
                     size_t sql_len,
                     struct sqldbal_stmt **stmt);

/**
 * Assign binary data to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index starting at 0.
 * @param[in] blob    Binary data saved to BLOB or BYTEA types.
 * @param[in] blobsz  Length of @p blob in bytes.
 * @return            See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_bind_blob(struct sqldbal_stmt *const stmt,
                       size_t col_idx,
                       const void *const blob,
                       size_t blobsz);

/**
 * Assign a 64-bit integer to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index starting at 0.
 * @param[in] i64     Integer to bind.
 * @return            See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_bind_int64(struct sqldbal_stmt *const stmt,
                        size_t col_idx,
                        int64_t i64);

/**
 * Assign a string to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index starting at 0.
 * @param[in] s       Null-terminated string to bind.
 * @param[in] slen    Length of @p s in bytes up to the null-terminator,
 *                    or -1 to have the library compute the string length.
 * @return            See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_bind_text(struct sqldbal_stmt *const stmt,
                       size_t col_idx,
                       const char *const s,
                       size_t slen);

/**
 * Assign a NULL value to a prepared statement placeholder.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Placeholder index starting at 0.
 * @return            See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_bind_null(struct sqldbal_stmt *const stmt,
                       size_t col_idx);

/**
 * Execute a compiled statement with bound parameters.
 *
 * The sqldbal_stmt_bind*() routines must get set for every parameter
 * before calling this function. A parameter only needs to get bound at
 * least once before this call - future calls will use the previously bound
 * parameter.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return         See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_execute(struct sqldbal_stmt *const stmt);

/**
 * Get the next row in the result set.
 *
 * After each @ref sqldbal_stmt_fetch call, the application can get the
 * individual columns using the sqldbal_stmt_column_* routines. The
 * return value from this function will indicate if more rows available.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return         See @ref sqldbal_fetch_result.
 */
enum sqldbal_fetch_result
sqldbal_stmt_fetch(struct sqldbal_stmt *const stmt);

/**
 * Retrieve the result column as blob/binary data.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index starting at 0.
 * @param[out] blob    Binary data.
 * @param[out] blobsz  Number of bytes in @p blob.
 * @return             See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_column_blob(struct sqldbal_stmt *const stmt,
                         size_t col_idx,
                         const void **blob,
                         size_t *blobsz);

/**
 * Retrieve the result column as an integer.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index starting at 0.
 * @param[out] i64     64-bit integer value.
 * @return             See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_column_int64(struct sqldbal_stmt *const stmt,
                          size_t col_idx,
                          int64_t *i64);

/**
 * Retrieve the result column as a string.
 *
 * @param[in]  stmt    See @ref sqldbal_stmt.
 * @param[in]  col_idx Column index starting at 0.
 * @param[out] text    Null-terminated string.
 * @param[out] textsz  Number of bytes in @p text. The caller can set this to
 *                     NULL if size not needed.
 * @return             See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_column_text(struct sqldbal_stmt *const stmt,
                         size_t col_idx,
                         const char **text,
                         size_t *textsz);

/**
 * Get the column data type.
 *
 * @note The Mariadb and PostgreSQL drivers currently only return the null
 *       and blob data types.
 *
 * @param[in] stmt    See @ref sqldbal_stmt.
 * @param[in] col_idx Column index.
 * @return            See @ref sqldbal_column_type.
 */
enum sqldbal_column_type
sqldbal_stmt_column_type(struct sqldbal_stmt *const stmt,
                         size_t col_idx);

/**
 * Free statement resources.
 *
 * @param[in] stmt See @ref sqldbal_stmt.
 * @return         See @ref sqldbal_status_code.
 */
enum sqldbal_status_code
sqldbal_stmt_close(struct sqldbal_stmt *const stmt);

#endif /* SQLDBAL_H */

