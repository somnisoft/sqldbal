#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "sqldbal.h"
#define DRIVER     SQLDBAL_DRIVER_SQLITE
#define LOCATION   "example.db"
#define PORT       NULL
#define USERNAME   NULL
#define PASSWORD   NULL
#define DATABASE   NULL
#define FLAGS      (SQLDBAL_FLAG_DEBUG              | \
                    SQLDBAL_FLAG_SQLITE_OPEN_CREATE | \
                    SQLDBAL_FLAG_SQLITE_OPEN_READWRITE)
int main(void){
  enum sqldbal_status_code rc;
  struct sqldbal_db *db;
  struct sqldbal_stmt *stmt;
  int64_t i64;
  const char *text;
  rc = sqldbal_open(DRIVER,
                    LOCATION,
                    PORT,
                    USERNAME,
                    PASSWORD,
                    DATABASE,
                    FLAGS,
                    NULL,
                    0,
                    &db);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_exec(db,
                    "CREATE TABLE IF NOT EXISTS test(id INTEGER, str TEXT)",
                    NULL,
                    NULL);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_prepare(db,
                            "INSERT INTO test(id, str) VALUES(?, ?)",
                            -1,
                            &stmt);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_bind_int64(stmt, 0, 10);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_bind_text(stmt, 1, "test string", -1);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_execute(stmt);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_close(stmt);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_prepare(db,
                              "SELECT id, str FROM test WHERE id = 10",
                              -1,
                              &stmt);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_stmt_execute(stmt);
  assert(rc == SQLDBAL_STATUS_OK);
  while(sqldbal_stmt_fetch(stmt) == SQLDBAL_FETCH_ROW){
    rc = sqldbal_stmt_column_int64(stmt, 0, &i64);
    assert(rc == SQLDBAL_STATUS_OK);
    rc = sqldbal_stmt_column_text(stmt, 1, &text, NULL);
    assert(rc == SQLDBAL_STATUS_OK);
    printf("%d / %s\n", (int)i64, text);
  }
  rc = sqldbal_stmt_close(stmt);
  assert(rc == SQLDBAL_STATUS_OK);
  rc = sqldbal_close(db);
  assert(rc == SQLDBAL_STATUS_OK);
  return 0;
}

