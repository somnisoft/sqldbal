/**
 * @file
 * @brief Test the SQLDBAL library.
 * @author James Humphrey (mail@somnisoft.com)
 * @version 0.99
 *
 * Test the SQLDBAL library with only the PostgreSQL driver.
 *
 * This software has been placed into the public domain using CC0.
 */

#include <assert.h>
#include <stddef.h>

#include "config.h"

/**
 * Main entry point for PostgreSQL test program.
 *
 * @retval 0 All tests passed.
 * @retval 1 Error.
 */
int main(void){
  struct sqldbal_test_db_config config;
  enum sqldbal_status_code rc;
  struct sqldbal_db *db;

  sqldbal_test_load_config_file(PATH_CONFIG_PREFIX "/postgresql.txt",
                                SQLDBAL_DRIVER_POSTGRESQL,
                                &config);
  rc = sqldbal_open(config.driver,
                    config.location,
                    config.port,
                    config.username,
                    config.password,
                    config.database,
                    config.flags,
                    NULL,
                    0,
                    &db);
  assert(rc == SQLDBAL_STATUS_OK);

  rc = sqldbal_close(db);
  assert(rc == SQLDBAL_STATUS_OK);

  return 0;
}

