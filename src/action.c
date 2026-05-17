#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "config.h"
#include "cJSON.h"
#include <tuyalink_core.h>

#include "action.h"
#include "ubus_invoke.h"

/* ─── Helpers ──────────────────────────────────────────────── */

/**
 * Report the result of an action back to the Tuya cloud.
 * Builds a JSON object:
 *   { "esp_action": <action>, "esp_status": "success"|"error",
 *     "esp_result": <data_or_error_message> }
 * and sends it via property report.
 */
static void report_result(tuya_mqtt_context_t *client,
                          const char *action,
                          int success,
                          const char *data)
{
    cJSON *report = cJSON_CreateObject();
    if (!report)
        return;

    cJSON_AddStringToObject(report, "esp_action", action);
    cJSON_AddStringToObject(report, "esp_status",
                            success ? "success" : "error");

    /* Try to embed the data as a parsed JSON object; fall back to string */
    if (data) {
        cJSON *parsed = cJSON_Parse(data);
        if (parsed) {
            cJSON_AddItemToObject(report, "esp_result", parsed);
        } else {
            cJSON_AddStringToObject(report, "esp_result", data);
        }
    } else {
        cJSON_AddStringToObject(report, "esp_result",
                                "No response from ESP daemon");
    }

    char *json_str = cJSON_PrintUnformatted(report);
    if (json_str) {
        syslog(LOG_SYSLOG_INFO, "Reporting to Tuya: %s", json_str);
        tuyalink_thing_property_report(client, NULL, json_str);
        free(json_str);
    }
    cJSON_Delete(report);
}

/**
 * Safely extract an integer from a cJSON value that might be
 * either a number or a numeric string.
 */
static int get_int_param(const cJSON *item, int *out)
{
    if (!item || !out)
        return -1;
    if (cJSON_IsNumber(item)) {
        *out = item->valueint;
        return 0;
    }
    if (cJSON_IsString(item) && item->valuestring) {
        char *endp;
        long v = strtol(item->valuestring, &endp, 10);
        if (endp != item->valuestring) {
            *out = (int)v;
            return 0;
        }
    }
    return -1;
}

/**
 * Safely extract a string from a cJSON value.
 */
static const char *get_str_param(const cJSON *item)
{
    if (item && cJSON_IsString(item) && item->valuestring)
        return item->valuestring;
    return NULL;
}

/* ─── Action: devices ──────────────────────────────────────── */

static void handle_devices(tuya_mqtt_context_t *client)
{
    syslog(LOG_SYSLOG_INFO, "Action 'devices': listing ESP devices");

    char *result = espd_call_devices();
    if (result) {
        syslog(LOG_SYSLOG_INFO, "Action 'devices': success — %s", result);
        report_result(client, ACTION_DEVICES, 1, result);
        free(result);
    } else {
        syslog(LOG_SYSLOG_ERR, "Action 'devices': failed to get device list");
        report_result(client, ACTION_DEVICES, 0,
                      "Failed to retrieve device list from espd");
    }
}

/* ─── Action: on ───────────────────────────────────────────── */

static void handle_on(tuya_mqtt_context_t *client, const cJSON *params)
{
    const char *port = get_str_param(cJSON_GetObjectItem(params, "port"));
    int pin;

    if (!port) {
        syslog(LOG_SYSLOG_ERR, "Action 'on': missing 'port' parameter");
        report_result(client, ACTION_ON, 0, "Missing 'port' parameter");
        return;
    }
    if (get_int_param(cJSON_GetObjectItem(params, "pin"), &pin) != 0) {
        syslog(LOG_SYSLOG_ERR, "Action 'on': missing or invalid 'pin'");
        report_result(client, ACTION_ON, 0,
                      "Missing or invalid 'pin' parameter");
        return;
    }

    syslog(LOG_SYSLOG_INFO, "Action 'on': port=%s pin=%d", port, pin);

    char *result = espd_call_on(port, pin);
    if (result) {
        syslog(LOG_SYSLOG_INFO, "Action 'on': success — %s", result);
        report_result(client, ACTION_ON, 1, result);
        free(result);
    } else {
        syslog(LOG_SYSLOG_ERR, "Action 'on': UBUS call to espd failed");
        report_result(client, ACTION_ON, 0,
                      "Failed to turn on pin via espd");
    }
}

/* ─── Action: off ──────────────────────────────────────────── */

static void handle_off(tuya_mqtt_context_t *client, const cJSON *params)
{
    const char *port = get_str_param(cJSON_GetObjectItem(params, "port"));
    int pin;

    if (!port) {
        syslog(LOG_SYSLOG_ERR, "Action 'off': missing 'port' parameter");
        report_result(client, ACTION_OFF, 0, "Missing 'port' parameter");
        return;
    }
    if (get_int_param(cJSON_GetObjectItem(params, "pin"), &pin) != 0) {
        syslog(LOG_SYSLOG_ERR, "Action 'off': missing or invalid 'pin'");
        report_result(client, ACTION_OFF, 0,
                      "Missing or invalid 'pin' parameter");
        return;
    }

    syslog(LOG_SYSLOG_INFO, "Action 'off': port=%s pin=%d", port, pin);

    char *result = espd_call_off(port, pin);
    if (result) {
        syslog(LOG_SYSLOG_INFO, "Action 'off': success — %s", result);
        report_result(client, ACTION_OFF, 1, result);
        free(result);
    } else {
        syslog(LOG_SYSLOG_ERR, "Action 'off': UBUS call to espd failed");
        report_result(client, ACTION_OFF, 0,
                      "Failed to turn off pin via espd");
    }
}

/* ─── Action: get ──────────────────────────────────────────── */

static void handle_get(tuya_mqtt_context_t *client, const cJSON *params)
{
    const char *port   = get_str_param(cJSON_GetObjectItem(params, "port"));
    const char *model  = get_str_param(cJSON_GetObjectItem(params, "model"));
    const char *sensor = get_str_param(cJSON_GetObjectItem(params, "sensor"));
    int pin;

    if (!port) {
        syslog(LOG_SYSLOG_ERR, "Action 'get': missing 'port' parameter");
        report_result(client, ACTION_GET, 0, "Missing 'port' parameter");
        return;
    }
    if (get_int_param(cJSON_GetObjectItem(params, "pin"), &pin) != 0) {
        syslog(LOG_SYSLOG_ERR, "Action 'get': missing or invalid 'pin'");
        report_result(client, ACTION_GET, 0,
                      "Missing or invalid 'pin' parameter");
        return;
    }
    if (!model) {
        syslog(LOG_SYSLOG_ERR, "Action 'get': missing 'model' parameter");
        report_result(client, ACTION_GET, 0, "Missing 'model' parameter");
        return;
    }
    if (!sensor) {
        syslog(LOG_SYSLOG_ERR, "Action 'get': missing 'sensor' parameter");
        report_result(client, ACTION_GET, 0, "Missing 'sensor' parameter");
        return;
    }

    syslog(LOG_SYSLOG_INFO,
           "Action 'get': port=%s pin=%d model=%s sensor=%s",
           port, pin, model, sensor);

    char *result = espd_call_get(port, pin, model, sensor);
    if (result) {
        syslog(LOG_SYSLOG_INFO, "Action 'get': success — %s", result);
        report_result(client, ACTION_GET, 1, result);
        free(result);
    } else {
        syslog(LOG_SYSLOG_ERR, "Action 'get': UBUS call to espd failed");
        report_result(client, ACTION_GET, 0,
                      "Failed to read sensor via espd");
    }
}

/* ─── Main dispatcher ──────────────────────────────────────── */

void process_action(const tuyalink_message_t *msg,
                    tuya_mqtt_context_t *client)
{
    if (!msg || !msg->data_string) {
        syslog(LOG_SYSLOG_WARNING, "process_action: invalid message");
        return;
    }

    cJSON *root = cJSON_Parse(msg->data_string);
    if (!root) {
        syslog(LOG_SYSLOG_ERR, "process_action: failed to parse JSON");
        return;
    }

    cJSON *action_code = cJSON_GetObjectItem(root, "actionCode");
    if (!action_code || !cJSON_IsString(action_code)) {
        syslog(LOG_SYSLOG_WARNING,
               "process_action: missing or invalid actionCode");
        cJSON_Delete(root);
        return;
    }

    const char *action = action_code->valuestring;
    syslog(LOG_SYSLOG_INFO, "Processing action: %s", action);

    /* Get inputParams — may be an object or a JSON-encoded string */
    cJSON *input_params = cJSON_GetObjectItem(root, "inputParams");
    cJSON *params_parsed = NULL;

    if (input_params && cJSON_IsString(input_params) &&
        input_params->valuestring) {
        /* inputParams is a stringified JSON — parse it */
        params_parsed = cJSON_Parse(input_params->valuestring);
        input_params = params_parsed;
    }

    /* Dispatch to the appropriate handler */
    if (strcmp(action, ACTION_DEVICES) == 0) {
        handle_devices(client);
    } else if (strcmp(action, ACTION_ON) == 0) {
        handle_on(client, input_params);
    } else if (strcmp(action, ACTION_OFF) == 0) {
        handle_off(client, input_params);
    } else if (strcmp(action, ACTION_GET) == 0) {
        handle_get(client, input_params);
    } else {
        syslog(LOG_SYSLOG_WARNING, "Unknown action: '%s'", action);
        report_result(client, action, 0, "Unknown action");
    }

    if (params_parsed)
        cJSON_Delete(params_parsed);
    cJSON_Delete(root);
}
