#ifndef CONFIG_H
#define CONFIG_H

/*
 * Syslog severity levels (from POSIX syslog.h)
 * Define these directly to avoid conflicts with Tuya SDK's log.h
 */
#define LOG_SYSLOG_DEBUG   7
#define LOG_SYSLOG_INFO    6
#define LOG_SYSLOG_WARNING 4
#define LOG_SYSLOG_ERR     3

/* Tuya MQTT connection settings */
#define TUYA_HOST       "m1.tuyacn.com"
#define TUYA_PORT       8883
#define TUYA_KEEPALIVE  100
#define TUYA_TIMEOUT_MS 2000

/* Main loop sleep interval (microseconds) */
#define USLEEP_USEC 100000

/* Maximum credential string length */
#define MAX_CREDENTIAL_LEN 64

/* MQTT reconnection parameters */
#define MQTT_RECONNECT_INITIAL_SEC 5
#define MQTT_RECONNECT_MAX_SEC     300

/* UBUS call timeout (milliseconds) */
#define UBUS_CALL_TIMEOUT_MS 10000

/* Action identifiers (must match Tuya platform custom function identifiers) */
#define ACTION_DEVICES "devices"
#define ACTION_ON      "on"
#define ACTION_OFF     "off"
#define ACTION_GET     "get"

#endif /* CONFIG_H */
