#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>
#include <stdint.h>
#include "multimodulo.h"
#define MAX_SERVERS 255

struct Server {
  char ip[255];
  int port;
};

struct Task {
    struct Server server;

    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    bool ok;
};



void *Worker(void *arg) {
    struct Task *task = arg;
    task->ok = false;

    struct hostent *hostname = gethostbyname(task->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n",
                task->server.ip);
        return NULL;
    }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(task->server.port);
    memcpy(&server.sin_addr, hostname->h_addr,
           sizeof(server.sin_addr));

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        return NULL;
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed\n");
        close(sck);
        return NULL;
    }

    uint64_t values[3] = {
        task->begin,
        task->end,
        task->mod
    };

    if (send(sck, values, sizeof(values), 0) < 0) {
        fprintf(stderr, "Send failed\n");
        close(sck);
        return NULL;
    }

    uint64_t response = 0;

    if (recv(sck, &response, sizeof(response), 0) <= 0) {
        fprintf(stderr, "Receive failed\n");
        close(sck);
        return NULL;
    }

    task->result = response;
    task->ok = true;

    close(sck);
    return NULL;
}


bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // TODO: explain why 255

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
        break;
      case 2:
        // TODO: your code here
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }
  if (k == 0 || mod == 0) {
    fprintf(stderr, "Invalid k or mod\n");
    return 1;
}

  // Открываем файл со списком серверов
  FILE *file = fopen(servers, "r");
  if (file == NULL) {
      perror("Cannot open servers file");
      return 1;
  }

  // Выделяем память под список серверов
  struct Server *to = malloc(sizeof(struct Server) * MAX_SERVERS);
  if (to == NULL) {
      perror("malloc failed");
      fclose(file);
      return 1;
  }

  unsigned int servers_num = 0;
  char line[512];

  // Читаем файл построчно
  while (fgets(line, sizeof(line), file) != NULL) {
      // Убираем перевод строки в конце
      line[strcspn(line, "\r\n")] = '\0';

      // Пропускаем пустые строки
      if (line[0] == '\0') {
          continue;
      }

      // Проверяем, что серверов не слишком много
      if (servers_num >= MAX_SERVERS) {
          fprintf(stderr, "Too many servers\n");
          free(to);
          fclose(file);
          return 1;
      }

      // Разбираем строку формата IP:PORT
      if (sscanf(line, "%254[^:]:%d",
                to[servers_num].ip,
                &to[servers_num].port) != 2) {
          fprintf(stderr, "Invalid server address: %s\n", line);
          free(to);
          fclose(file);
          return 1;
      }

      servers_num++;
  }

  fclose(file);

  if (servers_num == 0) {
      fprintf(stderr, "No servers found in file\n");
      free(to);
      return 1;
  }
  if (servers_num > k) {
    fprintf(stderr, "Number of servers cannot exceed k\n");
    free(to);
    return 1;
}
  

  // TODO: work continiously, rewrite to make parallel
  // Создаём массив задач и массив потоков
  struct Task *tasks = malloc(sizeof(struct Task) * servers_num);
  pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);

  if (tasks == NULL || threads == NULL) {
      perror("malloc failed");
      free(tasks);
      free(threads);
      free(to);
      return 1;
  }

  // Распределяем числа от 1 до k между серверами
  uint64_t chunk = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t begin = 1;

  for (unsigned int i = 0; i < servers_num; i++) {
      tasks[i].server = to[i];
      tasks[i].begin = begin;
      tasks[i].end = begin + chunk - 1;

      // Первым remainder серверам добавляем по одному числу
      if (remainder > 0) {
          tasks[i].end++;
          remainder--;
      }

      tasks[i].mod = mod;
      tasks[i].result = 1;
      tasks[i].ok = false;

      begin = tasks[i].end + 1;
  }

  // Запускаем отдельный поток для каждого сервера
  for (unsigned int i = 0; i < servers_num; i++) {
      if (pthread_create(&threads[i], NULL, Worker, &tasks[i]) != 0) {
          fprintf(stderr, "pthread_create failed\n");
          return 1;
      }
  }

  // Ждём завершения всех потоков
  for (unsigned int i = 0; i < servers_num; i++) {
      pthread_join(threads[i], NULL);
  }

  // Объединяем результаты серверов
  uint64_t answer = 1;

  for (unsigned int i = 0; i < servers_num; i++) {
      if (!tasks[i].ok) {
          fprintf(stderr, "Failed to get result from server %u\n", i);
          free(tasks);
          free(threads);
          free(to);
          return 1;
      }

      answer = MultModulo(answer, tasks[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);

  free(tasks);
  free(threads);
  free(to);

}