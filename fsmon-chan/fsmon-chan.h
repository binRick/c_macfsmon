#pragma once
#include "fsmon.h"
#include <stdint.h>
//////////////////////////////////////////////
typedef int (ctx_cb_t)(const struct WatchfulEvent *ev, void *ctx);
//////////////////////////////////////////////
typedef void (fsmon_event_handler)(char *PATH, int EVENT_TYPE);
int fsmon_init(fsmon_event_handler EVENT_HANDLER, const char *MONITORED_PATH);
int fsmon_monitor_path(const char *MONITORED_DIRECTORY);
int fsmon_monitor_start();
int fsmon_monitor_stop();
char **fsmon_monitored_paths();
size_t fsmon_monitored_paths_qty();

//////////////////////////////////////////////
