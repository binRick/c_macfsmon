#include "ansi-codes/ansi-codes.h"
#include "fsmon-chan-test.h"
#include "timestamp/timestamp.h"
typedef struct TEST_CASE_T TEST_CASE_T;
struct TEST_CASE_T {
  char            *MONITORED_PATHS, *NAME, *MSG;
  size_t          MONITORED_PATHS_QTY, RECEIVED_EVENTS_QTY;
  int             MONITORED_DURATION_MS;
  fsmon_stats_t   *FSMON_STATS;
  pthread_mutex_t *mu;
} static TEST_CASES[] = {
/*  {
 *    .NAME = "Test Case A",
 *    .MONITORED_PATHS = "/", .MONITORED_PATHS_QTY = 1,
 *    .RECEIVED_EVENTS_QTY = 0, .mu = NULL, .MSG = NULL,
 *    .FSMON_STATS = NULL,
 *    .MONITORED_DURATION_MS = 500,
 * },*/
  {
    .NAME                  = "Test Case B",
    .MONITORED_PATHS       = "/", .MONITORED_PATHS_QTY = 1,
    .RECEIVED_EVENTS_QTY   = 0, .mu = NULL, .MSG = NULL,
    .FSMON_STATS           = NULL,
    .MONITORED_DURATION_MS = 10000,
  },
};


void receive_event_handler(char *PATH, int EVENT_TYPE, void *CONTEXT){
  TEST_CASE_T *test_case = (TEST_CASE_T *)CONTEXT;

  pthread_mutex_lock(test_case->mu);
  test_case->RECEIVED_EVENTS_QTY++;
  char *NAME = strdup(test_case->NAME);

  pthread_mutex_unlock(test_case->mu);

  fprintf(stderr,
          "%s> RECEIVED EVENT ON CLIENT!: |path=%s|type=%d|\n",
          NAME,
          PATH, EVENT_TYPE
          );
  free(NAME);
  return;
}


TEST t_fsmon_validate(TEST_CASE_T *test_case){
  size_t qty = fsmon_monitored_paths_qty();

  sprintf(test_case->MSG, "Monitoring %lu paths", qty);
  ASSERT_EQ(qty, TEST_CASES[0].MONITORED_PATHS_QTY);
  PASSm(test_case->MSG);
}


TEST t_fsmon_start(TEST_CASE_T *test_case){
  int res = -1;

  res = fsmon_monitor_start();
  ASSERT_EQ(res, 0);
  sprintf(test_case->MSG, "Monitoring started on test case %s", test_case->NAME);
  PASSm(test_case->MSG);
}


TEST t_fsmon_watch(TEST_CASE_T *test_case){
  unsigned long long started_ts = timestamp();

  sprintf(test_case->MSG, "Monitoring events for %dms.", test_case->MONITORED_DURATION_MS);
  printf("%s\n", test_case->MSG);
  usleep(1000 * (int)test_case->MONITORED_DURATION_MS);
  sprintf(test_case->MSG, "Monitored events for %dms.", test_case->MONITORED_DURATION_MS);
  printf("%s\n", test_case->MSG);
  unsigned long long ts = timestamp();

  sprintf(test_case->MSG, "Calling fsmon_monitor_stop()");
  printf("%s\n", test_case->MSG);
  assert(fsmon_monitor_stop() == 0);
  sprintf(test_case->MSG, "Called fsmon_monitor_stop()");
  printf("%s\n", test_case->MSG);
  fprintf(stdout,
          AC_RESETALL AC_GREEN AC_BOLD "Called fsmon_monitor_stop() in %llums.\n" AC_RESETALL,
          timestamp() - ts
          );

  fsmon_stats_t *stats = fsmon_stats();

  fprintf(stdout,
          AC_GREEN AC_INVERSE "Received %lu/%lu (%lu client events) events in %llums. Calling fsmon_monitor_stop()....\n" AC_RESETALL,
          stats->received_events_qty,
          stats->processed_events_qty,
          test_case->RECEIVED_EVENTS_QTY,
          stats->duration_ms
          );
  PASSm(test_case->MSG);
}


TEST t_fsmon_init(TEST_CASE_T *test_case){
  int res = -1;

  fprintf(stdout, "Initializing...........\n");
  res = fsmon_init(&receive_event_handler, test_case->MONITORED_PATHS, (void *)test_case);
  fprintf(stdout, "Initialized...........\n");
  ASSERT_EQ(res, 0);
  PASSm("fsmon initialized");
}

SUITE(s_test_cases){
  size_t qty = sizeof(TEST_CASES) / sizeof(TEST_CASES[0]);

  for (size_t i = 0; i < qty; i++) {
    TEST_CASES[i].MSG = calloc(1024, sizeof(char));
    TEST_CASES[i].mu  = calloc(1, sizeof(pthread_mutex_t));
    assert(TEST_CASES[i].mu != NULL);
    assert(pthread_mutex_init(TEST_CASES[i].mu, NULL) == 0);
    assert(TEST_CASES[i].mu != NULL);
    printf("Test #%lu\n", i);
    RUN_TESTp(t_fsmon_init, &(TEST_CASES[i]));
    RUN_TESTp(t_fsmon_validate, &(TEST_CASES[i]));
    RUN_TESTp(t_fsmon_start, &(TEST_CASES[i]));
    RUN_TESTp(t_fsmon_watch, &(TEST_CASES[i]));
  }
}

GREATEST_MAIN_DEFS();


int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(s_test_cases);
}

