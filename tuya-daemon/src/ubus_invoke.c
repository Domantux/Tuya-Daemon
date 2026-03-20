#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <libubus.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>

#include "ubus_invoke.h"
#include "config.h"

static struct ubus_context *ctx = NULL;
static struct blob_buf b;

/**
 * Generic UBUS invoke callback.
 * Converts the response blob to a JSON string and stores it in req->priv.
 */
static void invoke_cb(struct ubus_request *req,
                      int type __attribute__((unused)),
                      struct blob_attr *msg)
{
    char **result = (char **)req->priv;
    if (msg && result) {
        free(*result);
        *result = blobmsg_format_json(msg, true);
    }
}

/**
 * Ensure we have a live UBUS connection.
 * Reconnects automatically if the previous connection was lost.
 */
static int ensure_connected(void)
{
    if (ctx) {
        /* Quick liveness check: try to look up a known object.
         * If the fd is dead ubus_lookup_id will fail and we reconnect. */
        uint32_t dummy;
        if (ubus_lookup_id(ctx, "espd", &dummy) == 0)
            return 0;

        /* Connection seems dead, free and retry */
        syslog(LOG_SYSLOG_WARNING, "UBUS connection lost, reconnecting...");
        ubus_free(ctx);
        ctx = NULL;
    }

    ctx = ubus_connect(NULL);
    if (!ctx) {
        syslog(LOG_SYSLOG_ERR, "Failed to connect to UBUS");
        return -1;
    }
    syslog(LOG_SYSLOG_INFO, "UBUS connection established");
    return 0;
}

/**
 * Invoke an espd UBUS method and return the JSON response.
 * @param method  Method name ("devices", "on", "off", "get")
 * @param msg     Blob message with arguments (may be NULL)
 * @return JSON string (caller frees) or NULL on error
 */
static char *invoke_espd(const char *method, struct blob_attr *msg)
{
    uint32_t id;
    char *result = NULL;

    if (ensure_connected() != 0)
        return NULL;

    if (ubus_lookup_id(ctx, "espd", &id)) {
        syslog(LOG_SYSLOG_ERR,
               "UBUS object 'espd' not found — is the espd daemon running?");
        return NULL;
    }

    int ret = ubus_invoke(ctx, id, method, msg, invoke_cb, &result,
                          UBUS_CALL_TIMEOUT_MS);
    if (ret) {
        syslog(LOG_SYSLOG_ERR, "UBUS call espd.%s failed: %s (code %d)",
               method, ubus_strerror(ret), ret);
        free(result);
        return NULL;
    }

    syslog(LOG_SYSLOG_DEBUG, "UBUS call espd.%s succeeded", method);
    return result;
}

/* ─── Public API ───────────────────────────────────────────── */

int espd_ubus_init(void)
{
    ctx = ubus_connect(NULL);
    if (!ctx) {
        syslog(LOG_SYSLOG_WARNING,
               "Initial UBUS connection failed — will retry on first call");
        return -1;
    }
    syslog(LOG_SYSLOG_INFO, "UBUS connection established");
    return 0;
}

void espd_ubus_cleanup(void)
{
    if (ctx) {
        ubus_free(ctx);
        ctx = NULL;
    }
    blob_buf_free(&b);
    syslog(LOG_SYSLOG_INFO, "UBUS connection closed");
}

char *espd_call_devices(void)
{
    syslog(LOG_SYSLOG_INFO, "Calling espd.devices via UBUS");
    return invoke_espd("devices", NULL);
}

char *espd_call_on(const char *port, int pin)
{
    syslog(LOG_SYSLOG_INFO, "Calling espd.on via UBUS (port=%s, pin=%d)",
           port, pin);

    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "port", port);
    blobmsg_add_u32(&b, "pin", (uint32_t)pin);
    return invoke_espd("on", b.head);
}

char *espd_call_off(const char *port, int pin)
{
    syslog(LOG_SYSLOG_INFO, "Calling espd.off via UBUS (port=%s, pin=%d)",
           port, pin);

    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "port", port);
    blobmsg_add_u32(&b, "pin", (uint32_t)pin);
    return invoke_espd("off", b.head);
}

char *espd_call_get(const char *port, int pin,
                    const char *model, const char *sensor)
{
    syslog(LOG_SYSLOG_INFO,
           "Calling espd.get via UBUS (port=%s, pin=%d, model=%s, sensor=%s)",
           port, pin, model, sensor);

    blob_buf_init(&b, 0);
    blobmsg_add_string(&b, "port", port);
    blobmsg_add_u32(&b, "pin", (uint32_t)pin);
    blobmsg_add_string(&b, "model", model);
    blobmsg_add_string(&b, "sensor", sensor);
    return invoke_espd("get", b.head);
}
