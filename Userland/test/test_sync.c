// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stdio.h>
#include "./include/syscall.h"
#include "./include/test_util.h"

#define SEM_ID "sem"
#define TOTAL_PROCESSES 6
#define RACE_SPIN_WITH_SEM 200
#define RACE_SPIN_WITHOUT_SEM 50000
#define RANDOM_SPIN_EXTRA 40000
#define UNSYNC_EXTRA_SLEEP_MS 40

static volatile int64_t global_shared_value = 0;
static const int8_t process_increments[TOTAL_PROCESSES] = { -2, -1, -1, 1, 1, 2 };

static void int_to_string(int value, char *buffer);

static void print_final_value(int64_t value) {
  char digits[32];
  int index = 0;
  int negative = 0;
  uint64_t magnitude;

  if (value < 0) {
    negative = 1;
    magnitude = (uint64_t)(-(value + 1)) + 1;
  } else {
    magnitude = (uint64_t)value;
  }

  do {
    digits[index++] = (char)('0' + (magnitude % 10));
    magnitude /= 10;
  } while (magnitude > 0 && index < (int)sizeof(digits));

  printf("Final value: ");
  if (negative) {
    printf("-");
  }

  while (index > 0) {
    printf("%c", digits[--index]);
  }
  printf("\n");
}

static void slowInc(volatile int64_t *p, int64_t inc, int8_t use_sem) {
  int64_t aux = *p;

  if (!use_sem) {
    uint32_t random_spin = RACE_SPIN_WITHOUT_SEM + GetUniform(RANDOM_SPIN_EXTRA);
    bussy_wait(random_spin);
    my_yield();
    random_spin = RACE_SPIN_WITHOUT_SEM + GetUniform(RANDOM_SPIN_EXTRA);
    bussy_wait(random_spin);
  } else {
    bussy_wait(RACE_SPIN_WITH_SEM);
  }

  aux += inc;

  if (!use_sem) {
    uint32_t random_spin = RACE_SPIN_WITHOUT_SEM + GetUniform(RANDOM_SPIN_EXTRA);
    bussy_wait(random_spin);
    my_yield();
    sleep(UNSYNC_EXTRA_SLEEP_MS);
  }

  *p = aux;
}

uint64_t my_process_inc(uint64_t argc, char *argv[]) {
  uint64_t n;
  int64_t inc;
  int64_t use_sem;
  int32_t semId = -1;

  if (argc != 3)
    return (uint64_t)-1;

  if ((n = satoi(argv[0])) <= 0)
    return (uint64_t)-1;
  if ((inc = satoi(argv[1])) == 0)
    return (uint64_t)-1;
  if ((use_sem = satoi(argv[2])) < 0)
    return (uint64_t)-1;

  if (use_sem) {
    semId = my_sem_open(SEM_ID, 1);
    if (semId < 0) {
      printf("test_sync: ERROR opening semaphore\n");
      return (uint64_t)-1;
    }
  }

  for (uint64_t i = 0; i < n; i++) {
    if (use_sem) {
      if (my_sem_wait(semId) < 0) {
        printf("test_sync: ERROR waiting semaphore\n");
        my_sem_close(semId);
        return (uint64_t)-1;
      }
    }
    slowInc(&global_shared_value, inc, use_sem);
    if (use_sem) {
      if (my_sem_post(semId) < 0) {
        printf("test_sync: ERROR posting semaphore\n");
        my_sem_close(semId);
        return (uint64_t)-1;
      }
    }
  }

  if (use_sem) {
    my_sem_close(semId);
  }

  return 0;
}

static void int_to_string(int value, char *buffer) {
  char temp[8];
  int idx = 0;
  int is_negative = 0;

  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return;
  }

  if (value < 0) {
    is_negative = 1;
    value = -value;
  }

  while (value > 0 && idx < (int)sizeof(temp)) {
    temp[idx++] = (char)('0' + (value % 10));
    value /= 10;
  }

  int pos = 0;
  if (is_negative) {
    buffer[pos++] = '-';
  }

  while (idx > 0) {
    buffer[pos++] = temp[--idx];
  }
  buffer[pos] = '\0';
}

void my_process_inc_wrapper(uint64_t argc, char **argv) {
  int64_t status = (int64_t)my_process_inc(argc, (char **)argv);
  exitProcess((int32_t)status);
}

uint64_t test_sync(uint64_t argc, char *argv[]) {
  if (argc != 2)
    return (uint64_t)-1;

  int64_t iterations = satoi(argv[0]);
  int64_t use_sem = satoi(argv[1]);

  if (iterations <= 0 || use_sem < 0)
    return (uint64_t)-1;

  uint64_t pids[TOTAL_PROCESSES];
  char incBuffers[TOTAL_PROCESSES][5];
  char *processArgv[TOTAL_PROCESSES][4];

  global_shared_value = 0;
  int32_t parentSemId = -1;

  if (use_sem) {
    parentSemId = my_sem_open(SEM_ID, 1);
    if (parentSemId < 0) {
      printf("test_sync: ERROR pre-creating semaphore\n");
      return (uint64_t)-1;
    }
  }

  for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
    int_to_string(process_increments[i], incBuffers[i]);
    processArgv[i][0] = argv[0];
    processArgv[i][1] = incBuffers[i];
    processArgv[i][2] = argv[1];
    processArgv[i][3] = NULL;

    pids[i] = my_create_process("my_process_inc", 3, processArgv[i]);
    if ((int64_t)pids[i] < 0) {
      printf("test_sync: ERROR creating worker process\n");
      if (use_sem && parentSemId >= 0) {
        my_sem_close(parentSemId);
      }
      return (uint64_t)-1;
    }
  }

  for (uint64_t i = 0; i < TOTAL_PROCESSES; i++) {
    my_wait(pids[i]);
  }

  if (use_sem) {
    my_sem_close(parentSemId);
  }

  print_final_value(global_shared_value);

  return 0;
}
