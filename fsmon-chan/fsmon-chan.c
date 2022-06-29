#pragma once
//////////////////////////////////////////////
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
#include "fsmon-chan.h"
size_t CHAN_EVENT_RECEIVED_SELECTS_QTY = 0;
//////////////////////////////////////////////////////////////////////////////////
typedef int (watchful_monitor_event_callback_t)(const struct WatchfulEvent *ev, void *ctx);
//////////////////////////////////////////////////////////////////////////////////
typedef struct ctx_t                    ctx_t;
typedef struct worker_event_handler_t   worker_event_handler_t;
typedef struct chan_type_t              chan_type_t;
//////////////////////////////////////////////////////////////////////////////////
static void *event_receiver(void *);
static int
init_ctx_chans(),
init_ctx_threads(),
init_ctx_paths(),
init_ctx_mutexes(),
watchful_monitor_event_handler(const struct WatchfulEvent *ev, void *),
free_workers(),
free_worker(const int WORKER_INDEX);

//////////////////////////////////////////////////////////////////////////////////
struct ctx_t {
  enum {
    THREAD_RECEIVER,
    THREADS_QTY,
  } worker_thread_type_id_t;
  enum {
    CALLBACK_MUTEX,
    CLIENT_EVENT_MUTEX,
    CHAN_EVENT_RECEIVED_SELECTS_QTY_MUTEX,
    EVENTS_DONE_MUTEX,
    MUTEXES_QTY,
  } mutex_type_id_t;
  struct chan_type_t {
    enum chan_type_id_t {
      CHAN_EVENTS_DONE,
      CHAN_EVENT_RECEIVED,
      CHANS_QTY,
    }                         chan_type_id_t;
    const char                *name;
    size_t                    size;
    const enum chan_type_id_t type_id;
    chan_t                    *chan;
  } chan_type_t;
  struct worker_event_handler_t {
    enum {
      WORKER_EVENT_TYPE_RECEIVER,
      WORKER_EVENT_TYPES_QTY
    }                         worker_event_type_id_t;
    const char                *name;
    const enum chan_type_id_t chan_type_id;
    void                      *(*event_thread_fxn)(void *);
  }                                 worker_event_type_id_t;
  volatile size_t                   processed_events_qty, received_events_qty;
  struct Vector                     *monitored_paths_v, *excluded_paths_v;
  double                            watchful_monitor_delay;
  bool                              is_done;
  watchful_monitor_event_callback_t *watchful_monitor_event_callback;
  WatchfulMonitor                   *watchful_monitor;
  fs_event_handler                  *client_event_handler;
  pthread_mutex_t                   *mutexes[MUTEXES_QTY];
  pthread_t                         *threads[THREADS_QTY];
  worker_event_handler_t            worker_event_handlers[WORKER_EVENT_TYPES_QTY];
  chan_type_t                       chan_types[CHANS_QTY];
  chan_t                            *chans[CHANS_QTY];
  void                              *client_context;
} static ctx = {
  .processed_events_qty            = 0,                               .received_events_qty = 0,
  .monitored_paths_v               = NULL,                            .excluded_paths_v    = NULL,
  .watchful_monitor_delay          = 0,                               .is_done             = false,
  .watchful_monitor_event_callback = &watchful_monitor_event_handler,
  .watchful_monitor                = NULL,
  .client_event_handler            = NULL,
  .mutexes                         = NULL,
  .client_context                  = NULL,
  .threads                         = NULL,
  .chans                           = {
    [CHAN_EVENT_RECEIVED] = NULL,
    [CHAN_EVENTS_DONE]    = NULL,
  },
  .chan_types                      = {
    [CHAN_EVENT_RECEIVED] = { .name= "events_received", .type_id                               = CHAN_EVENT_RECEIVED, .chan                = NULL, .size = DEFAULT_BUFFERED_FILESYSTEM_EVENTS_QTY },
    [CHAN_EVENTS_DONE]    = { .name = "events_done",    .type_id                               = CHAN_EVENTS_DONE,    .chan                = NULL, .size = 0                                      },
  },
  .worker_event_handlers           = {
    [WORKER_EVENT_TYPE_RECEIVER] = { .name= "receiver", .chan_type_id                             = CHAN_EVENT_RECEIVED, .event_thread_fxn = event_receiver },
  },
};


//////////////////////////////////////////////////////////////////////////////////


static void *event_receiver(void *NONE){
  pthread_mutex_lock(ctx.mutexes[EVENTS_DONE_MUTEX]);
  bool is_done = ctx.is_done;
  pthread_mutex_unlock(ctx.mutexes[EVENTS_DONE_MUTEX]);
  void *EV;
  while (is_done == false) {
    chan_recv(ctx.chans[CHAN_EVENT_RECEIVED], &EV);
    if (FSMON_CHAN_DEBUG_MODE) {
      printf("received message! |path:%s|old_path:%s|at:%lu|type:%d|\n",
             (char *)((struct WatchfulEvent *)(EV))->path, (char *)((struct WatchfulEvent *)(EV))->old_path, (time_t)((struct WatchfulEvent *)(EV))->at, (int)((struct WatchfulEvent *)(EV))->type);
    }
    pthread_mutex_lock(ctx.mutexes[CLIENT_EVENT_MUTEX]);
    {
      ctx.client_event_handler((char *)((struct WatchfulEvent *)(EV))->path, (int)((struct WatchfulEvent *)(EV))->type, (void *)ctx.client_context);
      free(EV);
      ctx.processed_events_qty++;
    }
    pthread_mutex_unlock(ctx.mutexes[CLIENT_EVENT_MUTEX]);
    if (FSMON_CHAN_DEBUG_MODE) {
      printf("**DISPATCHED MSG TO EVENT HANDLER**\n");
    }
    pthread_mutex_lock(ctx.mutexes[CHAN_EVENT_RECEIVED_SELECTS_QTY_MUTEX]);
    CHAN_EVENT_RECEIVED_SELECTS_QTY++;
    pthread_mutex_unlock(ctx.mutexes[CHAN_EVENT_RECEIVED_SELECTS_QTY_MUTEX]);
    pthread_mutex_lock(ctx.mutexes[EVENTS_DONE_MUTEX]);
    is_done = ctx.is_done;
    pthread_mutex_unlock(ctx.mutexes[EVENTS_DONE_MUTEX]);
  }
  /*
   *
   * while (is_done == false) {
   * if (FSMON_CHAN_DEBUG_MODE) {
   *  fprintf(stdout, ">EVENT RECEIVER> |name:%s|type:%d|qty:%d/%d|....................\n",
   *          ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].name, ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].chan_type_id,
   *          chan_size(ctx.chans[CHAN_EVENT_RECEIVED]), ctx.chans[CHAN_EVENT_RECEIVED]->queue->capacity);
   * }
   *
   * switch (chan_select(ctx.chans, CHANS_QTY, &EV, NULL, 0, NULL)){
   * case CHAN_EVENTS_DONE:
   *    printf("DONE SIGNAL!\n");
   *    pthread_mutex_lock(ctx.mutexes[EVENTS_DONE_MUTEX]);
   *    ctx.is_done = true;
   *    pthread_mutex_unlock(ctx.mutexes[EVENTS_DONE_MUTEX]);
   *  break;
   * case CHAN_EVENT_RECEIVED:
   *  if (FSMON_CHAN_DEBUG_MODE) {
   *    printf("received message! |path:%s|old_path:%s|at:%lu|type:%d|\n",
   *           (char *)((struct WatchfulEvent *)(EV))->path, (char *)((struct WatchfulEvent *)(EV))->old_path, (time_t)((struct WatchfulEvent *)(EV))->at, (int)((struct WatchfulEvent *)(EV))->type);
   *  }
   *  pthread_mutex_lock(ctx.mutexes[CLIENT_EVENT_MUTEX]);
   *  {
   *    ctx.client_event_handler((char *)((struct WatchfulEvent *)(EV))->path, (int)((struct WatchfulEvent *)(EV))->type);
   *    free(EV);
   *    ctx.processed_events_qty++;
   *  }
   *  pthread_mutex_unlock(ctx.mutexes[CLIENT_EVENT_MUTEX]);
   *  if (FSMON_CHAN_DEBUG_MODE) {
   *    printf("**DISPATCHED MSG TO EVENT HANDLER**\n");
   *  }
   *  break;
   * }
   * pthread_mutex_lock(ctx.mutexes[CHAN_EVENT_RECEIVED_SELECTS_QTY_MUTEX]);
   * CHAN_EVENT_RECEIVED_SELECTS_QTY++;
   * pthread_mutex_unlock(ctx.mutexes[CHAN_EVENT_RECEIVED_SELECTS_QTY_MUTEX]);
   * pthread_mutex_lock(ctx.mutexes[EVENTS_DONE_MUTEX]);
   * is_done = ctx.is_done;
   * pthread_mutex_unlock(ctx.mutexes[EVENTS_DONE_MUTEX]);
   * }
   */
  printf("event receiver end\n");
  return(NULL);
} /* event_receiver */


static int watchful_monitor_event_handler(const struct WatchfulEvent *ev, void *NONE){
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

  assert(chan_send(ctx.chans[CHAN_EVENT_RECEIVED], (void *)EV_COPY) == 0);

  if (FSMON_CHAN_DEBUG_MODE) {
    int chan_qty = chan_size(ctx.chans[CHAN_EVENT_RECEIVED]);
    printf("SENT '%s@%lu' EVENT TO CHAN! :: qty2=%d\n", event_type, EV_COPY->at, chan_qty);
  }

  return(0);
}


static int init_ctx_mutexes(){
  for (int i = 0; i < MUTEXES_QTY; i++) {
    ctx.mutexes[i] = calloc(1, sizeof(pthread_mutex_t));
    assert(ctx.mutexes[i] != NULL);
    assert(pthread_mutex_init(ctx.mutexes[i], NULL) == 0);
  }
  return(0);
}


static int init_ctx_chans(){
  for (int i = 0; i < CHANS_QTY; i++) {
    ctx.chans[i] = calloc(1, sizeof(chan_t));
    assert(ctx.chans[i] != NULL);
    ctx.chans[i] = chan_init(ctx.chan_types[i].size);
    assert(ctx.chans[i] != NULL);
  }

  return(0);
}


static int init_ctx_paths(){
  ctx.monitored_paths_v = vector_new();
  assert(ctx.monitored_paths_v != NULL);

  ctx.excluded_paths_v = vector_new();
  assert(ctx.excluded_paths_v != NULL);


  return(0);
}


static int init_ctx_threads(){
  for (int i = 0; i < THREADS_QTY; i++) {
    ctx.threads[i] = calloc(1, sizeof(pthread_t));
    assert(ctx.threads[i] != NULL);
  }

  assert(
    pthread_create(
      ctx.threads[THREAD_RECEIVER],
      NULL,
      ctx.worker_event_handlers[WORKER_EVENT_TYPE_RECEIVER].event_thread_fxn,
      (void *)NULL
      ) == 0);

  return(0);
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

  pthread_mutex_lock(ctx.mutexes[EVENTS_DONE_MUTEX]);
  res = watchful_monitor_stop(ctx.watchful_monitor);
  assert(res == 0);
  ctx.is_done = true;
  pthread_mutex_unlock(ctx.mutexes[EVENTS_DONE_MUTEX]);
  //void *MSG;
  //assert(chan_recv(ctx.chans[CHAN_EVENTS_DONE], &MSG) == 0);
  if (FSMON_CHAN_DEBUG_MODE) {
  }
  return(res);
}


int fsmon_monitor_start(){
  ctx.watchful_monitor = watchful_monitor_create(
    &watchful_fsevents, (char *)vector_get(ctx.monitored_paths_v, 0), (int)vector_size(ctx.excluded_paths_v),
    (const char **)vector_to_array(ctx.excluded_paths_v), WATCHFUL_EVENT_ALL, ctx.watchful_monitor_delay, (ctx.watchful_monitor_event_callback), (void *)NULL
    );
  assert(ctx.watchful_monitor != NULL);
  assert(watchful_monitor_start(ctx.watchful_monitor) == 0);
  assert(ctx.watchful_monitor->is_watching == true);

  return(0);
}


int fsmon_monitor_path(const char *MONITORED_DIRECTORY){
  int ok = vector_push(ctx.monitored_paths_v, (void *)MONITORED_DIRECTORY);

  assert(ok == true);
  return(0);
}


int fsmon_init(fs_event_handler *CLIENT_EVENT_HANDLER, const char *PATH, void *CLIENT_CONTEXT){
  assert(CLIENT_EVENT_HANDLER != NULL);
  ctx.client_event_handler = (CLIENT_EVENT_HANDLER);
  ctx.client_context       = CLIENT_CONTEXT;
  assert(init_ctx_paths() == 0);
  assert(init_ctx_mutexes() == 0);
  assert(init_ctx_chans() == 0);
  assert(init_ctx_threads() == 0);
  assert(fsmon_monitor_path(PATH) == 0);

  return(0);
}


static int free_worker(const int WORKER_INDEX){
  return(0);
}


static int free_workers(){
  return(0);
}

