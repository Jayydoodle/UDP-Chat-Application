#include "StringList.h"

void addToStringList(char *string, StringList_t **stringList)
{
  StringList_t *tmp = (StringList_t*)malloc(sizeof(StringList_t));

  strcpy(tmp->string, string);

  if(*stringList == NULL){
      *stringList = tmp;
  }
  else{
    StringList_t *current = *stringList;

    while (current->next != NULL) {  current = current->next;  }

    current->next = tmp;
    tmp->previous = current;
  }
}

StringList_t lookupStringFromList(char *string, StringList_t **stringList)
{
  StringList_t *current = *stringList;

  while (current != NULL)
  {
    if(strcmp(current->string, string) == 0)
      return *current;

    current = current->next;
  }
}

char *getStringList(StringList_t **stringList)
{
  char users[1024] = "";
  char *tmp = users;

  StringList_t *current = *stringList;
  int length = 0;

  while (current != NULL) {

    length += sprintf(tmp + length, "%s, ", current->string);

    current = current->next;
  }
  return tmp;
}

void removeFromStringList(char* entry, StringList_t **list){

  StringList_t *current = *list;
  StringList_t *tmp;

  while (current != NULL) {

      if(strcmp(current->string, entry) == 0){

        if(*list == NULL || current == NULL)
          return;

        if(*list == current)
          *list = current->next;

        if(current->next != NULL)
          current->next->previous = current->previous;

        if(current->previous != NULL)
          current->previous->next = current->next;

        free(current);
      }
        current = current->next;
  }
}
