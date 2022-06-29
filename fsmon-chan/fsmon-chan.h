#pragma once
#include "fsmon.h"
#include <stdint.h>
//////////////////////////////////////////////
typedef void (fs_event_handler)(char *PATH, int EVENT_TYPE, void *CLIENT_CONTEXT);
int fsmon_init(fs_event_handler EVENT_HANDLER, const char *MONITORED_PATH, void *CLIENT_CONTEXT);
int fsmon_monitor_path(const char *MONITORED_DIRECTORY);
int fsmon_monitor_start();
int fsmon_monitor_stop();
char **fsmon_monitored_paths();
size_t fsmon_monitored_paths_qty();

//////////////////////////////////////////////
