/**
 * @file
 * @brief Load database configuration files.
 * @author James Humphrey (mail@somnisoft.com)
 * @version 0.99
 *
 * Keeps the test database configuration separate from the testing framework,
 * stored in key-value files.
 *
 * This software has been placed into the public domain using CC0.
 */
#ifndef SQLDBAL_TEST_CONFIG_H
#define SQLDBAL_TEST_CONFIG_H

#include "../src/sqldbal.h"

/**
 * Relative path to directory containing the database configuration files.
 */
#define PATH_CONFIG_PREFIX "test/config"

/**
 * Database connection parameters expected by the test framework.
 */
struct sqldbal_test_db_config{
  /**
   * Driver type to connect with.
   *
   * See @ref sqldbal_driver.
   */
  enum sqldbal_driver driver;

  /**
   * See @ref sqldbal_open.
   */
  char location[100];

  /**
   * See @ref sqldbal_open.
   */
  char port[100];

  /**
   * See @ref sqldbal_open.
   */
  char username[100];

  /**
   * See @ref sqldbal_open.
   */
  char password[100];

  /**
   * See @ref sqldbal_open.
   */
  char database[100];

  /**
   * See @ref sqldbal_flag.
   */
  unsigned long flags;
};

void
sqldbal_test_load_config_file(const char *const path,
                              enum sqldbal_driver driver,
                              struct sqldbal_test_db_config *const config);

#endif /* SQLDBAL_TEST_CONFIG_H */

