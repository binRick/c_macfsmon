#include "fsmon-chan.h"
//////////////////////////////////////////////////////////////////////////////////
static void *event_submitter(void *CTX);
static void *event_receiver(void *CTX);
static int init_worker(const int WORKER_INDEX, const int WORKER_TYPE);
static int free_worker(const int WORKER_INDEX);
static int free_workers();
static int init_ctx_chans();
static int init_ctx_workers();


//////////////////////////////////////////////////////////////////////////////////


static void *event_submitter(void *CTX){
  fprintf(stdout, ">EVENT SUBMITTER>\n");
  return(NULL);
}


static void *event_receiver(void *CTX){
  fprintf(stdout, ">EVENT RECEIVER>\n");
  return(NULL);
}

static worker_event_handler_t worker_event_handlers[] = {
  [WORKER_TYPE_RECEIVER] = { .name = "receiver", .type = EVENT_RECEIVED_CHANNEL, .fxn = event_receiver, .chan = NULL },
};


static int ctx_ev_cb(const struct WatchfulEvent *ev, void *ctx){
  char event_type[64];

  switch (ev->type) {
  case WATCHFUL_EVENT_MODIFIED: sprintf(event_type, "MODIFIED"); break;
  case WATCHFUL_EVENT_CREATED: sprintf(event_type, "CREATED"); break;
  case WATCHFUL_EVENT_DELETED: sprintf(event_type, "DELETED"); break;
  case WATCHFUL_EVENT_RENAMED: sprintf(event_type, "RENAMED"); break;
  case WATCHFUL_EVENT_ALL: sprintf(event_type, "ALL"); break;
  case WATCHFUL_EVENT_NONE: sprintf(event_type, "NONE"); break;
  default: sprintf(event_type, "UNKNOWN"); break;
  }
  printf("[%s] @%lu \n\t|path:%s|\n\t|old path:%s|\n\t|info:%s|\n",
         event_type,
         ev->at, ev->path,
         ev->old_path ? ev->old_path : "(NONE)",
         (char *)ctx
         );
  return(0);
}

static ctx_t ctx = {
  .processed_events_qty = 0,
  .received_events_qty  = 0,
  .chans                = NULL,
  .threads              = NULL,
  .workers              = NULL,
  .monitored_paths      = NULL,
  .excluded_paths       = NULL,
  .monitor_delay        = 0,
  .cb                   = &ctx_ev_cb,
  .wm                   = NULL,
};


static int init_worker(const int WORKER_INDEX, const int WORKER_TYPE){
  int res = 0;

  ctx.workers[WORKER_INDEX]->index               = WORKER_INDEX;
  ctx.workers[WORKER_INDEX]->type                = WORKER_TYPE;
  ctx.workers[WORKER_INDEX]->thread              = ctx.threads[WORKER_INDEX];
  ctx.workers[WORKER_INDEX]->event_handler       = &(worker_event_handlers[WORKER_TYPE]);
  ctx.workers[WORKER_INDEX]->event_handler->chan = ctx.chans[WORKER_TYPE];
  assert(res == 0);
  return(res);
}


static int init_ctx_chans(){
  int res = 0;

  ctx.chans[EVENTS_DONE_CHANNEL] = chan_init(0);
  assert(ctx.chans[EVENTS_DONE_CHANNEL] != NULL);

  ctx.chans[EVENT_RECEIVED_CHANNEL] = chan_init(BUFFERED_EVENTS_QTY);
  assert(ctx.chans[EVENT_RECEIVED_CHANNEL] != NULL);

  ctx.chans[EVENT_PROCESSED_CHANNEL] = chan_init(BUFFERED_EVENTS_QTY);
  assert(ctx.chans[EVENT_PROCESSED_CHANNEL] != NULL);

  return(res);
}


static int init_ctx_workers(){
  int res = 0;

  for (int i = 0; i < MAX_WORKER_THREADS; i++) {
    ctx.workers[i] = calloc(1, sizeof(worker_t));
    assert(ctx.workers[i] != NULL);
    ctx.chans[i] = calloc(1, sizeof(chan_t));
    assert(ctx.chans[i] != NULL);
    ctx.threads[i] = calloc(1, sizeof(pthread_t));
    assert(ctx.threads[i] != NULL);
  }

  res = init_worker(0, WORKER_TYPE_RECEIVER);
  assert(res == 0);

  return(res);
}


size_t fsmon_monitored_paths_qty(){
  return((size_t)vector_size(ctx.monitored_paths));
}


char **fsmon_monitored_paths(){
  char **paths = NULL;

  return(paths);
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
  printf("wathing %s!\n", ctx.wm->path);

  return(res);
}


int fsmon_monitor_path(const char *MONITORED_DIRECTORY){
  int res = 0;
  int ok  = vector_push(ctx.monitored_paths, (void *)MONITORED_DIRECTORY);

  assert(ok == true);
  return(res);
}


int fsmon_init(const char *path){
  int res = -1;

  ctx.monitored_paths = vector_new();
  ctx.excluded_paths  = vector_new();

  res = init_ctx_chans();
  assert(res == 0);

  res = init_ctx_workers();
  assert(res == 0);

  res = fsmon_monitor_path(path);
  assert(res == 0);

  return(res);
}


static int free_worker(const int WORKER_INDEX){
  return(0);
}


static int free_workers(){
  return(0);
}

