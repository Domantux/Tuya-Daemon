#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <errno.h>

#include "config.h"
#include "tuya_cacert.h"
#include <tuya_error_code.h>

#include "app.h"
#include "argp_parser.h"
#include "mqtt.h"
#include "mqtt_callbacks.h"
#include "ubus_invoke.h"
#include "daemon.h"

#ifndef VERSION_STRING
#define VERSION_STRING "1.0.0"
#endif

/* Global state */
tuya_mqtt_context_t client_instance;
volatile sig_atomic_t running = 1;

static const char *get_error_description(int error_code)
{
    switch (error_code) {
    case -7694:
        return "Authentication failed — invalid device_id, product_id, or device_secret";
    case -1:
        return "Connection failed — check network and Tuya cloud status";
    case -2:
        return "MQTT initialization failed";
    case -3:
        return "Invalid parameters";
    default:
        return "Unknown error";
    }
}

int main(int argc, char **argv)
{
    char device_id[MAX_CREDENTIAL_LEN]     = "";
    char device_secret[MAX_CREDENTIAL_LEN] = "";
    char product_id[MAX_CREDENTIAL_LEN]    = "";

    /* Parse command-line arguments */
    struct arguments args;
    parse_arguments(argc, argv, &args);

    /* Open syslog early */
    openlog("tuya_espd", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_SYSLOG_INFO,
           "Starting tuya_espd v%s (PID %d)", VERSION_STRING, getpid());

    /* Check for another instance */
    if (daemon_check_and_write_pid() != 0) {
        closelog();
        return EXIT_FAILURE;
    }

    /* Copy credentials */
    if (args.device_id)
        strncpy(device_id, args.device_id, sizeof(device_id) - 1);
    if (args.product_id)
        strncpy(product_id, args.product_id, sizeof(product_id) - 1);
    if (args.device_secret)
        strncpy(device_secret, args.device_secret, sizeof(device_secret) - 1);

    /* Verify credentials */
    if (device_id[0] == '\0' || product_id[0] == '\0' ||
        device_secret[0] == '\0') {
        syslog(LOG_SYSLOG_ERR, "Cannot start: missing credentials");
        syslog(LOG_SYSLOG_ERR,
               "Usage: tuya_espd -d <device_id> -s <secret> -p <product_id> [-D]");
        daemon_remove_pid();
        closelog();
        return EXIT_FAILURE;
    }

    syslog(LOG_SYSLOG_INFO, "All credentials present, proceeding with startup");

    /* Daemonize if requested */
    if (args.daemon_mode) {
        closelog();
        if (daemon_daemonize() != 0)
            return EXIT_FAILURE;
        openlog("tuya_espd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
        daemon_rewrite_pid();
        syslog(LOG_SYSLOG_INFO,
               "Daemon started successfully (PID %d)", getpid());
    }

    daemon_setup_signals();

    /* ── Initialise UBUS (for calling espd) ── */
    if (espd_ubus_init() != 0) {
        syslog(LOG_SYSLOG_WARNING,
               "Initial UBUS connection failed — will retry when needed");
    } else {
        syslog(LOG_SYSLOG_INFO, "UBUS connection to espd established");
    }

    /* ── Initialise Tuya MQTT ── */
    int ret = 0;
    tuya_mqtt_context_t *client = &client_instance;
    int exit_code = EXIT_FAILURE;

    syslog(LOG_SYSLOG_INFO, "Initializing MQTT connection to Tuya cloud...");
    ret = mqtt_init_wrapper(client, device_id, device_secret);
    if (ret != OPRT_OK) {
        syslog(LOG_SYSLOG_ERR, "MQTT init failed: %d (%s)",
               ret, get_error_description(ret));
        goto cleanup;
    }

    ret = tuya_mqtt_connect(client);
    if (ret != OPRT_OK) {
        syslog(LOG_SYSLOG_ERR, "MQTT connect failed: %d (%s)",
               ret, get_error_description(ret));
        goto cleanup;
    }

    syslog(LOG_SYSLOG_INFO, "MQTT connection initiated, entering main loop");
    syslog(LOG_SYSLOG_INFO,
           "Waiting for ESP device control actions from Tuya cloud...");

    time_t last_reconnect_attempt = time(NULL);
    int reconnect_backoff = MQTT_RECONNECT_INITIAL_SEC;

    /* ── Main loop ── */
    while (running) {
        tuya_mqtt_loop(client);

        time_t now = time(NULL);
        if (now == (time_t)-1) {
            syslog(LOG_SYSLOG_WARNING, "time() failed: %s", strerror(errno));
            usleep(USLEEP_USEC);
            continue;
        }

        /* MQTT reconnection with exponential backoff */
        if (!mqtt_is_connected() &&
            now - last_reconnect_attempt >= reconnect_backoff) {
            syslog(LOG_SYSLOG_INFO,
                   "Attempting MQTT reconnection (backoff: %ds)...",
                   reconnect_backoff);
            ret = tuya_mqtt_connect(client);
            last_reconnect_attempt = now;
            if (ret != OPRT_OK) {
                syslog(LOG_SYSLOG_WARNING,
                       "MQTT reconnect failed (code %d), next retry in %ds",
                       ret, reconnect_backoff);
                if (reconnect_backoff < MQTT_RECONNECT_MAX_SEC)
                    reconnect_backoff *= 2;
            } else {
                syslog(LOG_SYSLOG_INFO, "MQTT reconnection initiated");
                reconnect_backoff = MQTT_RECONNECT_INITIAL_SEC;
            }
        }

        usleep(USLEEP_USEC);
    }

    exit_code = EXIT_SUCCESS;
    syslog(LOG_SYSLOG_INFO, "Main loop exited normally");

cleanup:
    syslog(LOG_SYSLOG_INFO, "Shutting down...");

    tuya_mqtt_disconnect(client);
    tuya_mqtt_deinit(client);
    syslog(LOG_SYSLOG_INFO, "MQTT disconnected");

    espd_ubus_cleanup();

    daemon_remove_pid();
    syslog(LOG_SYSLOG_INFO, "tuya_espd stopped (exit code: %d)", exit_code);
    syslog(LOG_SYSLOG_INFO, "----------------------------------------------");
    closelog();

    return exit_code;
}
