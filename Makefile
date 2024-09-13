MODULES = replication
EXTENSION = replication
DATA = replication--1.0.sql
PG_CONFIG = /usr/pgsql-16/bin/pg_config
PGXS = /var/lib/pgsql/16/src/postgresql-16.4/src/makefiles/pgxs.mk
include $(PGXS)
