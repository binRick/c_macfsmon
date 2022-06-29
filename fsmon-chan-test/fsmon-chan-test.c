#include "fsmon-chan-test.h"

typedef struct TEST_CASE_T TEST_CASE_T;
struct TEST_CASE_T {
  char   *MONITORED_PATHS;
  size_t MONITORED_PATHS_QTY;
  int    MONITORED_DURATION;
};

const TEST_CASE_T TEST_CASES[] = {
  { .MONITORED_PATHS = "/", .MONITORED_PATHS_QTY = 1, .MONITORED_DURATION = 10, },
};
char              *msg;


void receive_event_handler(char *PATH, int EVENT_TYPE){
  printf(">RECEIVED EVENT ON CLIENT!: %s|%d\n", PATH, EVENT_TYPE);
  return;
}


TEST t_fsmon_validate(void){
  int    res = -1;
  size_t qty = fsmon_monitored_paths_qty();

  sprintf(msg, "Monitoring %lu paths", qty);
  ASSERT_EQ(qty, TEST_CASES[0].MONITORED_PATHS_QTY);
  PASSm(msg);
}


TEST t_fsmon_start(void){
  int res = -1;

  res = fsmon_monitor_start();
  ASSERT_EQ(res, 0);
  sprintf(msg, "Monitoring started");
  PASSm(msg);
}


TEST t_fsmon_watch(int DURATION){
  for (int i = 0; i < DURATION; i++) {
    sleep(1);
  }
  sprintf(msg, "Monitored events for %d seconds.", DURATION);
  PASSm(msg);
}


TEST t_fsmon_init(int MONITORED_PATH_INDEX){
  int res = -1;

  res = fsmon_init(&receive_event_handler, TEST_CASES[MONITORED_PATH_INDEX].MONITORED_PATHS);
  ASSERT_EQ(res, 0);
  PASSm("fsmon initialized");
}

GREATEST_MAIN_DEFS();


int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  msg = malloc(1024);
  RUN_TESTp(t_fsmon_init, (int)0);
  RUN_TEST(t_fsmon_validate);
  RUN_TEST(t_fsmon_start);
  RUN_TESTp(t_fsmon_watch, TEST_CASES[0].MONITORED_DURATION);
  free(msg);
}

