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
#ifndef SQLDBAL_SEAMS_H
#define SQLDBAL_SEAMS_H

#include "test.h"

/*
 * Undefine these functions and later redefine them to point to internal
 * test seam functions.
 */
#undef calloc
#undef malloc
#undef mysql_autocommit
#undef mysql_commit
#undef mysql_errno
#undef mysql_init
#undef mysql_options
#undef mysql_rollback
#undef mysql_stmt_attr_set
#undef mysql_stmt_bind_param
#undef mysql_stmt_bind_result
#undef mysql_stmt_errno
#undef mysql_stmt_execute
#undef mysql_stmt_fetch
#undef mysql_stmt_init
#undef mysql_stmt_result_metadata
#undef mysql_stmt_store_result
#undef mysql_store_result
#undef PQconnectdb
#undef PQexec
#undef PQexecParams
#undef PQprepare
#undef PQresultStatus
#undef realloc
#undef sprintf
#undef strlen
#undef strtoll
#undef strtoul
#undef sqlite3_bind_blob
#undef sqlite3_bind_int64
#undef sqlite3_bind_text
#undef sqlite3_bind_null
#undef sqlite3_close_v2
#undef sqlite3_column_blob
#undef sqlite3_errcode
#undef sqlite3_exec
#undef sqlite3_finalize
#undef sqlite3_last_insert_rowid
#undef sqlite3_reset
#undef sqlite3_step
#undef sqlite3_trace_v2

/**
 * Inject a test seam on calls to calloc() that can control
 * when this function fails.
 */
#define calloc                     sqldbal_test_seam_calloc

/**
 * Inject a test seam on calls to malloc() that can control
 * when this function fails.
 */
#define malloc                     sqldbal_test_seam_malloc

/**
 * Inject a test seam on calls to mysql_autocommit() that can control
 * when this function fails.
 */
#define mysql_autocommit           sqldbal_test_seam_mysql_autocommit

/**
 * Inject a test seam on calls to mysql_commit() that can control
 * when this function fails.
 */
#define mysql_commit               sqldbal_test_seam_mysql_commit

/**
 * Inject a test seam on calls to mysql_errno() that can control
 * when this function fails.
 */
#define mysql_errno                sqldbal_test_seam_mysql_errno

/**
 * Inject a test seam on calls to mysql_init() that can control
 * when this function fails.
 */
#define mysql_init                 sqldbal_test_seam_mysql_init

/**
 * Inject a test seam on calls to mysql_options() that can control
 * when this function fails.
 */
#define mysql_options              sqldbal_test_seam_mysql_options

/**
 * Inject a test seam on calls to mysql_rollback() that can control
 * when this function fails.
 */
#define mysql_rollback             sqldbal_test_seam_mysql_rollback

/**
 * Inject a test seam on calls to mysql_stmt_attr_set() that can control
 * when this function fails.
 */
#define mysql_stmt_attr_set        sqldbal_test_seam_mysql_stmt_attr_set

/**
 * Inject a test seam on calls to mysql_stmt_bind_param() that can control
 * when this function fails.
 */
#define mysql_stmt_bind_param      sqldbal_test_seam_mysql_stmt_bind_param

/**
 * Inject a test seam on calls to mysql_stmt_bind_result() that can control
 * when this function fails.
 */
#define mysql_stmt_bind_result     sqldbal_test_seam_mysql_stmt_bind_result

/**
 * Inject a test seam on calls to mysql_stmt_errno() that can control
 * when this function fails.
 */
#define mysql_stmt_errno           sqldbal_test_seam_mysql_stmt_errno

/**
 * Inject a test seam on calls to mysql_stmt_execute() that can control
 * when this function fails.
 */
#define mysql_stmt_execute         sqldbal_test_seam_mysql_stmt_execute

/**
 * Inject a test seam on calls to mysql_stmt_fetch() that can control
 * when this function fails.
 */
#define mysql_stmt_fetch           sqldbal_test_seam_mysql_stmt_fetch

/**
 * Inject a test seam on calls to mysql_stmt_init() that can control
 * when this function fails.
 */
#define mysql_stmt_init            sqldbal_test_seam_mysql_stmt_init

/**
 * Inject a test seam on calls to mysql_stmt_result_metadata() that can control
 * when this function fails.
 */
#define mysql_stmt_result_metadata sqldbal_test_seam_mysql_stmt_result_metadata

/**
 * Inject a test seam on calls to mysql_stmt_store_result() that can control
 * when this function fails.
 */
#define mysql_stmt_store_result    sqldbal_test_seam_mysql_stmt_store_result

/**
 * Inject a test seam on calls to mysql_store_result() that can control
 * when this function fails.
 */
#define mysql_store_result         sqldbal_test_seam_mysql_store_result

/**
 * Inject a test seam on calls to PQconnectdb() that can control
 * when this function fails.
 */
#define PQconnectdb                sqldbal_test_seam_PQconnectdb

/**
 * Inject a test seam on calls to PQexec() that can control
 * when this function fails.
 */
#define PQexec                     sqldbal_test_seam_PQexec

/**
 * Inject a test seam on calls to PQexecParams() that can control
 * when this function fails.
 */
#define PQexecParams               sqldbal_test_seam_PQexecParams

/**
 * Inject a test seam on calls to PQprepare() that can control
 * when this function fails.
 */
#define PQprepare                  sqldbal_test_seam_PQprepare

/**
 * Inject a test seam on calls to PQresultStatus() that can control
 * when this function fails.
 */
#define PQresultStatus             sqldbal_test_seam_PQresultStatus

/**
 * Inject a test seam on calls to realloc() that can control
 * when this function fails.
 */
#define realloc                    sqldbal_test_seam_realloc

/**
 * Inject a test seam on calls to sprintf() that can control
 * when this function fails.
 */
#define sprintf                    sqldbal_test_seam_sprintf

/**
 * Inject a test seam on calls to strlen() that can control
 * the return value of this function.
 */
#define strlen                     sqldbal_test_seam_strlen

/**
 * Inject a test seam on calls to strtoll() that can control
 * the return value of this function.
 */
#define strtoll                    sqldbal_test_seam_strtoll

/**
 * Inject a test seam on calls to strtoul() that can control
 * the return value of this function.
 */
#define strtoul                    sqldbal_test_seam_strtoul

/**
 * Inject a test seam on calls to sqlite3_bind_blob() that can control
 * when this function fails.
 */
#define sqlite3_bind_blob          sqldbal_test_seam_sqlite3_bind_blob

/**
 * Inject a test seam on calls to sqlite3_bind_int64() that can control
 * when this function fails.
 */
#define sqlite3_bind_int64         sqldbal_test_seam_sqlite3_bind_int64

/**
 * Inject a test seam on calls to sqlite3_bind_text() that can control
 * when this function fails.
 */
#define sqlite3_bind_text          sqldbal_test_seam_sqlite3_bind_text

/**
 * Inject a test seam on calls to sqlite3_bind_null() that can control
 * when this function fails.
 */
#define sqlite3_bind_null          sqldbal_test_seam_sqlite3_bind_null

/**
 * Inject a test seam on calls to sqlite3_close_v2() that can control
 * when this function fails.
 */
#define sqlite3_close_v2           sqldbal_test_seam_sqlite3_close_v2

/**
 * Inject a test seam on calls to sqlite3_column_blob() that can control
 * when this function fails.
 */
#define sqlite3_column_blob        sqldbal_test_seam_sqlite3_column_blob

/**
 * Inject a test seam on calls to sqlite3_column_text() that can control
 * when this function fails.
 */
#define sqlite3_column_text        sqldbal_test_seam_sqlite3_column_text

/**
 * Inject a test seam on calls to sqlite3_errcode() that can control
 * when this function fails.
 */
#define sqlite3_errcode            sqldbal_test_seam_sqlite3_errcode

/**
 * Inject a test seam on calls to sqlite3_exec() that can control
 * when this function fails.
 */
#define sqlite3_exec               sqldbal_test_seam_sqlite3_exec

/**
 * Inject a test seam on calls to sqlite3_finalize() that can control
 * when this function fails.
 */
#define sqlite3_finalize           sqldbal_test_seam_sqlite3_finalize

/**
 * Inject a test seam on calls to sqlite3_last_insert_rowid() that can control
 * when this function fails.
 */
#define sqlite3_last_insert_rowid  sqldbal_test_seam_sqlite3_last_insert_rowid

/**
 * Inject a test seam on calls to sqlite3_reset() that can control
 * when this function fails.
 */
#define sqlite3_reset              sqldbal_test_seam_sqlite3_reset

/**
 * Inject a test seam on calls to sqlite3_step() that can control
 * when this function fails.
 */
#define sqlite3_step               sqldbal_test_seam_sqlite3_step

/**
 * Inject a test seam on calls to sqlite3_trace_v2() that can control
 * when this function fails.
 */
#define sqlite3_trace_v2           sqldbal_test_seam_sqlite3_trace_v2

#endif /* SQLDBAL_SEAMS_H */

