#ifndef ACTION_H
#define ACTION_H

#include <tuyalink_core.h>

/**
 * Process an incoming Tuya action and execute the corresponding
 * ESP device command via the espd UBUS daemon.
 *
 * Supported actions (matching Tuya platform custom functions):
 *   "devices" (DP 101) — list connected ESP devices
 *   "on"      (DP 102) — turn on a GPIO pin
 *   "off"     (DP 103) — turn off a GPIO pin
 *   "get"     (DP 104) — read sensor data
 *
 * Results (success or error) are reported back to the Tuya cloud
 * via property report.
 *
 * @param msg    Incoming tuyalink message containing action payload
 * @param client MQTT client context for sending responses
 */
void process_action(const tuyalink_message_t *msg,
                    tuya_mqtt_context_t *client);

#endif /* ACTION_H */
