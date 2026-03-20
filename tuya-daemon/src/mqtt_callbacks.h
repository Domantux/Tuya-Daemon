#ifndef MQTT_CALLBACKS_H
#define MQTT_CALLBACKS_H

#include <tuyalink_core.h>

/**
 * Callback for MQTT connected event
 */
void on_connected(tuya_mqtt_context_t *context, void *user_data);

/**
 * Callback for MQTT disconnect event
 */
void on_disconnect(tuya_mqtt_context_t *context, void *user_data);

/**
 * Callback for incoming MQTT messages (actions from Tuya cloud)
 */
void on_messages(tuya_mqtt_context_t *context, void *user_data,
                 const tuyalink_message_t *msg);

/**
 * Check if MQTT is currently connected
 * @return 1 if connected, 0 if disconnected
 */
int mqtt_is_connected(void);

#endif /* MQTT_CALLBACKS_H */
