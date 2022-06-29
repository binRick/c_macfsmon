#pragma once
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
//////////////////////////////////////////////////////////////////////////////////
#include "fsmon-chan.h"
//////////////////////////////////////////////////////////////////////////////////
#define FSMON_CHAN_DEBUG_MODE    false
//////////////////////////////////////////////////////////////////////////////////
#define MAX_WORKER_THREADS       10
#define BUFFERED_EVENTS_QTY      50
//////////////////////////////////////////////////////////////////////////////////
typedef struct ctx_t                    ctx_t;
typedef struct worker_event_handler_t   worker_event_handler_t;
//////////////////////////////////////////////////////////////////////////////////
typedef enum {
  RECEIVER_MUTEX,
  CALLBACK_MUTEX,
  MUTEXES_QTY,
} mutex_type_t;
typedef enum {
  WORKER_EVENT_TYPE_RECEIVER,
  WORKER_EVENT_TYPES_QTY,
} worker_event_type_t;
typedef enum {
  EVENT_RECEIVED_CHANNEL,
  EVENTS_DONE_CHANNEL,
  EVENTS_QTY,
} worker_channel_t;
//////////////////////////////////////////////
struct worker_event_handler_t {
  const char       *name;
  worker_channel_t type;
  chan_t           *chan;
  void             *(*fxn)(void *CTX);
};
//////////////////////////////////////////////////////////////////////////////////
static void *event_receiver(void *CTX);
static int free_worker(const int WORKER_INDEX);
static int free_workers();
static int init_ctx_chans();
static int init_ctx_workers();
static int ctx_ev_cb(const struct WatchfulEvent *ev, void *CTX);

//////////////////////////////////////////////////////////////////////////////////
struct ctx_t {
  volatile size_t        processed_events_qty, received_events_qty;
  struct Vector          *monitored_paths, *excluded_paths;
  double                 monitor_delay;
  ctx_cb_t               *cb;
  WatchfulMonitor        *wm;
  fsmon_event_handler    *client_event_handler;
  pthread_mutex_t        *mu[MUTEXES_QTY];
  chan_t                 *chans[MAX_WORKER_THREADS];
  pthread_t              *threads[MAX_WORKER_THREADS];
  worker_event_handler_t worker_event_handlers[WORKER_EVENT_TYPES_QTY];
} static ctx = {
  .processed_events_qty  = 0,          .received_events_qty = 0,
  .monitored_paths       = NULL,       .excluded_paths      = NULL,
  .monitor_delay         = 0,
  .cb                    = &ctx_ev_cb,
  .wm                    = NULL,
  .client_event_handler  = NULL,
  .mu                    = NULL,
  .chans                 = NULL,
  .threads               = NULL,
  .worker_event_handlers = {
    [WORKER_EVENT_TYPE_RECEIVER] = { .name= "receiver", .type                = EVENT_RECEIVED_CHANNEL, .fxn    = event_receiver, .chan = NULL },
  },
};


//////////////////////////////////////////////////////////////////////////////////


static void *event_receiver(void *WORKER_EVENT_HANDLER){
  while (true) {
    if (FSMON_CHAN_DEBUG_MODE) {
      fprintf(stdout, ">EVENT RECEIVER> |name:%s|type:%d|qty:%d/%d|....................\n",
              ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].name,
              ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].type,
              chan_size(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan),
              ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan->queue->capacity
              );
    }
    void *EV;
    switch (chan_select(&(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan), 1, &EV, NULL, 0, NULL)) {
    case 0:
      if (FSMON_CHAN_DEBUG_MODE) {
        printf("received message! |path:%s|old_path:%s|at:%lu|type:%d|\n",
               (char *)((struct WatchfulEvent *)(EV))->path,
               (char *)((struct WatchfulEvent *)(EV))->old_path,
               (time_t)((struct WatchfulEvent *)(EV))->at,
               (int)((struct WatchfulEvent *)(EV))->type
               );
      }
      ctx.client_event_handler(
        (char *)((struct WatchfulEvent *)(EV))->path,
        (int)((struct WatchfulEvent *)(EV))->type
        );
      if (FSMON_CHAN_DEBUG_MODE) {
        printf("**DISPATCHED MSG TO EVENT HANDLER**\n");
      }
      break;
    default:
      if (FSMON_CHAN_DEBUG_MODE) {
        printf("no activity\n");
      }
    }
    sleep(1);
  }
  return(NULL);
} /* event_receiver */


static int ctx_ev_cb(const struct WatchfulEvent *ev, void *LOCAL_CTX){
  pthread_mutex_lock(ctx.mu[CALLBACK_MUTEX]);
  struct WatchfulEvent *EV_COPY = malloc(sizeof(struct WatchfulEvent));
  char                 event_type[64];
  switch (ev->type) {
  case WATCHFUL_EVENT_MODIFIED: sprintf(event_type, "MODIFIED"); break;
  case WATCHFUL_EVENT_CREATED: sprintf(event_type, "CREATED"); break;
  case WATCHFUL_EVENT_DELETED: sprintf(event_type, "DELETED"); break;
  case WATCHFUL_EVENT_RENAMED: sprintf(event_type, "RENAMED"); break;
  case WATCHFUL_EVENT_ALL: sprintf(event_type, "ALL"); break;
  case WATCHFUL_EVENT_NONE: sprintf(event_type, "NONE"); break;
  default: sprintf(event_type, "UNKNOWN"); break;
  }
  if (FSMON_CHAN_DEBUG_MODE) {
    printf("[%s] @%lu \n\t|path:%s|\n\t|old path:%s|\n",
           event_type,
           ev->at, ev->path,
           ev->old_path ? ev->old_path : "(NONE)"
           );
  }

  EV_COPY->old_path = ev->old_path ? strdup(ev->old_path) : NULL;
  EV_COPY->path     = ev->path ? strdup(ev->path) : NULL;
  EV_COPY->at       = ev->at;
  EV_COPY->type     = ev->type;
  pthread_mutex_unlock(ctx.mu[CALLBACK_MUTEX]);

  int sent = chan_send(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan, (void *)EV_COPY);
  assert(sent == 0);

  int qty = chan_size(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan);
  if (FSMON_CHAN_DEBUG_MODE) {
    printf("SENT '%s@%lu' EVENT TO CHAN! :: qty2=%d\n", event_type, EV_COPY->at, qty);
  }

  return(0);
}


static int init_ctx_chans(){
  int res = 0;

  for (int i = 0; i < MUTEXES_QTY; i++) {
    ctx.mu[i] = calloc(1, sizeof(pthread_mutex_t));
    assert(ctx.mu[i] != NULL);
    res = pthread_mutex_init(ctx.mu[i], NULL);
    assert(res == 0);
  }
  for (int i = 0; i < MAX_WORKER_THREADS; i++) {
    ctx.chans[i] = calloc(1, sizeof(chan_t));
    assert(ctx.chans[i] != NULL);
  }

  ctx.chans[EVENTS_DONE_CHANNEL] = chan_init(0);
  assert(ctx.chans[EVENTS_DONE_CHANNEL] != NULL);

  ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan = chan_init(BUFFERED_EVENTS_QTY);
  assert(ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan != NULL);

  pthread_t th;

  res = pthread_create(
    &th,
    NULL,
    ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].fxn,
    (void *)1
    );
  assert(res == 0);

  return(res);
}


static int init_ctx_workers(){
  int res = 0;

  for (int i = 0; i < MAX_WORKER_THREADS; i++) {
    ctx.threads[i] = calloc(1, sizeof(pthread_t));
    assert(ctx.threads[i] != NULL);
  }

  return(res);
}


size_t fsmon_monitored_paths_qty(){
  return((size_t)vector_size(ctx.monitored_paths));
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

  ctx.wm = watchful_monitor_create(
    &watchful_fsevents,
    (char *)vector_get(ctx.monitored_paths, 0),
    (int)vector_size(ctx.excluded_paths),
    (const char **)vector_to_array(ctx.excluded_paths),
    WATCHFUL_EVENT_ALL,
    ctx.monitor_delay,
    (ctx.cb),
    (void *)(&ctx)
    );
  assert(ctx.wm != NULL);
  res = watchful_monitor_start(ctx.wm);
  assert(res == 0);
  assert(ctx.wm->is_watching == true);
  if (FSMON_CHAN_DEBUG_MODE) {
    printf("wathing %s!\n", ctx.wm->path);
  }

  return(res);
}


int fsmon_monitor_path(const char *MONITORED_DIRECTORY){
  int res = 0;
  int ok  = vector_push(ctx.monitored_paths, (void *)MONITORED_DIRECTORY);

  assert(ok == true);
  return(res);
}


int fsmon_init(fsmon_event_handler *CLIENT_EVENT_HANDLER, const char *PATH){
  int res = -1;

  ctx.monitored_paths = vector_new();
  ctx.excluded_paths  = vector_new();

  res = init_ctx_chans();
  assert(res == 0);

  res = init_ctx_workers();
  assert(res == 0);

  res = fsmon_monitor_path(PATH);
  assert(res == 0);

  ctx.client_event_handler = (CLIENT_EVENT_HANDLER);
  assert(ctx.client_event_handler != NULL);

  return(res);
}


static int free_worker(const int WORKER_INDEX){
  return(0);
}


static int free_workers(){
  return(0);
}

