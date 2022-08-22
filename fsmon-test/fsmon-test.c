#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "fsmatch.h"
#include "fsmon.h"
static int ev_callback(const struct WatchfulEvent *, void *);

int main(int argc, const char **argv){
  assert(argc > 1);
  char       *cb_info           = "my_context";
  const char *monitored_path    = strdup(argv[1]);
  char       **excluded_paths   = NULL;
  size_t     excluded_paths_qty = 0;
  double     monitor_delay      = 0;
  if (argc > 2) {
    for (int i = 2; i < argc; i++) {
      printf("adding exclude..'%s'\n", argv[i]);
    }
  }
  assert(watchful_path_is_dir(monitored_path));
  WatchfulMonitor *wm = watchful_monitor_create(
    &watchful_fsevents,
    monitored_path,
    excluded_paths_qty,
    (const char **)excluded_paths,
    WATCHFUL_EVENT_ALL,
    monitor_delay,
    ev_callback,
    (void *)cb_info
    );
  assert(wm != NULL);
  assert(watchful_monitor_start(wm) == 0);
  assert(wm->is_watching == true);
  while (wm->is_watching) {
    if (false) {
      printf("WATCHING> path:%s|watching?%d|events:%d|delay:%f|backend:%s|# excludes:%lu|\n",
             wm->path, wm->is_watching, wm->events, wm->delay, wm->backend->name,
             wm->excludes->len
             );
    }
    sleep(1);
  }
  //assert(watchful_monitor_stop(wm));
  return(0);
}

static int ev_callback(const struct WatchfulEvent *ev, void *ctx){
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
