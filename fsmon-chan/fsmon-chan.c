#pragma once
#define FSMON_CHAN_DEBUG_MODE                     false
#define DEFAULT_BUFFERED_FILESYSTEM_EVENTS_QTY    50
//////////////////////////////////////////////
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
//////////////////////////////////////////////
#include "c_vector/include/vector.h"
#include "chan/src/chan.h"
#include "chan/src/queue.h"
//////////////////////////////////////////////////////////////////////////////////
#include "fsmon-chan.h"
//////////////////////////////////////////////////////////////////////////////////
typedef struct ctx_t                    ctx_t;
typedef struct worker_event_handler_t   worker_event_handler_t;
typedef struct chan_type_t              chan_type_t;
typedef int (watchful_monitor_event_callback_t)(const struct WatchfulEvent *ev, void *ctx);
//////////////////////////////////////////////////////////////////////////////////
typedef enum {
  CALLBACK_MUTEX,
  CLIENT_EVENT_MUTEX,
  MUTEXES_QTY,
} mutex_type_id_t;
typedef enum {
  THREAD_RECEIVER,
  THREADS_QTY,
} worker_thread_type_id_t;
typedef enum {
  WORKER_EVENT_TYPE_RECEIVER,
  WORKER_EVENT_TYPES_QTY,
} worker_event_type_id_t;
typedef enum {
  CHAN_EVENT_RECEIVED,
  CHAN_EVENTS_DONE,
  CHANS_QTY,
} chan_type_id_t;
//////////////////////////////////////////////
struct chan_type_t {
  const char           *name;
  size_t               size;
  const chan_type_id_t type_id;
  chan_t               *chan;
};
struct worker_event_handler_t {
  const char           *name;
  const chan_type_id_t chan_type_id;
  void                 *(*event_thread_fxn)(void *);
};
//////////////////////////////////////////////////////////////////////////////////
static void *event_receiver(void *);
static int free_worker(const int WORKER_INDEX);
static int free_workers();
static int init_ctx_chans();
static int init_ctx_threads();
static int watchful_monitor_event_handler(const struct WatchfulEvent *ev, void *);

//////////////////////////////////////////////////////////////////////////////////
struct ctx_t {
  volatile size_t                   processed_events_qty, received_events_qty;
  struct Vector                     *monitored_paths_v, *excluded_paths_v;
  double                            watchful_monitor_delay;
  watchful_monitor_event_callback_t *watchful_monitor_event_callback;
  WatchfulMonitor                   *watchful_monitor;
  fs_event_handler                  *client_event_handler;
  pthread_mutex_t                   *mutexes[MUTEXES_QTY];
  pthread_t                         *threads[THREADS_QTY];
  chan_type_t                       chans[CHANS_QTY];
  worker_event_handler_t            worker_event_handlers[WORKER_EVENT_TYPES_QTY];
} static ctx = {
  .processed_events_qty            = 0,                               .received_events_qty = 0,
  .monitored_paths_v               = NULL,                            .excluded_paths_v    = NULL,
  .watchful_monitor_delay          = 0,
  .watchful_monitor_event_callback = &watchful_monitor_event_handler,
  .watchful_monitor                = NULL,
  .client_event_handler            = NULL,
  .mutexes                         = NULL,
  .threads                         = NULL,
  .chans                           = {
    [CHAN_EVENT_RECEIVED] = { .name= "events_received", .type_id                               = CHAN_EVENT_RECEIVED, .chan                = NULL, .size = DEFAULT_BUFFERED_FILESYSTEM_EVENTS_QTY },
    [CHAN_EVENTS_DONE]    = { .name = "events_done",    .type_id                               = CHAN_EVENTS_DONE,    .chan                = NULL, .size = 0                                      },
  },
  .worker_event_handlers           = {
    [WORKER_EVENT_TYPE_RECEIVER] = { .name= "receiver", .chan_type_id                             = CHAN_EVENT_RECEIVED, .event_thread_fxn = event_receiver },
  },
};


//////////////////////////////////////////////////////////////////////////////////


static void *event_receiver(void *NONE){
  while (true) {
    if (FSMON_CHAN_DEBUG_MODE) {
      fprintf(stdout, ">EVENT RECEIVER> |name:%s|type:%d|qty:%d/%d|....................\n",
              ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].name, ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan_type_id,
              chan_size(ctx.chans[CHAN_EVENT_RECEIVED].chan), ctx.chans[CHAN_EVENT_RECEIVED].chan->queue->capacity);
    }
    void *EV;
    switch (chan_select(&(ctx.chans[CHAN_EVENT_RECEIVED].chan), 1, &EV, NULL, 0, NULL)) {
    case 0:
      if (FSMON_CHAN_DEBUG_MODE) {
        printf("received message! |path:%s|old_path:%s|at:%lu|type:%d|\n",
               (char *)((struct WatchfulEvent *)(EV))->path, (char *)((struct WatchfulEvent *)(EV))->old_path, (time_t)((struct WatchfulEvent *)(EV))->at, (int)((struct WatchfulEvent *)(EV))->type);
      }
      pthread_mutex_lock(ctx.mutexes[CLIENT_EVENT_MUTEX]);
      {
        ctx.client_event_handler((char *)((struct WatchfulEvent *)(EV))->path, (int)((struct WatchfulEvent *)(EV))->type);
        ctx.processed_events_qty++;
      }
      pthread_mutex_unlock(ctx.mutexes[CLIENT_EVENT_MUTEX]);
      if (FSMON_CHAN_DEBUG_MODE) {
        printf("**DISPATCHED MSG TO EVENT HANDLER**\n");
      }
      break;
    default:
      if (FSMON_CHAN_DEBUG_MODE) {
        printf("no activity\n");
      }
      usleep(1000 * 1000);
    }
    usleep(1000 * 10);
  }
  return(NULL);
} /* event_receiver */


static int watchful_monitor_event_handler(const struct WatchfulEvent *ev, void *){
  char                 event_type[64];
  struct WatchfulEvent *EV_COPY = malloc(sizeof(struct WatchfulEvent));

  pthread_mutex_lock(ctx.mutexes[CALLBACK_MUTEX]);
  {
    ctx.received_events_qty++;
    switch (ev->type) {
    case WATCHFUL_EVENT_MODIFIED: sprintf(event_type, "MODIFIED"); break;
    case WATCHFUL_EVENT_CREATED: sprintf(event_type, "CREATED"); break;
    case WATCHFUL_EVENT_DELETED: sprintf(event_type, "DELETED"); break;
    case WATCHFUL_EVENT_RENAMED: sprintf(event_type, "RENAMED"); break;
    case WATCHFUL_EVENT_ALL: sprintf(event_type, "ALL"); break;
    case WATCHFUL_EVENT_NONE: sprintf(event_type, "NONE"); break;
    default: sprintf(event_type, "UNKNOWN"); break;
    }

    EV_COPY->old_path = ev->old_path ? strdup(ev->old_path) : NULL;
    EV_COPY->path     = ev->path ? strdup(ev->path) : NULL;
    EV_COPY->at       = ev->at;
    EV_COPY->type     = ev->type;
  }
  pthread_mutex_unlock(ctx.mutexes[CALLBACK_MUTEX]);
  if (FSMON_CHAN_DEBUG_MODE) {
    printf("[%s] @%lu \n\t|path:%s|\n\t|old path:%s|\n", event_type, EV_COPY->at, EV_COPY->path, EV_COPY->old_path ? EV_COPY->old_path : "(NONE)");
  }

  int send_res = chan_send(ctx.chans[CHAN_EVENT_RECEIVED].chan, (void *)EV_COPY);

  assert(send_res == 0);

  if (FSMON_CHAN_DEBUG_MODE) {
    int chan_qty = chan_size(ctx.chans[CHAN_EVENT_RECEIVED].chan);
    printf("SENT '%s@%lu' EVENT TO CHAN! :: qty2=%d\n", event_type, EV_COPY->at, chan_qty);
  }

  return(0);
}


static int init_ctx_chans(){
  int res = 0;

  for (int i = 0; i < MUTEXES_QTY; i++) {
    ctx.mutexes[i] = calloc(1, sizeof(pthread_mutex_t));
    assert(ctx.mutexes[i] != NULL);
    res = pthread_mutex_init(ctx.mutexes[i], NULL);
    assert(res == 0);
  }
  for (int i = 0; i < CHANS_QTY; i++) {
    ctx.chans[i].chan = calloc(1, sizeof(chan_t));
    assert(ctx.chans[i].chan != NULL);
    ctx.chans[i].chan = chan_init(ctx.chans[i].size);
    assert(ctx.chans[i].chan != NULL);
  }

  return(res);
}


static int init_ctx_threads(){
  int res = 0;

  for (int i = 0; i < THREADS_QTY; i++) {
    ctx.threads[i] = calloc(1, sizeof(pthread_t));
    assert(ctx.threads[i] != NULL);
  }

  res = pthread_create(
    ctx.threads[THREAD_RECEIVER],
    NULL,
    ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].event_thread_fxn,
    (void *)NULL
    );
  assert(res == 0);


  return(res);
}


size_t fsmon_monitored_paths_qty(){
  return((size_t)vector_size(ctx.monitored_paths_v));
}


char **fsmon_monitored_paths(){
  char **paths = NULL;

  return(paths);
}


int fsmon_monitor_stop(){
  int res = 0;

//  int sent = chan_send(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan, (void *)EV_COPY);
// assert(sent == 0);

  //int qty = chan_size(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan);
  if (FSMON_CHAN_DEBUG_MODE) {
//    printf("SENT '%s@%lu' EVENT TO CHAN! :: qty2=%d\n", event_type, EV_COPY->at, qty);
  }

  return(res);
}


int fsmon_monitor_start(){
  int res = 0;

  ctx.watchful_monitor = watchful_monitor_create(
    &watchful_fsevents, (char *)vector_get(ctx.monitored_paths_v, 0), (int)vector_size(ctx.excluded_paths_v),
    (const char **)vector_to_array(ctx.excluded_paths_v), WATCHFUL_EVENT_ALL, ctx.watchful_monitor_delay, (ctx.watchful_monitor_event_callback), (void *)NULL
    );
  assert(ctx.watchful_monitor != NULL);

  res = watchful_monitor_start(ctx.watchful_monitor);
  assert(res == 0);
  assert(ctx.watchful_monitor->is_watching == true);

  return(res);
}


int fsmon_monitor_path(const char *MONITORED_DIRECTORY){
  int ok = vector_push(ctx.monitored_paths_v, (void *)MONITORED_DIRECTORY);

  assert(ok == true);
  return(0);
}


int fsmon_init(fs_event_handler *CLIENT_EVENT_HANDLER, const char *PATH){
  int res = -1;

  ctx.client_event_handler = (CLIENT_EVENT_HANDLER);
  assert(ctx.client_event_handler != NULL);

  ctx.monitored_paths_v = vector_new();
  assert(ctx.monitored_paths_v != NULL);

  ctx.excluded_paths_v = vector_new();
  assert(ctx.excluded_paths_v != NULL);

  res = init_ctx_chans();
  assert(res == 0);

  res = init_ctx_threads();
  assert(res == 0);

  res = fsmon_monitor_path(PATH);
  assert(res == 0);

  return(res);
}


static int free_worker(const int WORKER_INDEX){
  return(0);
}


static int free_workers(){
  return(0);
}

