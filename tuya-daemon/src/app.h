#ifndef APP_H
#define APP_H

#include <signal.h>
#include <tuyalink_core.h>

/* Command-line arguments */
struct arguments {
    char *device_id;
    char *device_secret;
    char *product_id;
    int daemon_mode;
};

/* Global MQTT client instance */
extern tuya_mqtt_context_t client_instance;

/* Flag to control the main loop */
extern volatile sig_atomic_t running;

#endif /* APP_H */
