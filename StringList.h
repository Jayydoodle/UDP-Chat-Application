#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_SIZE 255

typedef struct StringList{

  char string[MAX_STRING_SIZE];
  struct StringList *next;
  struct StringList *previous;
} StringList_t;

void addToStringList(char *string, StringList_t **stringList);
StringList_t lookupStringFromList(char *string, StringList_t **stringList);
char *getStringList(StringList_t **stringList);
void removeFromStringList(char* entry, StringList_t **list);
