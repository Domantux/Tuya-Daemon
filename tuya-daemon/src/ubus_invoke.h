#ifndef UBUS_INVOKE_H
#define UBUS_INVOKE_H

/**
 * UBUS client for invoking the espd daemon's methods.
 *
 * The espd daemon exposes the following UBUS methods:
 *   espd.devices  - list connected ESP devices
 *   espd.on       - turn on a GPIO pin
 *   espd.off      - turn off a GPIO pin
 *   espd.get      - read a sensor value
 *
 * All call functions return a malloc'd JSON string (caller must free)
 * or NULL on error.
 */

/**
 * Initialize UBUS connection for calling espd methods.
 * @return 0 on success, -1 on failure
 */
int espd_ubus_init(void);

/**
 * Cleanup UBUS connection.
 */
void espd_ubus_cleanup(void);

/**
 * Call espd "devices" method - list all connected ESP devices.
 * @return JSON string with device list, or NULL on error (caller must free)
 */
char *espd_call_devices(void);

/**
 * Call espd "on" method - turn on a GPIO pin.
 * @param port  Serial port path (e.g., "/dev/ttyUSB0")
 * @param pin   GPIO pin number
 * @return JSON string with result, or NULL on error (caller must free)
 */
char *espd_call_on(const char *port, int pin);

/**
 * Call espd "off" method - turn off a GPIO pin.
 * @param port  Serial port path (e.g., "/dev/ttyUSB0")
 * @param pin   GPIO pin number
 * @return JSON string with result, or NULL on error (caller must free)
 */
char *espd_call_off(const char *port, int pin);

/**
 * Call espd "get" method - read a sensor value.
 * @param port   Serial port path
 * @param pin    GPIO pin number
 * @param model  Sensor model (e.g., "dht11", "dht22")
 * @param sensor Sensor type (e.g., "dht")
 * @return JSON string with sensor data, or NULL on error (caller must free)
 */
char *espd_call_get(const char *port, int pin,
                    const char *model, const char *sensor);

#endif /* UBUS_INVOKE_H */
