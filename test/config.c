/**
 * @file
 * @brief Load database configuration files.
 * @author James Humphrey (mail@somnisoft.com)
 * @version 0.99
 *
 * Keeps the test database configuration separate from the testing framework,
 * stored in key-value files.
 *
 * Example commands to open databases:
 *  - sqlite3 dbpath
 *  - mysql -u sqldbal-test -h localhost -p
 *  - psql -U 'sqldbal-test' -W -h localhost postgres
 *
 * This software has been placed into the public domain using CC0.
 */

#include <sys/types.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

/**
 * Load database configuration file into a @ref sqldbal_test_db_config
 * data structure.
 *
 * @param[in]  path   File path to configuration file.
 * @param[in]  driver See @ref sqldbal_driver.
 * @param[out] config See @ref sqldbal_test_db_config.
 */
void
sqldbal_test_load_config_file(const char *const path,
                              enum sqldbal_driver driver,
                              struct sqldbal_test_db_config *const config){
  FILE *fp;
  int rc;
  char *line;
  size_t len;
  ssize_t nread;
  const char *key;
  const char *value;

  memset(config, 0, sizeof(*config));
  config->driver = driver;

  fp = fopen(path, "r");
  assert(fp);

  line = NULL;
  len = 0;
  while(1){
    errno = 0;
    nread = getline(&line, &len, fp);
    if(nread < 0){
      break;
    }

    key = strtok(line, "=");
    value = strtok(NULL, "\n");
    if(strcmp(key, "location") == 0){
      strcpy(config->location, value);
    }
    else if(strcmp(key, "port") == 0){
      strcpy(config->port, value);
    }
    else if(strcmp(key, "username") == 0){
      strcpy(config->username, value);
    }
    else if(strcmp(key, "password") == 0){
      strcpy(config->password, value);
    }
    else if(strcmp(key, "database") == 0){
      strcpy(config->database, value);
    }
    else{
      errx(1, "unknown key in config file: %s\n", key);
    }
  }
  assert(ferror(fp) == 0);
  free(line);

  rc = fclose(fp);
  assert(rc == 0);
}

