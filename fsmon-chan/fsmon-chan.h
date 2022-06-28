#pragma once
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <inttypes.h>
#include <math.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
//////////////////////////////////////////////
#include "ansi-codes/ansi-codes.h"
#include "c_timer/include/c_timer.h"
#include "c_vector/include/vector.h"
#include "cargs/include/cargs.h"
#include "chan/src/chan.h"
#include "chan/src/queue.h"
#include "greatest/greatest.h"
#include "log.h/log.h"
#include "ms/ms.h"
#include "timestamp/timestamp.h"
//////////////////////////////////////////////
#include "fsmon.h"
//////////////////////////////////////////////
int fsmon_init(const char *MONITORED_PATH);
int fsmon_monitor_path(const char *MONITORED_DIRECTORY);
int fsmon_monitor_start();
char **fsmon_monitored_paths();
size_t fsmon_monitored_paths_qty();

//////////////////////////////////////////////
#define MAX_WORKER_THREADS     10
#define BUFFERED_EVENTS_QTY    50
//////////////////////////////////////////////
typedef struct worker_t                 worker_t;
typedef struct ctx_t                    ctx_t;
typedef struct worker_event_handler_t   worker_event_handler_t;
typedef int (ctx_cb_t)(const struct WatchfulEvent *ev, void *ctx);
//////////////////////////////////////////////
typedef enum {
  WORKER_TYPE_RECEIVER,
  WORKER_TYPE_SUBMITTER,
  WORKER_TYPES_QTY,
} worker_type_t;
typedef enum {
  EVENT_RECEIVED_CHANNEL,
  EVENT_PROCESSED_CHANNEL,
  EVENTS_DONE_CHANNEL,
  EVENTS_QTY,
} worker_channel_t;
//////////////////////////////////////////////
struct worker_event_handler_t {
  const char       *name;
  worker_channel_t type;
  chan_t           *chan;
  void             * (*fxn)(void *CTX);
};
struct worker_t {
  int                    delay_ms;
  int                    index;
  worker_type_t          type;
  pthread_t              *thread;
  worker_event_handler_t *event_handler;
};
struct ctx_t {
  volatile size_t processed_events_qty, received_events_qty;
  worker_t        *workers[MAX_WORKER_THREADS];
  chan_t          *chans[MAX_WORKER_THREADS];
  pthread_t       *threads[MAX_WORKER_THREADS];
  struct Vector   *monitored_paths;
  struct Vector   *excluded_paths;
  double          monitor_delay;
  ctx_cb_t        *cb;
  WatchfulMonitor *wm;
};
//////////////////////////////////////////////
