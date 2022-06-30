#pragma once
#include "fsmon.h"
#include <stdint.h>
//////////////////////////////////////////////
typedef struct fsmon_stats_t fsmon_stats_t;
struct fsmon_stats_t {
  size_t             chan_selects_qty, received_events_qty, processed_events_qty;
  unsigned long long started_ts, ended_ts, duration_ms;
};
typedef void (fs_event_handler)(char *PATH, int EVENT_TYPE, void *CLIENT_CONTEXT);
int fsmon_init(fs_event_handler EVENT_HANDLER, const char *MONITORED_PATH, void *CLIENT_CONTEXT);
int fsmon_monitor_path(const char *MONITORED_DIRECTORY);
fsmon_stats_t *fsmon_stats();
int fsmon_monitor_start();
int fsmon_monitor_stop();
char **fsmon_monitored_paths();
size_t fsmon_monitored_paths_qty();

//////////////////////////////////////////////
