/*
** Jason Allen
**
** This program is a basic chat application over UDP.  The server receives all
** requests and routes them accordingly
**
** to run, type 'make' in a unix shell, ./server to start up the server first, Then
** ./client for each user client you would like to start up.  Clients will follow
** the onscreen instructions to proceed.
**
*/

#include "Programming2.h"

void *startServer();
void ProcessRequest(Message_t message, struct sockaddr_in clientAddress);
void *HandleUserLogin(void *arg);
void *HandleUserLogout(void *arg);
void *HandlePrivateMessageRequest(void *arg);
void *HandleBroadcastMessageRequest(void *arg);
void *HandleFileSendRequest(void *arg);
void *HandleFileReceiveRequest(void *arg);
void sendMessage(char *messageContents, char *data, MESSAGE_TYPE messageType, User_t sender, User_t receiver);
void addToUserList(User_t user);
char *getUserList();
void *HandleActiveUserRequest(void *arg);
User_t lookupUser(char *name);
void removeFromUserList(char* entry, UserList_t **list);
void addToFileTransferQueue(char *fileName, char *sender, char *receiver);
FileTransferQueue_t lookupFileTransfer(char *fileName, char *receiver);
void removeFromFileTransferQueue(char* entry, FileTransferQueue_t **queue);
void readFile(char *fileName, User_t receiver);
char *getTime(bool shouldPrint);

int sock; /* Socket */
UserList_t *userList;
processThread_t *processList;
FileTransferQueue_t *fileTransferQueue;
struct User server;

int main(int argc, char *argv[])
{
    strcpy(server.name, SYSTEM_USER_NAME);
    startServer();

    return 0;
}
/* ====================== startSever ==========================
/* Start the UDP server and wait for any incoming user requests
/*
/* @params none
/* @returns nothing
*/
void *startServer()
{
  struct sockaddr_in echoServAddr; /* Local address */
  char echoBuffer[ECHOMAX]; /* Buffer for echo string */
  unsigned short echoServPort; /* Server port */

  echoServPort = PORT;

  /* Create socket for incoming connections */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    DieWithError( "socket () failed") ;

  /* Construct local address structure */
  memset(&echoServAddr, 0, sizeof(echoServAddr)); /* Zero out structure */
  echoServAddr.sin_family = AF_INET; /* Internet address family */
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
  echoServAddr.sin_port = htons(echoServPort); /* Local port */

  /* Bind to the local address */
  if (bind(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
    DieWithError ( "bind () failed");

  printf("Server Started.\n\n");

  while(1) /* Run forever */
  {
      printf("Awaiting Request...\n");
      struct sockaddr_in echoClntAddr; /* Client address */
      unsigned int clientAddrLen = sizeof(echoClntAddr); /* Length of incoming message */
      int recvMsgSize; /* Size of received message */

      /* Block until receive message from a client */
      if ((recvMsgSize = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &echoClntAddr, &clientAddrLen)) < 0)
        DieWithError("recvfrom() failed") ;

      /* received client request */
      printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

      Message_t message;
      memcpy(&message, echoBuffer, sizeof(message));
      ProcessRequest(message, echoClntAddr);
  }
}
/* ====================== ProcessRequest ===============================
/* Process a request sent by a client.  Determines the type of requests
/* from the message.type and then creates a new thread to handle the request
/* so that the main server thread is free to continuing receiving requests
/* from other users
/*
/* @param message - the message to process
          clientAddress - the IP address of the client
/* @returns nothing
*/
void ProcessRequest(Message_t message, struct sockaddr_in clientAddress)
{
  processThread_t *serverProcess = addToProcessList(&processList);
  Message_t *messagePtr = malloc(sizeof(message));

  memcpy(messagePtr, &message, sizeof(message));

  if(message.type == LOGIN_REQUEST)
  {
      printf("Login Request\n");
      User_t user = message.sender;
      user.address = clientAddress;
      pthread_create(&serverProcess->thread, NULL, HandleUserLogin, &user);
  }
  else if(message.type == LOGOUT_REQUEST)
  {
      printf("Logout Request\n");
      User_t user = message.sender;
      pthread_create(&serverProcess->thread, NULL, HandleUserLogout, &user);
  }
  else if(message.type == USER_LIST_REQUEST)
  {
      printf("User List Request\n");
      User_t user = lookupUser(message.sender.name);
      pthread_create(&serverProcess->thread, NULL, HandleActiveUserRequest, &user);
  }
  else if(message.type == PRIVATE_MESSAGE_REQUEST || message.type == PRIVATE_MESSAGE)
  {
      printf("Private Message Request\n");
      pthread_create(&serverProcess->thread, NULL, HandlePrivateMessageRequest, messagePtr);
  }
  else if(message.type == BROADCAST_MESSAGE_REQUEST)
  {
      printf("Broadcast Message Request\n");
      pthread_create(&serverProcess->thread, NULL, HandleBroadcastMessageRequest, messagePtr);
  }
  else if(message.type == FILE_SEND_REQUEST)
  {
      printf("File Send Request\n");
      pthread_create(&serverProcess->thread, NULL, HandleFileSendRequest, messagePtr);
  }
  else if(message.type == FILE_RECV_REQUEST)
  {
      printf("File Receive Request\n");
      pthread_create(&serverProcess->thread, NULL, HandleFileReceiveRequest, messagePtr);
  }
}
/* ====================== HandleUserLogin ===============================
/* Handles a login request from the user
/*
/* @param user - the user to log in (casted from void * for threading)
/* @returns nothing
*/
void *HandleUserLogin(void *arg)
{
   User_t *user = (User_t*)arg;
   User_t lookup = lookupUser(user->name);

   // check to see if the username is already taken
   if(strcmp(lookup.name, user->name) == 0)
   {
     sendMessage("Desired username already taken", NULL, REQUEST_FAILURE, server, *user);
   }
   else
   {
     printf("%s logged in.\n", user->name);
     sendMessage("You are now logged in", NULL, LOGIN_REQUEST, server, *user);
     addToUserList(*user);
   }

   pthread_detach(pthread_self());
   pthread_exit(NULL);
}

/* ====================== HandleUserLogout ===============================
/* Handles a logout request from the user
/*
/* @param user - the user to log in (casted from void * for threading)
/* @returns nothing
*/
void *HandleUserLogout(void *arg)
{
   User_t *user = (User_t*)arg;

   printf("%s logged out.\n", user->name);
   removeFromUserList(user->name, &userList);

   pthread_detach(pthread_self());
   pthread_exit(NULL);
}
/* ====================== HandleActiveUserRequest ===============================
/* Handles a request for the list of active users and sends it to the client
/*
/* @param user - the requesting client (casted from void * for threading)
/* @returns nothing
*/
void *HandleActiveUserRequest(void *arg)
{
  User_t *user = (User_t*)arg;
  char message[MAX_MESSAGE_SIZE] = "Active Users: ";
  char *userList = getUserList();

  strcat(message, userList);
  sendMessage(message, NULL, USER_LIST_REQUEST, server, *user);

  pthread_detach(pthread_self());
  pthread_exit(NULL);
}
/* ====================== HandlePrivateMessageRequest ====================
/* Handles a request to send a private message from one user to another
/*
/* @param message - the message to send (casted from void * for threading)
/* @returns nothing
*/
void *HandlePrivateMessageRequest(void *arg)
{
  Message_t *message = (Message_t*)arg;

  // find the user and receiver from server database to ensure address info is correct
  User_t sender = lookupUser(message->sender.name);
  User_t receiver = lookupUser(message->receiver.name);

  if(strcmp(receiver.name, message->receiver.name) != 0)
  {
    sendMessage("User not online.", NULL, REQUEST_FAILURE, server, sender);
  }
  else
  {
    if(message->type == PRIVATE_MESSAGE_REQUEST) // send confirmation to sender
        sendMessage("Message Sent.", NULL, REQUEST_SUCCESS, server, sender);

    sendMessage(message->contents, NULL, PRIVATE_MESSAGE, sender, receiver); // send to receiver
  }

  pthread_detach(pthread_self());
  pthread_exit(NULL);
}
/* ====================== HandleBroadcastMessageRequest ===================
/* Handles a request to send a message to all users currently online
/*
/* @param message - the message to send (casted from void * for threading)
/* @returns nothing
*/
void *HandleBroadcastMessageRequest(void *arg)
{
  Message_t *message = (Message_t*)arg;
  User_t sender = lookupUser(message->sender.name);

  UserList_t *current = userList;

  while (current != NULL) {

    sendMessage(message->contents, NULL, PRIVATE_MESSAGE, sender, current->user);

    current = current->next;
  }

  pthread_detach(pthread_self());
  pthread_exit(NULL);
}
/* ====================== HandleFileSendRequest ===================
/* Handles a request to send a file from one user to another.  Since the
/* requirements are that the receiver must respond to the request before
/* downloading, request is sent to a queue and the receiver is informed of
/* the pending download.  Once the receiver responds, the information about
/* the transfer is retrieved from the queue, and the transfer is completed
/* in HandleFileReceiveRequest
/*
/* @param message - the message to process (casted from void * for threading)
/* @returns nothing
*/
void *HandleFileSendRequest(void *arg)
{
    Message_t *message = (Message_t*)arg;

    User_t sender = lookupUser(message->sender.name);
    User_t receiver = lookupUser(message->receiver.name);

    if(strcmp(receiver.name, message->receiver.name) != 0)
    {
        sendMessage("User not online.", NULL, REQUEST_FAILURE, server, sender);
    }
    else
    {   // check if the file exists and has read acces
        if( access(message->contents, R_OK ) != -1)
        {
            // add the message to file transfer queue and send confirmation to sender
            addToFileTransferQueue(message->contents, message->sender.name, message->receiver.name);
            sendMessage("Sending File.", NULL, REQUEST_SUCCESS, server, sender);

            // send a message to the receiver that a file is waiting to be downloaded
            char fileWaitingMessage[MAX_MESSAGE_SIZE + 50];
            sprintf(fileWaitingMessage, "%s would like to send you the file \"%s\"", message->sender.name, message->contents);
            sendMessage(fileWaitingMessage, message->contents, FILE_SEND_REQUEST, server, receiver);
        }
        else
        {
            char errorMessage[MAX_MESSAGE_SIZE + 50];
            sprintf(errorMessage, "Error, invalid file name \"%s\"", message->contents);
            sendMessage(errorMessage, NULL, REQUEST_FAILURE, server, sender);
        }
    }
}
/* ====================== HandleFileSendRequest ========================
/* Handles a request to receive a pending file download from the server.
/* Once the receiver has responded to the file send request, information
/* about the transfer is gathered from the transfer queue.  The transfer
/* is then either accepted or denied.
/*
/* @param message - the message to process (casted from void * for threading)
/* @returns nothing
*/
void *HandleFileReceiveRequest(void *arg)
{
    Message_t *message = (Message_t*)arg;

    FileTransferQueue_t transferData = lookupFileTransfer(message->data, message->sender.name);
    User_t sender = lookupUser(transferData.sender);
    User_t receiver = lookupUser(transferData.receiver);

    // receiver has denied the transfer
    if(strcmp(message->contents, VALIDATION_NO) == 0)
    {
      char sendResultMessage[MAX_MESSAGE_SIZE + 50];
      sprintf(sendResultMessage, "%s refused to download the file \"%s\"", receiver.name, transferData.fileName);
      sendMessage(sendResultMessage, NULL, PRIVATE_MESSAGE, server, sender); // inform the sender

      removeFromFileTransferQueue(transferData.fileName, &fileTransferQueue);
    }
    else
    {
      readFile(transferData.fileName, receiver);
    }
}
/* ====================== sendMessage ============================
/* Sends a formatted message from the server to a client.  Message
/* can originate from the server or from another client
/*
/* @params messageContents - the contents of the messages
/*         data - extra data not tied to the message contents
/*         messageType - the type of messages
/*         sender - the sender of the message
/*         receiver - the receiver of the message
/* @returns nothing
*/
void sendMessage(char *messageContents, char *data, MESSAGE_TYPE messageType, User_t sender, User_t receiver)
{
  Message_t message;
  message.sender = sender;
  message.receiver = receiver;
  message.type = messageType;
  strcpy(message.contents, messageContents);

  if(data != NULL){ strcpy(message.data, data); }

  if (sendto(sock, &message, sizeof(message), 0, (struct sockaddr *) &receiver.address, sizeof(receiver.address)) != sizeof(message))
    DieWithError("sendto() sent a different number of bytes than expected");
}
/* ====================== addToUserList ============================
/* Add a user to the server maintained active user list
/*
/* @params user - the user to add
/* @returns nothing
*/
void addToUserList(User_t user){

  UserList_t *tmp = (UserList_t*)malloc(sizeof(UserList_t));

  tmp->user = user;

  if(userList == NULL){
      userList = tmp;
  }
  else{
    UserList_t *current = userList;

    while (current->next != NULL) {  current = current->next;  }

    current->next = tmp;
    tmp->previous = current;
  }
}
/* ====================== getUserList ============================
/* Get the active user list
/*
/* @params none
/* @returns list of users in a comma delimited string
*/
char *getUserList()
{
  char users[MAX_MESSAGE_SIZE] = "";
  char *tmp = users;

  UserList_t *current = userList;
  int length = 0;

  while (current != NULL) {

    length += sprintf(tmp + length, "%s, ", current->user.name);

    current = current->next;
  }
  return tmp;
}
/* ====================== lookupUser ============================
/* Lookup a user in the active user list
/*
/* @params name - the name of the user to search for
/* @returns User_t
*/
User_t lookupUser(char *name)
{
  UserList_t *current = userList;

  while (current != NULL)
  {
    if(strcmp(current->user.name, name) == 0)
      return current->user;

    current = current->next;
  }
}
/* ====================== removeFromUserList ============================
/* Remove a user from the user list
/*
/* @params entry - the name of the entry to search for
/*         queue - the user list (address must be passed)
/* @returns none
*/
void removeFromUserList(char* entry, UserList_t **list){

  UserList_t *current = *list;
  UserList_t *tmp;

  while (current != NULL) {

      if(strcmp(current->user.name, entry) == 0){

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
/* ====================== addToFileTransferQueue ============================
/* Add information about a file transfer session to a queue for
/* later proccessing
/*
/* @params fileName - the name of the file
/*         sender - the sender of the file
/*         receiver - the receiver of the file
/* @returns nothing
*/
void addToFileTransferQueue(char *fileName, char *sender, char *receiver)
{
  FileTransferQueue_t *tmp = (FileTransferQueue_t*)malloc(sizeof(FileTransferQueue_t));

  strcpy(tmp->fileName, fileName);
  strcpy(tmp->sender, sender);
  strcpy(tmp->receiver, receiver);

  if(fileTransferQueue == NULL){
      fileTransferQueue = tmp;
  }
  else{
    FileTransferQueue_t *current = fileTransferQueue;

    while (current->next != NULL) {  current = current->next;  }

    current->next = tmp;
    tmp->previous = current;
  }
}
/* ====================== lookupFileTransfer ============================
/* Lookup a file transfer session currently in the queue
/*
/* @params fileName - the name of the file
/*         receiver - the receiver of the filer
/* @returns FileTransferQueue_t
*/
FileTransferQueue_t lookupFileTransfer(char *fileName, char *receiver)
{
  FileTransferQueue_t *current = fileTransferQueue;

  while (current != NULL)
  {
    if(strcmp(current->fileName, fileName) == 0 && strcmp(current->receiver, receiver) == 0)
      return *current;

    current = current->next;
  }
}
/* ====================== removeFromFileTransferQueue ============================
/* Remove a file transfer session from the queue
/*
/* @params entry - the name of the entry to search for
/*         queue - the file transfer queue (address must be passed)
/* @returns none
*/
void removeFromFileTransferQueue(char* entry, FileTransferQueue_t **queue){

  FileTransferQueue_t *current = *queue;
  FileTransferQueue_t *tmp;

  while (current != NULL) {

      if(strcmp(current->fileName, entry) == 0){

        if(*queue == NULL || current == NULL)
          return;

        if(*queue == current)
          *queue = current->next;

        if(current->next != NULL)
          current->next->previous = current->previous;

        if(current->previous != NULL)
          current->previous->next = current->next;

        free(current);
      }
        current = current->next;
  }
}

/* ====================== readFile =========================
/* Reads a file and sends it's contents back to the receiver
/* line by line
/*
/* @param fileName - the name of the file
/*        receiver - the receiver client
/* @returns nothing
*/
void readFile(char *fileName, User_t receiver)
{
  FILE *input = fopen(fileName, "rb");
  size_t line_buf_size = 0;
  ssize_t line_size = 0;
  int recvMsgSize = 0;
  int totalBytesSent = 0;

  if(input == NULL)
  {
      // if the file was not found, send error message to client
      char errorMessage[MAX_MESSAGE_SIZE];
      sprintf(errorMessage, "File \"%s\" could not be downloaded", fileName);
      sendMessage(errorMessage, NULL, REQUEST_FAILURE, server, receiver);
  }
  else
  {
      printf("Sending file \"%s\" to %s\n", fileName, receiver.name);

      // read file line by line
      while (line_size >= 0)
      {
        char *line_buf = NULL;
        line_size = getline(&line_buf, &line_buf_size, input);

        if(line_size < 0)
          continue;

        totalBytesSent += line_size;

        printf("read line: %s\n", line_buf);

        sendMessage(line_buf, NULL, FILE_TRANSFER, server, receiver);
      }

      fclose(input);

      printf("\nTotal Bytes Sent: %d\n\n", totalBytesSent);

      // send EOF to the data field of the message to signal tranfer complete
      sendMessage("", "EOF", FILE_TRANSFER, server, receiver);
  }
}
/* ====================== getTime ==============================
** Get the current local data and time
**
** @params shouldPrint - whether or not to also print to stdout
** @returns time string in Www Mmm dd hh:mm:ss yyyy format
*/
char *getTime(bool shouldPrint)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    if(shouldPrint)
      printf ( "Current local time and date: %s", asctime (timeinfo) );

    return asctime(timeinfo);
}
