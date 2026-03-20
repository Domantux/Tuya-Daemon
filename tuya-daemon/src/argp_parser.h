#ifndef ARGP_PARSER_H
#define ARGP_PARSER_H

#include "app.h"

/**
 * Parse command-line arguments using argp.
 * Fills in the struct arguments with device_id, device_secret,
 * product_id, and daemon_mode.
 */
void parse_arguments(int argc, char **argv, struct arguments *args);

#endif /* ARGP_PARSER_H */
