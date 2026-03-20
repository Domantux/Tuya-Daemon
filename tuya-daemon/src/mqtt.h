#ifndef MQTT_H
#define MQTT_H

#include <tuya_error_code.h>
#include "app.h"
#include "config.h"
#include "tuya_cacert.h"
#include "mqtt_callbacks.h"

/**
 * Convenience wrapper to initialize the Tuya MQTT client
 * with embedded CA certificate and callback functions.
 */
static inline int mqtt_init_wrapper(tuya_mqtt_context_t *client,
                                    const char *device_id,
                                    const char *device_secret)
{
    return tuya_mqtt_init(client, &(const tuya_mqtt_config_t){
        .host        = TUYA_HOST,
        .port        = TUYA_PORT,
        .cacert      = (const uint8_t *)tuya_cacert_pem,
        .cacert_len  = sizeof(tuya_cacert_pem),
        .device_id   = device_id,
        .device_secret = device_secret,
        .keepalive   = TUYA_KEEPALIVE,
        .timeout_ms  = TUYA_TIMEOUT_MS,
        .on_connected  = on_connected,
        .on_disconnect = on_disconnect,
        .on_messages   = on_messages,
    });
}

#endif /* MQTT_H */
