#!/bin/sh
##
## @file
## @brief SQLDBAL PostgreSQL certificate script.
## @author James Humphrey (humphreyj@somnisoft.com)
## @version 0.99
##
## Script used to generate PostgreSQL server and client certificates
## for testing TLS database connections.
##
## This software has been placed into the public domain using CC0.
##

# Exit script if any command returns non-zero.
set -e

# Remove older keys before replacing them.
rm -f /var/lib/pq/*.key /var/lib/pq/*.csr /var/lib/pq/*.crt

# Generate root CA private key.
openssl genrsa -out /var/lib/pq/root.key 4096

# Generate root CA public key.
openssl req -new -x509 -days 9999 -subj "/CN=localhost" -key /var/lib/pq/root.key -out /var/lib/pq/root.crt

# Generate server private key.
openssl genrsa -out /var/lib/pq/server.key 4096

# Generate root certificate signing request.
openssl req -new -key /var/lib/pq/server.key -subj "/CN=127.0.0.1" -text -out /var/lib/pq/server.csr

# Generate server certificate.
openssl x509 -req -in /var/lib/pq/server.csr -text -days 9999 -CA /var/lib/pq/root.crt -CAkey /var/lib/pq/root.key -CAcreateserial -out /var/lib/pq/server.crt

# Generate client key.
openssl genrsa -out /var/lib/pq/client.key 4096

# Generate client certificate signing request.
openssl req -new -key /var/lib/pq/client.key -subj "/CN=postgres" -out /var/lib/pq/client.csr

# Generate client certificate.
openssl x509 -req -in /var/lib/pq/client.csr -CA /var/lib/pq/root.crt -CAkey /var/lib/pq/root.key -CAcreateserial -days 9999 -text -out /var/lib/pq/client.crt

# Set database owner.
sudo /bin/chown postgres:postgres /var/lib/pq/server.key /var/lib/pq/server.crt

# Restart database.
sudo /usr/sbin/service postgresql restart

# Check log file.
sudo /usr/bin/tail /var/log/postgresql/postgresql-10-main.log

# Test database connection.
psql "postgresql://postgres@localhost:5432/sqldbal?sslmode=verify-ca&sslkey=/var/lib/pq/client.key&sslcert=/var/lib/pq/client.crt&sslrootcert=/var/lib/pq/root.crt"

