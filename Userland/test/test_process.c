// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stdio.h>
#include <sys.h>
#include "test_util.h"

#define PROCESS_SNAPSHOT_CAP 64

static void endless_loop_entry(uint64_t argc, char **argv) {
  (void)argc;
  (void)argv;
  endless_loop();
}

typedef enum {
  TEST_STATE_RUNNING,
  TEST_STATE_BLOCKED,
  TEST_STATE_KILLED
} test_state_t;

typedef struct P_rq {
  int32_t pid;
  test_state_t state;
} p_rq;

static int process_is_active(int32_t pid) {
  if (pid <= 0) {
    return 0;
  }

  ProcessInfo snapshot[PROCESS_SNAPSHOT_CAP];
  int32_t count = getProcesses(snapshot, PROCESS_SNAPSHOT_CAP);

  if (count <= 0) {
    return 0;
  }

  for (int32_t i = 0; i < count; i++) {
    if (snapshot[i].pid == pid && snapshot[i].state != TERMINATED) {
      return 1;
    }
  }

  return 0;
}

static void mark_as_killed(p_rq *entry, uint8_t *aliveCounter) {
  if (entry->state != TEST_STATE_KILLED) {
    entry->state = TEST_STATE_KILLED;
    if (*aliveCounter > 0) {
      (*aliveCounter)--;
    }
  }
}

int64_t test_processes(uint64_t argc, char *argv[]) {
  uint8_t rq;
  uint8_t alive = 0;
  uint8_t action;
  uint64_t max_processes;

  if (argc != 1)
    return -1;

  if ((max_processes = satoi(argv[0])) <= 0)
    return -1;

  p_rq p_rqs[max_processes];

  while (1) {

    for (rq = 0; rq < max_processes; rq++) {
      p_rqs[rq].pid = createProcess("endless_loop", endless_loop_entry, 0, 0, 0, 0, 0, 0);

      if (p_rqs[rq].pid <= 0) {
        printf("test_processes: ERROR creating process\n");
        return -1;
      } else {
        p_rqs[rq].state = TEST_STATE_RUNNING;
        alive++;
      }
    }

    while (alive > 0) {

      for (rq = 0; rq < max_processes; rq++) {
        action = GetUniform(100) % 2;

        switch (action) {
          case 0:
            if (p_rqs[rq].state == TEST_STATE_RUNNING || p_rqs[rq].state == TEST_STATE_BLOCKED) {
              if (killProcess(p_rqs[rq].pid) < 0) {
                if (!process_is_active(p_rqs[rq].pid)) {
                  mark_as_killed(&p_rqs[rq], &alive);
                  continue;
                }
                printf("test_processes: ERROR killing process\n");
                return -1;
              }
              mark_as_killed(&p_rqs[rq], &alive);
            }
            break;

          case 1:
            if (p_rqs[rq].state == TEST_STATE_RUNNING) {
              if (toggleBlockProcess(p_rqs[rq].pid) < 0) {
                if (!process_is_active(p_rqs[rq].pid)) {
                  mark_as_killed(&p_rqs[rq], &alive);
                  continue;
                }
                printf("test_processes: ERROR blocking process\n");
                return -1;
              }
              p_rqs[rq].state = TEST_STATE_BLOCKED;
            }
            break;
        }
      }

      for (rq = 0; rq < max_processes; rq++)
        if (p_rqs[rq].state == TEST_STATE_BLOCKED && GetUniform(100) % 2) {
          if (unblockProcess(p_rqs[rq].pid) < 0) {
            if (!process_is_active(p_rqs[rq].pid)) {
              mark_as_killed(&p_rqs[rq], &alive);
              continue;
            }
            printf("test_processes: ERROR unblocking process\n");
            return -1;
          }
          p_rqs[rq].state = TEST_STATE_RUNNING;
        }
    }
  }
}
