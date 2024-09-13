#include "postgres.h"
#include "fmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// Function to execute shell commands
static int exec_command(const char *command) {
    int result = system(command);
    if (result != 0) {
        elog(ERROR, "Command failed: %s", command);
    }
    return result;
}

// Function to configure the primary node
void configure_primary() {
    FILE *conf_file = fopen("/var/lib/pgsql/16/data/postgresql.conf", "a");
    if (conf_file == NULL) {
        elog(ERROR, "Failed to open postgresql.conf for primary. Check file path and permissions.");
        return;
    }
    fprintf(conf_file, "\nwal_level = replica\n");
    fprintf(conf_file, "max_wal_senders = 10\n");
    fprintf(conf_file, "hot_standby = on\n");

    // Ensure the changes are written to disk
    fflush(conf_file);
    fsync(fileno(conf_file));
    fclose(conf_file);

    // Reload PostgreSQL to apply the new configuration
    exec_command("/usr/pgsql-16/bin/pg_ctl reload -D /var/lib/pgsql/16/data/");
}

// Function to perform base backup for standby node
void perform_base_backup(const char *standby_dir) {
    char command[256];
    snprintf(command, sizeof(command), "/usr/pgsql-16/bin/pg_basebackup -D %s -Fp -Xs -P -R", standby_dir);
    exec_command(command);
}

// Function to configure a standby node with a specific port
void configure_standby(const char *standby_dir, const char *standby_name, int port, int primary_port) {
    char conf_path[256];
    snprintf(conf_path, sizeof(conf_path), "%s/postgresql.conf", standby_dir);

    FILE *conf_file = fopen(conf_path, "a");
    if (conf_file == NULL) {
        elog(ERROR, "Failed to open postgresql.conf for standby %s. Check file path and permissions.", standby_name);
        return;
    }
    fprintf(conf_file, "\nport = %d\n", port);
    fprintf(conf_file, "primary_conninfo = 'host=localhost port=%d user=postgres password=test'\n", primary_port);

    fflush(conf_file);
    fsync(fileno(conf_file));
    fclose(conf_file);

    // Start the standby node
    char command[256];
    snprintf(command, sizeof(command), "/usr/pgsql-16/bin/pg_ctl -D %s start", standby_dir);
    exec_command(command);
}

// Main function to set up replication with 3 nodes and different ports
PG_FUNCTION_INFO_V1(setup_replication);

Datum
setup_replication(PG_FUNCTION_ARGS) {
    int primary_port = 5432;  // Port for the primary
    int standby1_port = 5433; // Port for standby node 1
    int standby2_port = 5434; // Port for standby node 2
    elog(INFO, "Configuring primary node on port %d", primary_port);
    configure_primary();

    elog(INFO, "Performing base backup for standby node 1...");
    perform_base_backup("/var/lib/pgsql/standby_data_1");
    elog(INFO, "Performing base backup for standby node 2...");
    perform_base_backup("/var/lib/pgsql/standby_data_2");

    elog(INFO, psprintf("Configuring standby node 1 on port %d...", standby1_port));
    configure_standby("/var/lib/pgsql/standby_data_1", "standby_1", standby1_port, primary_port);

    elog(INFO, psprintf("Configuring standby node 2 on port %d...", standby2_port));
    configure_standby("/var/lib/pgsql/standby_data_2", "standby_2", standby2_port, primary_port);

    elog(INFO, "Replication setup complete for 1 primary and 2 standby nodes!");
    PG_RETURN_VOID();
}
