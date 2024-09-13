-- replication--1.0.sql
CREATE FUNCTION setup_replication()
RETURNS void
LANGUAGE c
AS 'replication', 'setup_replication';
