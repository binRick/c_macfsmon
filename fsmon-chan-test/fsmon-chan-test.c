#include "fsmon-chan-test.h"

static const char MONITORED_PATHS[][128] = {
  "/",
};
size_t            MONITORED_PATHS_QTY = sizeof(MONITORED_PATHS) / sizeof(MONITORED_PATHS[0]);

char              *msg;


TEST t_fsmon_validate(void){
  int    res = -1;
  size_t qty = fsmon_monitored_paths_qty();

  sprintf(msg, "Monitoring %lu paths", qty);
  ASSERT_EQ(qty, MONITORED_PATHS_QTY);
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
  PASS();
}


TEST t_fsmon_init(int MONITORED_PATH_INDEX){
  int res = -1;

  res = fsmon_init(MONITORED_PATHS[MONITORED_PATH_INDEX]);
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
  RUN_TESTp(t_fsmon_watch, 5);
  free(msg);
}

