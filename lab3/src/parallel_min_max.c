#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"


/*
 * Флаг таймаута.
 *
 * 0 - таймаут ещё не наступил
 * 1 - таймаут наступил
 */
volatile sig_atomic_t timeout_flag = 0;


/*
 * Обработчик сигнала SIGALRM.
 *
 * Когда alarm() закончится, ОС отправит SIGALRM,
 * после чего будет вызвана эта функция.
 */
void timeout_handler(int signal) {
  timeout_flag = 1;
}


int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;

  // -1 означает, что --timeout не был указан
  int timeout = -1;

  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int option_index = 0;

    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:

        switch (option_index) {

          // --seed
          case 0:
            seed = atoi(optarg);
            break;

          // --array_size
          case 1:
            array_size = atoi(optarg);
            break;

          // --pnum
          case 2:
            pnum = atoi(optarg);
            break;

          // --by_files
          case 3:
            with_files = true;
            break;

          // --timeout
          case 4:
            timeout = atoi(optarg);
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }

        break;

      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }


  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }


  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf(
        "Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
        "[--by_files] [--timeout \"num\"]\n",
        argv[0]
    );

    return 1;
  }


  int *array = malloc(sizeof(int) * array_size);

  GenerateArray(array, array_size, seed);


  int active_child_processes = 0;

  int pipes[pnum][2];


  /*
   * Массив PID всех дочерних процессов.
   *
   * child_pids[i] = PID ребёнка i
   */
  pid_t child_pids[pnum];


  /*
   * Состояние детей.
   *
   * false = ребёнок ещё работает
   * true  = ребёнок уже завершился
   */
  bool child_finished[pnum];


  for (int i = 0; i < pnum; i++) {

    child_finished[i] = false;

    if (pipe(pipes[i]) == -1) {
      printf("Pipe failed!\n");
      free(array);
      return 1;
    }
  }


  struct timeval start_time;

  gettimeofday(&start_time, NULL);


  /*
   * Если timeout задан, устанавливаем обработчик SIGALRM.
   */
  if (timeout != -1) {
    signal(SIGALRM, timeout_handler);
  }


  /*
   * Создаём дочерние процессы.
   */
  for (int i = 0; i < pnum; i++) {

    pid_t child_pid = fork();

    if (child_pid >= 0) {

      // fork() успешно выполнился

      if (child_pid == 0) {

        /*
         * ЭТО РЕБЁНОК.
         */

        int begin = i * array_size / pnum;
        int end = (i + 1) * array_size / pnum;


        struct MinMax min_max = GetMinMax(array, begin, end);


        if (with_files) {

          char filename[50];

          sprintf(filename, "result_%d.txt", i);

          FILE *file = fopen(filename, "w");

          fprintf(file, "%d %d\n", min_max.min, min_max.max);

          fclose(file);

        } else {

          close(pipes[i][0]);

          write(
              pipes[i][1],
              &min_max.min,
              sizeof(int)
          );

          write(
              pipes[i][1],
              &min_max.max,
              sizeof(int)
          );

          close(pipes[i][1]);
        }

        return 0;
      }


      /*
       * ЭТО РОДИТЕЛЬ.
       *
       * Сохраняем PID ребёнка.
       */
      child_pids[i] = child_pid;

      child_finished[i] = false;

      active_child_processes++;

    } else {

      printf("Fork failed!\n");
      free(array);
      return 1;
    }
  }


  /*
   * Если timeout задан, запускаем таймер.
   *
   * Например:
   *
   * --timeout 10
   *
   * означает:
   *
   * через 10 секунд текущему процессу
   * придёт SIGALRM.
   */
  if (timeout != -1) {
    alarm(timeout);
  }


  /*
   * Если timeout НЕ задан,
   * оставляем старую логику программы.
   */
  if (timeout == -1) {

    while (active_child_processes > 0) {

      wait(NULL);

      active_child_processes--;
    }

  }


  /*
   * Если timeout задан,
   * используем неблокирующий waitpid().
   */
  else {

    while (active_child_processes > 0) {

      /*
       * Проверяем всех детей.
       */
      for (int i = 0; i < pnum; i++) {

        if (!child_finished[i]) {

          int status;

          pid_t result = waitpid(
              child_pids[i],
              &status,
              WNOHANG
          );


          /*
           * result > 0:
           * ребёнок завершился.
           */
          if (result > 0) {

            child_finished[i] = true;

            active_child_processes--;
          }
        }
      }


      /*
       * Если пришёл SIGALRM,
       * значит таймаут закончился.
       */
      if (timeout_flag) {

        printf("Timeout!\n");


        /*
         * Убиваем всех детей,
         * которые ещё работают.
         */
        for (int i = 0; i < pnum; i++) {

          if (!child_finished[i]) {

            kill(child_pids[i], SIGKILL);
          }
        }


        /*
         * После kill() нужно дождаться
         * завершения этих процессов.
         */
        for (int i = 0; i < pnum; i++) {

          if (!child_finished[i]) {

            int status;

            waitpid(
                child_pids[i],
                &status,
                0
            );

            child_finished[i] = true;

            active_child_processes--;
          }
        }

        break;
      }

      /*
       * Небольшая пауза, чтобы не крутить
       * цикл слишком быстро.
       */
      usleep(1000);
    }
  }


  /*
   * Собираем результаты.
   */
  struct MinMax min_max;

  min_max.min = INT_MAX;
  min_max.max = INT_MIN;


  for (int i = 0; i < pnum; i++) {

    int min = INT_MAX;
    int max = INT_MIN;


    /*
     * Если ребёнок был убит по timeout,
     * у него может не быть результата.
     *
     * Поэтому пропускаем незавершившихся
     * до таймаута детей.
     */
    if (timeout_flag && !child_finished[i]) {
      continue;
    }


    if (with_files) {

      char filename[50];

      sprintf(filename, "result_%d.txt", i);

      FILE *file = fopen(filename, "r");


      if (file != NULL) {

        fscanf(
            file,
            "%d %d",
            &min,
            &max
        );

        fclose(file);
      }

    } else {

      close(pipes[i][1]);


      /*
       * Читаем результат ребёнка.
       */
      read(
          pipes[i][0],
          &min,
          sizeof(int)
      );

      read(
          pipes[i][0],
          &max,
          sizeof(int)
      );


      close(pipes[i][0]);
    }


    if (min < min_max.min)
      min_max.min = min;

    if (max > min_max.max)
      min_max.max = max;
  }


  struct timeval finish_time;

  gettimeofday(&finish_time, NULL);


  double elapsed_time =
      (finish_time.tv_sec - start_time.tv_sec) * 1000.0;

  elapsed_time +=
      (finish_time.tv_usec - start_time.tv_usec) / 1000.0;


  free(array);


  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);

  fflush(NULL);

  return 0;
}