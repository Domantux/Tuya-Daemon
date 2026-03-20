#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>

/* Undefine syslog severity macros that conflict with Tuya SDK's log.h */
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_TRACE
#undef LOG_WARN
#undef LOG_ERROR
#undef LOG_FATAL

#include <tuya_log.h>

#include "config.h"
#include <tuyalink_core.h>

#include "mqtt_callbacks.h"
#include "action.h"

static volatile sig_atomic_t connected = 0;

void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
    (void)context;
    (void)user_data;
    connected = 1;
    syslog(LOG_SYSLOG_INFO, "Connected to Tuya cloud");
    TY_LOGI("on connected");
}

void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
    (void)context;
    (void)user_data;
    connected = 0;
    syslog(LOG_SYSLOG_WARNING, "Disconnected from Tuya cloud");
    TY_LOGI("on disconnect");
}

void on_messages(tuya_mqtt_context_t *context, void *user_data,
                 const tuyalink_message_t *msg)
{
    (void)user_data;

    if (!msg) {
        syslog(LOG_SYSLOG_WARNING, "on_messages: received NULL message");
        return;
    }

    TY_LOGI("on message id:%s, type:%d, code:%d", msg->msgid, msg->type,
            msg->code);
    syslog(LOG_SYSLOG_DEBUG, "Received message type: %d", msg->type);

    switch (msg->type) {
    case THING_TYPE_MODEL_RSP:
        TY_LOGI("Model data: %s", msg->data_string);
        break;

    case THING_TYPE_PROPERTY_SET:
        TY_LOGI("Property set: %s", msg->data_string);
        break;

    case THING_TYPE_PROPERTY_REPORT_RSP:
        syslog(LOG_SYSLOG_DEBUG, "Property report acknowledged");
        break;

    case THING_TYPE_ACTION_EXECUTE:
        TY_LOGI("Action execute: %s", msg->data_string);
        syslog(LOG_SYSLOG_INFO, "Action received: %s", msg->data_string);
        process_action(msg, context);
        break;

    default:
        break;
    }
}

int mqtt_is_connected(void)
{
    return connected;
}
