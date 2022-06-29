#include "ansi-codes/ansi-codes.h"
#include "fsmon-chan-test.h"
#include "timestamp/timestamp.h"
typedef struct TEST_CASE_T TEST_CASE_T;
extern size_t CHAN_EVENT_RECEIVED_SELECTS_QTY;
struct TEST_CASE_T {
  char            *MONITORED_PATHS, *NAME, *MSG;
  size_t          MONITORED_PATHS_QTY, RECEIVED_EVENTS_QTY;
  int             MONITORED_DURATION_MS;
  pthread_mutex_t *mu;
} static TEST_CASES[] = {
  { .NAME = "a", .MONITORED_PATHS = "/", .MONITORED_PATHS_QTY = 1, .MONITORED_DURATION_MS = 2000, .RECEIVED_EVENTS_QTY = 0, .mu = NULL, .MSG = NULL, },
  { .NAME = "b", .MONITORED_PATHS = "/", .MONITORED_PATHS_QTY = 1, .MONITORED_DURATION_MS = 5000, .RECEIVED_EVENTS_QTY = 0, .mu = NULL, .MSG = NULL },
};


void receive_event_handler(char *PATH, int EVENT_TYPE, void *CONTEXT){
  TEST_CASE_T *test_case = (TEST_CASE_T *)CONTEXT;

  fprintf(stderr,
          "%s> RECEIVED EVENT ON CLIENT!: |path=%s|type=%d|\n",
          test_case->NAME,
          PATH, EVENT_TYPE
          );
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
  sprintf(test_case->MSG, "Monitored events for %dms.", test_case->MONITORED_DURATION_MS);
  fprintf(stdout, "CHAN_EVENT_RECEIVED_SELECTS_QTY:%lu\n", CHAN_EVENT_RECEIVED_SELECTS_QTY);
  fprintf(stdout, "calling stop...\n");
  unsigned long long ts = timestamp();
  assert(fsmon_monitor_stop() == 0);
  fprintf(stdout,
          AC_RESETALL AC_GREEN AC_BOLD "called stop in %llums.\n" AC_RESETALL,
          timestamp() - ts
          );
  PASSm(test_case->MSG);
}


TEST t_fsmon_init(TEST_CASE_T *test_case){
  int res = -1;

  res = fsmon_init(&receive_event_handler, test_case->MONITORED_PATHS, (void *)test_case);
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
    free(TEST_CASES[i].MSG);
  }
}

GREATEST_MAIN_DEFS();


int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(s_test_cases);
}

