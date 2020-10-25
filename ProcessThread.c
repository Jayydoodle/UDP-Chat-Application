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

processThread_t *addToProcessList(processThread_t **pList){

  processThread_t *tmp = (processThread_t*)malloc(sizeof(processThread_t));

  if(*pList == NULL){
      *pList = tmp;
      (*pList)->tid = 0;
  }
  else{
    processThread_t *current = *pList;
    
    while (current->next != NULL) {

        current = current->next;
    }
    current->next = tmp;
    tmp->previous = current;
    tmp->tid = (tmp->previous->tid) + 1;
  }
  return tmp;
}
void freeProcessList(processThread_t **pList){
  void *thread;
  processThread_t *current;
  long total = 0;
  while(*pList != NULL){

      current = *pList;
      *pList = (*pList)->next;

      printf("waiting for thread '%d' to exit.\n", current->tid);;
      pthread_join(current->thread, &thread);
      free(current);
  }
}
