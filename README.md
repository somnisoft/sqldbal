# SQLDBAL - SQL Database Abstraction Library
SQL database abstraction library written in C which can get included
directly into another program. Each driver has been implemented
using the client connector library provided by the corresponding
database system.

## Feature List
* Single C and header file
* Cross-platform (POSIX, BSD, Windows)
* Simple API that handles most common database operations
  (driver-specific database/statement handles available if needed)
* Test cases with 100% code and branch coverage
* Doxygen with 100% documentation
* Free software (permissive - CC0)

Supports the following database engines:
* MariaDB/mySQL (requires libmariadbclient or libmysqlclient)
* PostgreSQL (requires libpq)
* SQLite (requires libsqlite3)

To include the library into your application, simply copy the src/sqldbal.h
and src/sqldbal.c files into your project directory. Then include the
sqldbal.h header into your C file, compile sqldbal.c, and include the
resulting object file into the build system.

## Examples
The following example code demonstrates how to use this library with the
SQLite driver. This example opens a database file, creates a table,
inserts a record, and then reads back the inserted records.

```C
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
```

Place the code snippet above into a file named 'example.c' and change
each #define to the appropriate values for your database. Then copy sqldbal.c
and sqldbal.h into the same directory and run the following commands to
compile with support for all databases.

cc -DSQLDBAL_MARIADB -DSQLDBAL_POSTGRESQL -DSQLDBAL_SQLITE sqldbal.c -c -o sqldbal.o

cc -DSQLDBAL_MARIADB -DSQLDBAL_POSTGRESQL -DSQLDBAL_SQLITE example.c -c -o example.o

cc example.o sqldbal.o -o sqldbal_test -lsqlite3 -lpq -lmariadbclient

Remove the corresponding -D and -l arguments for the database drivers that
you do not need to use. The commands as above should create an
executable called 'sqldbal_test'.

## Technical Documentation
See the
[Technical Documentation](https://www.somnisoft.com/sqldbal/technical-documentation/index.html)
generated from Doxygen.

