#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct processThread{

  int tid;
  pthread_t thread;
  struct processThread *next;
  struct processThread *previous;

} processThread_t;

processThread_t *addToProcessList(processThread_t **pList);
void freeProcessList(processThread_t **pList);
