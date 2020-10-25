/*
** Jason Allen
**
** This program is a basic chat application over UDP.  The client is the user
** interface for the application and sends requests to the server for processing
**
** to run, type 'make' in a unix shell, ./server to start up the server first, Then
** ./client for each user client you would like to start up.  Clients will follow
** the onscreen instructions to proceed.
**
*/

#include "Programming2.h"
#include "StringList.h"

int sock;
struct sockaddr_in servAddr;
unsigned short servPort;
char *servIP;
char buffer[ECHOMAX+1];
int respStringLen;
struct User user;
bool privateChatMode = false;

processThread_t *receiveProcess;
pthread_mutex_t mutex;

StringList_t *fileTransferList;

char *getTime(bool shouldPrint, bool isFormatted);
void login();
void logout();
void getActiveUsers();
void sendPrivateMessage();
void broadcastMessage();
void sendFile();
void showSendRequests();
void respondToSendRequests();
void downloadFile();
void startPrivateChat(char *user);
Message_t createMessage(char *receiverName, char *messageContents, MESSAGE_TYPE messageType, char *data);
void sendMessageToServer(Message_t message);
Message_t receiveMessageFromServer(bool shouldPrint);
Message_t peekMessageFromServer();
char *prompt(char *prompt, char *promptErrorMessage, int maxLength, bool usesYesNoValidation);
void *createReceiveThread();
void *establishHost();

int main(int argc, char *argv[])
{
  pthread_mutex_init(&mutex, NULL);
  establishHost();
  login();

  receiveProcess = (processThread_t*)malloc(sizeof(processThread_t));
  pthread_create(&receiveProcess->thread, NULL, createReceiveThread, NULL);

  bool loggedIn = true;

  while(loggedIn)
  {
    printf("\n============= Welcome To Chatroom 450 ================\n\n");
    printf("1: Get List Of Active Users\n");
    printf("2: Send a Private Message\n");
    printf("3: Broadcast a Message\n");
    printf("4: Send a File to Another User\n");
    printf("5: Show File Send Requests\n");
    printf("6: Respond to File Send Requests\n");
    printf("7: Logout\n");

    printf("\nEnter Your Selection: ");

    int selection;
    scanf("%d", &selection);
    printf("\n");
    scanf("%*[^\n]"); scanf("%*c"); // clear input Buffer

    switch(selection)
    {
      case 1: getActiveUsers();
        break;
      case 2: sendPrivateMessage();
        break;
      case 3: broadcastMessage();
        break;
      case 4: sendFile();
        break;
      case 5: showSendRequests();
        break;
      case 6: respondToSendRequests();
        break;
      case 7: logout(); loggedIn = false;
        break;
    }
  }

  return 0;
}
/* ====================== getTime ==============================
** Get the current local data and time
**
** @params shouldPrint - whether or not to also print to stdout
**         isFormatted - boolean, changes format of output
** @returns time string in Www Mmm dd hh:mm:ss yyyy format
*/
char *getTime(bool shouldPrint, bool isFormatted)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    if(shouldPrint)
      printf ( "Current local time and date: %s", asctime (timeinfo) );

    if(!isFormatted)
      return asctime(timeinfo);
    else
    {
        char tmp[50] = "";
        char *buff = tmp;
        strftime(buff, sizeof tmp, "%A %I:%M:%S", timeinfo);

        return buff;
    }
}
/* ====================== login ==============================
** Send a request to the server to log in under a specified
** user name
**
** @param none
** @returns none
*/
void login()
{
  char *userName = prompt("Enter your user name", "User name cannot be longer than", MAX_USER_NAME, false);
  strcpy(user.name, userName);

  getTime(true, false);

  Message_t message = createMessage("", "", LOGIN_REQUEST, NULL);
  sendMessageToServer(message);

  pthread_mutex_lock(&mutex); // mutex lock to protect shared buffer during write
  Message_t serverResponse = receiveMessageFromServer(true);
  pthread_mutex_unlock(&mutex);

  if(serverResponse.type == REQUEST_FAILURE)
    login();
}
/* ====================== logout ===========
** Send a request to the server to log out
**
** @param none
** @returns none
*/
void logout()
{
  Message_t message = createMessage("", "", LOGOUT_REQUEST, NULL);
  sendMessageToServer(message);
}
/* ====================== getActiveUsers ======================
** Send a request to the server to get a comma delimited List
** of currently active users
**
** @param none
** @returns none
*/
void getActiveUsers()
{
  Message_t message = createMessage("", "", USER_LIST_REQUEST, NULL);
  sendMessageToServer(message);

  pthread_mutex_lock(&mutex); // mutex lock to protect shared buffer during write
  receiveMessageFromServer(true);
  pthread_mutex_unlock(&mutex);
}
/* ====================== sendPrivateMessage ===================
** Send a request to the server to send a private message to
** another user that is currently online
**
** @param none
** @returns none
*/
void sendPrivateMessage()
{
  char *receiverName = prompt("Enter the name of the user", "User name cannot be longer than", MAX_USER_NAME, false);
  char *messageContents = prompt("Enter the message", "Message cannot be longer than", MAX_MESSAGE_SIZE, false);

  Message_t message = createMessage(receiverName, messageContents, PRIVATE_MESSAGE_REQUEST, NULL);
  Message_t serverResponse;

  sendMessageToServer(message);

  pthread_mutex_lock(&mutex); // mutex lock to protect shared buffer during write
  serverResponse = receiveMessageFromServer(true);
  pthread_mutex_unlock(&mutex);

  // if the request was successful, ask the user if they would like to start a live chat session
  if(serverResponse.type == REQUEST_SUCCESS)
  {
    char *shouldBeginPrivateChat = prompt("Would you like to begin a private chat session? (yes/no)", NULL, 0, true);

    if(strcmp(shouldBeginPrivateChat, VALIDATION_YES) == 0) { startPrivateChat(message.receiver.name); }
  }
}
/* ====================== broadcastMessage ===================
** Send a request to the server to broadcast a message to
** all users that are currently online
**
** @param none
** @returns none
*/
void broadcastMessage()
{
  char *messageContents = prompt("Enter the message", "Message cannot be longer than", MAX_MESSAGE_SIZE, false);
  Message_t message = createMessage("", messageContents, BROADCAST_MESSAGE_REQUEST, NULL);
  sendMessageToServer(message);
}
/* ========================== sendFile ========================
** Send a request to the server to send a file to another user
**
** @param none
** @returns none
*/
void sendFile()
{
  char *fileName = prompt("Enter the name of the file you would like to send", "Filename cannot be longer than", MAX_MESSAGE_SIZE, false);
  char *receiverName = prompt("Enter the name of the user", "User name cannot be longer than", MAX_USER_NAME, false);

  Message_t message = createMessage(receiverName, fileName, FILE_SEND_REQUEST, NULL);
  sendMessageToServer(message);

  pthread_mutex_lock(&mutex); // mutex lock to protect shared buffer during write
  receiveMessageFromServer(true);
  pthread_mutex_unlock(&mutex);
}
/* ========================== sendFile ==========================
** Display the file send requests currently pending for this user
**
** @param none
** @returns none
*/
void showSendRequests()
{
  char *currentFiles = getStringList(&fileTransferList);
  if(strlen(currentFiles) == 0) currentFiles = "No files found.";
  printf("These are the files waiting for download: \n%s\n", currentFiles);
}
/* ========================== respondToSendRequests ==========================
** Respond to send requests sent by other users.  When a transfer request is
** sent by another user, the filename is saved into the 'fileTransferList' on
** this client.  Then we can later loop through that list in this function and
** allow to user to either accept or decline each request
**
** @param none
** @returns none
*/
void respondToSendRequests()
{
  StringList_t *current = fileTransferList;

  if(current == NULL)
  {
      printf("No files waiting for transfer.\n");
      return;
  }

  while (current != NULL)
  {
    char downloadPrompt[MAX_MESSAGE_SIZE + 50];
    sprintf(downloadPrompt, "Would you like to download the file \"%s\"? (yes/no)", current->string);

    char *shouldDownloadFile = prompt(downloadPrompt, NULL, 0, true);

    // Tell the server we would like to download the file
    if(strcmp(shouldDownloadFile, VALIDATION_YES) == 0)
    {
      Message_t message = createMessage("", (char *)VALIDATION_YES, FILE_RECV_REQUEST, current->string);
      sendMessageToServer(message);
      downloadFile();
      removeFromStringList(current->string, &fileTransferList);
    }
    else // decline to download the file
    {
      Message_t message = createMessage("", (char *)VALIDATION_NO, FILE_RECV_REQUEST, current->string);
      sendMessageToServer(message);
      removeFromStringList(current->string, &fileTransferList);
    }

    current = current->next;
  }
}
/* ========================== downloadFile ==========================
** Download the file from the server
**
** @param none
** @returns none
*/
void downloadFile()
{
  pthread_mutex_lock(&mutex);

  char *fileName = "download.txt";
  FILE *output = fopen(fileName, "ab");
  Message_t message;

  while(1)
  {
      message = receiveMessageFromServer(false);

      if(strcmp(message.data, "EOF") != 0) // server sends EOF to the data field when transfer is complete
      {
        fputs(message.contents, output);
      }
      else
          break;
  }
  fclose(output);
  pthread_mutex_unlock(&mutex);

  printf("\nDownload Complete.  Saved to file: %s\n", fileName);
}

/* ====================== startPrivateChat =====================
** Starts a private chat session with a specified user.  In this
** mode, all input sent to the server will be directed to the
** user on the opposite end of the chat session until the calling
** user exits the session
**
** @param user - the name of the user on the other end of the chat
** @returns none
*/
void startPrivateChat(char *user){

  privateChatMode = true;
  printf("You are now chatting with: %s\nEnter 'q' to exit.\n", user);

  while(1)
  {
    char *messageContents = prompt(">", "Message cannot be longer than", MAX_MESSAGE_SIZE, false);

    if(messageContents[0] == 'q'){ break; }
    else
    {
      Message_t message = createMessage(user, messageContents, PRIVATE_MESSAGE, NULL);
      sendMessageToServer(message);
    }
  }
  privateChatMode = false;
}
/* ====================== createMessage =====================
** Packs a message into a format that can be easily processed
** by the server
**
** @params receiverName - the name of the recipient
**         messageContents - contents of the messageType
**         messageType - the type of message being Sent
**
** @returns message formatted as struct Message_t
*/
Message_t createMessage(char *receiverName, char *messageContents, MESSAGE_TYPE messageType, char *data)
{
    Message_t message;
    User_t receiver;

    strcpy(receiver.name, receiverName);
    strcpy(message.contents, messageContents);
    if(data != NULL) { strcpy(message.data, data); }
    message.type = messageType;
    message.sender = user;
    message.receiver = receiver;

    return message;
}
/* ====================== sendMessageToServer =====================
** Send a message to the server
**
** @params message - the message to send
** @returns none
*/
void sendMessageToServer(Message_t message)
{
    if (sendto(sock, &message, sizeof(message), 0, (struct sockaddr *)&servAddr, sizeof(servAddr)) != sizeof(message))
      DieWithError("send() sent a different number of bytes than expected");
}
/* ====================== receiveMessageFromServer ===========================
** Receive a message from the server
**
** @params shouldPrint - true/false if the message should be printed to stdout
** @returns message formatted as struct Message_t
*/
Message_t receiveMessageFromServer(bool shouldPrint)
{
    struct sockaddr_in fromAddr;
    unsigned int fromSize = sizeof(fromAddr);

    if ((respStringLen = recvfrom(sock, buffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) == 0)
      DieWithError("recvfrom() failed");

    Message_t message;
    memcpy(&message, buffer, sizeof(message));

    if(shouldPrint)
    {
      char *time = getTime(false, true);
      printf("%s: %s --- %s\n", time, message.sender.name, message.contents);
    }


    return message;
}
/* ====================== peekMessageFromServer =======================
** Peek at a message from the server.  This is used to that the secondary
** receive thread doesn't block messages that are intended for the main
** thread.  The method is similar to receiveMessageFromServer, but it has
** the MSG_PEEK flag set.  This allows the 2nd thread to look at the message,
** determine the message type, and either proceed with processing it, or do
** nothing, which allows the next call to recvfrom to process the message
** after it releases the mutex.
**
** @params none
** @returns message formatted as struct Message_t
*/
Message_t peekMessageFromServer()
{
    struct sockaddr_in fromAddr; /* Source address of echo */
    unsigned int fromSize = sizeof(fromAddr);

    if ((respStringLen = recvfrom(sock, buffer, ECHOMAX, MSG_PEEK, (struct sockaddr *) &fromAddr, &fromSize)) == 0)
      DieWithError("recvfrom() failed");

    Message_t message;
    memcpy(&message, buffer, sizeof(message));

    return message;
}
/* ============================ prompt ================================
** Prompts the user for input, with light error handling
**
** @params prompt - the prompt to display to the user
**         promptErrorMessage - the error message to display on occurence
**         maxLength - max characters for the desired input
**         usesYesNoValidation - boolean for a yes/no prompt response
**
** @returns Char* string
*/
char *prompt(char *prompt, char *promptErrorMessage, int maxLength, bool usesYesNoValidation)
{
  char *input = calloc(256, sizeof(char));

  while(1)
  {
      printf("%s: ", prompt);
      if(fgets(input, 256, stdin) == NULL)
      {
          printf("\n");
          continue;
      }
      else if(input[0] == '\n') // handle enter key press
      {
          continue;
      }
      else if(usesYesNoValidation)
      {
          input[strlen(input) - 1] = '\0';

          if(strcmp(input, VALIDATION_YES) == 0 || strcmp(input, VALIDATION_NO) == 0){ return input; }
          else
          {
            printf("\nInvalid Input\n");
            continue;
          }
      }
      else if(strlen(input) > maxLength)
      {
          printf("\n%s %d characters\n\n", promptErrorMessage, maxLength);
          continue;
      }
      else{ break; }
  }

  input[strlen(input) - 1] = '\0'; // remove any extra bytes

  return input;
}
/* ====================== createReceiveThread =======================
** Creates a secondary thread to allow for nonblocking receives while
** waiting for user input from the main menu
**
** @params none
** @returns none
*/
void *createReceiveThread()
{
    while(1)
    {
      pthread_mutex_lock(&mutex); // mutex lock to protect shared buffer during write

      Message_t message = peekMessageFromServer();

      if(message.type == PRIVATE_MESSAGE)
      {
        if(!privateChatMode) { printf("\n"); }

        receiveMessageFromServer(true);

        if(privateChatMode){ printf(">: "); fflush(stdout); }
        else{ printf("Enter Your Selection: "); fflush(stdout); }
      }
      else if(message.type == FILE_SEND_REQUEST)
      {
        if(!privateChatMode) { printf("\n"); }

        receiveMessageFromServer(true);
        addToStringList(message.data, &fileTransferList);

        if(privateChatMode){ printf(">: "); fflush(stdout); }
        else{ printf("Enter Your Selection: "); fflush(stdout); }
      }

      pthread_mutex_unlock(&mutex);
    }
}
/* ====================== establishHost =======================
** Establish a UDP connection to the server for passing messages
** back and forth
**
** @params none
** @returns none
*/
void *establishHost()
{
  char hostbuffer[256];
  char *IPbuffer;
  struct hostent *host_entry;

  gethostname(hostbuffer, sizeof(hostbuffer)); // Retrieve hostname
  host_entry = gethostbyname(hostbuffer); // Retrieve host information
  IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])); // Convert network address to ascii string

  printf("Hostname: %s\n", hostbuffer);
  printf("Host IP: %s\n", IPbuffer);

  // Set the ip address and port
  servIP = IPbuffer;
  servPort = PORT;

  /* Create a datagram/UDP socket */
  if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))<0)
    DieWithError("socket() failed");

  /*Construct the server address structure*/
  memset(&servAddr, 0, sizeof(servAddr)); /* Zero out structure */
  servAddr.sin_family = AF_INET;   /* Internet addr family */
  servAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
  servAddr.sin_port = htons(servPort);  /* Server port */
}
