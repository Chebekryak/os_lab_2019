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

#include "lib.h"

struct Server {
  char ip[255];
  int port;
};

struct ServerThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};


void* ServerThread(void* args) {
  struct ServerThreadArgs* thread_args = (struct ServerThreadArgs*)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(thread_args->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

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

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    close(sck);
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    close(sck);
    return NULL;
  }

  memcpy(&thread_args->result, response, sizeof(uint64_t));
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file_path[255] = {'\0'};

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
        memcpy(servers_file_path, optarg, strlen(optarg));
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

  if (k == -1 || mod == -1 || !strlen(servers_file_path)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE* servers_file = fopen(servers_file_path, "r");
  if (!servers_file) {
    fprintf(stderr, "Cannot open servers file\n");
    return 1;
  }

  unsigned int servers_num = 0;
  char line[255];
  while (fgets(line, sizeof(line), servers_file)) {
    servers_num++;
  }
  rewind(servers_file);

  struct Server* to = malloc(sizeof(struct Server) * servers_num);
  for (int i = 0; i < servers_num; i++) {
    fscanf(servers_file, "%[^:]:%d\n", to[i].ip, &to[i].port);
  }
  fclose(servers_file);

  pthread_t threads[servers_num];
  struct ServerThreadArgs thread_args[servers_num];

  uint64_t chunk_size = k / servers_num;
  for (int i = 0; i < servers_num; i++) {
    thread_args[i].server = to[i];
    thread_args[i].begin = 1 + i * chunk_size;
    thread_args[i].end = (i == servers_num - 1) ? k : (i + 1) * chunk_size;
    thread_args[i].mod = mod;
    
    pthread_create(&threads[i], NULL, ServerThread, (void*)&thread_args[i]);
  }

  uint64_t total = 1;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    total = MultModulo(total, thread_args[i].result, mod);
  }

  printf("Final answer: %llu\n", total);
  free(to);

  return 0;
}