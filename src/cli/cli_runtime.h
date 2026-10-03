#ifndef FLASHUX_CLI_RUNTIME_H
#define FLASHUX_CLI_RUNTIME_H
#include "cli_args.h"
#include <signal.h>
extern volatile sig_atomic_t flashux_cli_interrupted;
void flashux_cli_json_string(const char *text);
int flashux_cli_list_devices(flashux_context_t *ctx, bool include_fixed);
#endif
