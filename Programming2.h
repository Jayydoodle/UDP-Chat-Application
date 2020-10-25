#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "ProcessThread.h"

#define PORT 4000
#define MAXPENDING 5
#define RCVBUFSIZE 1024
#define ECHOMAX 1024
#define MAX_MESSAGE_SIZE 255
#define MAX_USER_NAME 10
#define MAX_USER_IP 20
#define MAX_DATETIME 27
#define MAX_DATA 50

const char *SYSTEM_USER_NAME = "SYSTEM";
const char *VALIDATION_YES = "yes";
const char *VALIDATION_NO = "no";

typedef struct User{

	char name[MAX_USER_NAME];
	struct sockaddr_in address;
} User_t;

typedef struct UserList{

	struct User user;
	struct UserList *next;
	struct UserList *previous;

} UserList_t;

typedef enum {LOGIN_REQUEST, LOGOUT_REQUEST, PRIVATE_MESSAGE_REQUEST, USER_LIST_REQUEST,
              BROADCAST_MESSAGE_REQUEST, FILE_SEND_REQUEST, FILE_RECV_REQUEST,
              FILE_TRANSFER, PRIVATE_MESSAGE, REQUEST_SUCCESS, REQUEST_FAILURE} MESSAGE_TYPE;

typedef struct Message{

  MESSAGE_TYPE type;
	char contents[MAX_MESSAGE_SIZE];
  char data[MAX_MESSAGE_SIZE];
	struct User sender;
	struct User receiver;

} Message_t;

typedef struct FileTransferQueue{

  char fileName[MAX_MESSAGE_SIZE];
  char sender[MAX_USER_NAME];
  char receiver[MAX_USER_NAME];
  struct FileTransferQueue *next;
  struct FileTransferQueue *previous;

} FileTransferQueue_t;

void DieWithError(char *errorMessage);
