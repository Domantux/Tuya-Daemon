#include <argp.h>
#include <stdlib.h>
#include <string.h>

#include "argp_parser.h"
#include "app.h"

const char *argp_program_version =
    "tuya_espd 1.0.0 (built " __DATE__ " " __TIME__ ")";
const char *argp_program_bug_address = "<support@example.com>";

static char doc[] =
    "Tuya-ESP Bridge Daemon — controls ESP devices via Tuya IoT cloud";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"device-id",     'd', "ID",     0, "Tuya device ID",     0},
    {"device-secret", 's', "SECRET", 0, "Tuya device secret", 0},
    {"product-id",    'p', "ID",     0, "Tuya product ID",    0},
    {"daemon",        'D', 0,        0, "Run as daemon",      0},
    {0}
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    switch (key) {
    case 'd':
        arguments->device_id = arg;
        break;
    case 's':
        arguments->device_secret = arg;
        break;
    case 'p':
        arguments->product_id = arg;
        break;
    case 'D':
        arguments->daemon_mode = 1;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};

void parse_arguments(int argc, char **argv, struct arguments *args)
{
    memset(args, 0, sizeof(struct arguments));
    argp_parse(&argp, argc, argv, 0, 0, args);
}
