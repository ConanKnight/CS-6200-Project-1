#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>

#include "gfserver.h"
#include "gfserver-student.h"


struct gfserver_t {
  int max_npending; //max pending connections
  unsigned short port; //port which server listens on
  void * handler_arg; //phantom third argument for handler
  ssize_t (*handler)(); //function pointer for handler

};

struct gfcontext_t {
  int socket; //socket descriptor
  size_t bytes_remaining; //how many bytes of a file are left to be sent
};

/*
 * Aborts the connection to the client associated with the input
 * gfcontext_t.
 */
void gfs_abort(gfcontext_t *ctx){
  close(ctx->socket);
  free(ctx); //free the gfcontext_t struct
}

/* 
 * This function must be the first one called as part of 
 * setting up a server.  It returns a gfserver_t handle which should be
 * passed into all subsequent library calls of the form gfserver_*.  It
 * is not needed for the gfs_* call which are intended to be called from
 * the handler callback.
 */
gfserver_t* gfserver_create(){
    struct gfserver_t *gfs;
    if((gfs = malloc(sizeof(struct gfserver_t))) != NULL) {
        gfs->port = 6200;
        gfs->max_npending = MAX_REQUEST_LEN;
        return gfs;
    }
    return (gfserver_t *)NULL;
}

/*
 * Sends size bytes starting at the pointer data to the client 
 * This function should only be called from within a callback registered 
 * with gfserver_set_handler.  It returns once the data has been
 * sent.
 */
ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len){
    ssize_t sendMsgSize;
    if ((sendMsgSize = send(ctx->socket, (char *)data, len, 0)) == -1) {
        fprintf(stderr, "%s @ %d: [SERVER] Failure to send() data\n", __FILE__, __LINE__);
        return -1;
    }
    ctx->bytes_remaining -= sendMsgSize;
    if (ctx->bytes_remaining == 0) gfs_abort(ctx);
    return sendMsgSize;
}

/*
 * Sends to the client the Getfile header containing the appropriate 
 * status and file length for the given inputs.  This function should
 * only be called from within a callback registered gfserver_set_handler.
 */
ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){
    char messageBuffer[BUFFERLEN];
    char statmsg[16];
    ssize_t sendMsgSize;
    ctx->bytes_remaining = file_len;

    if (status == GF_OK) {
        strcpy(statmsg,"OK");
        //sprintf(lenstr, "%d", file_len);
        sprintf(messageBuffer, "GETFILE %s %d%s", statmsg,file_len,MARKER);
    } else {
        if (status == GF_FILE_NOT_FOUND) strcpy(statmsg,"FILE_NOT_FOUND");
        else if (status == GF_ERROR) strcpy(statmsg,"ERROR");
        else strcpy(statmsg,"INVALID");
        sprintf(messageBuffer, "GETFILE %s%s", statmsg,MARKER);
    }

    if ((sendMsgSize = send(ctx->socket, messageBuffer, BUFFERLEN, 0)) == -1) {
        fprintf(stderr, "%s @ %d: [SERVER] Failure to send() header\n", __FILE__, __LINE__);
        return -1;
    }
    if (status != GF_OK)  gfs_abort(ctx);
    return sendMsgSize;
}

/*
 * Starts the server.  Does not return.
 */
void gfserver_serve(gfserver_t *gfs){

  //Declare variables
  int serverSocket;
  char portnumchar[6];
  char messageBuffer[BUFFERLEN];
  struct addrinfo hints, *serverinfo, *p;
  struct sockaddr their_addr; // connector's address information
  socklen_t sin_size;
  int yes=1;
  int returnvalue,recvMsgSize;

  //Initialize some variables
  sprintf(portnumchar, "%d", gfs->port);

  //create the listening socket

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((returnvalue = getaddrinfo(LOCALHOST, portnumchar, &hints, &serverinfo)) != 0) 
  {
    fprintf(stderr, "%s @ %d: [SERVER] Failure at getaddrinfo() (%s)\n", __FILE__, __LINE__, gai_strerror(returnvalue));
    free(gfs); //free the gfserver_t struct
    exit(1);
  }

  for(p = serverinfo; p != NULL; p = p->ai_next) {
    if ((serverSocket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
    {
      fprintf(stderr, "%s @ %d: [SERVER] Failure at socket()\n", __FILE__, __LINE__);
      continue;
    }

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) 
    {
      fprintf(stderr, "%s @ %d: [SERVER] Failure at setsockopt()\n", __FILE__, __LINE__);
      free(gfs); //free the gfserver_t struct
      exit(1);
    }

    if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) 
    {
      close(serverSocket);
      fprintf(stderr, "%s @ %d: [SERVER] Failure at bind()\n", __FILE__, __LINE__);
      continue;
    }

    break;
  }

  freeaddrinfo(serverinfo); // all done with this structure

  if (p == NULL)  
  {
    fprintf(stderr, "%s @ %d: [SERVER] Failed to bind() to any port\n", __FILE__, __LINE__);
    free(gfs); //free the gfserver_t struct
    exit(1);
  }

  if (listen(serverSocket, gfs->max_npending) == -1) 
  {
    fprintf(stderr, "%s @ %d: [SERVER] Failure at listen() with backlog (%d)\n", __FILE__, __LINE__, gfs->max_npending);
    free(gfs); //free the gfserver_t struct
    exit(1);
  }

  //Allocate memory for path and context
  char * path;
  if ((path = malloc(200)) == NULL) {
    fprintf(stderr, "%s @ %d: [SERVER] Failure to malloc() the path variable\n", __FILE__, __LINE__);
    free(gfs); //free the gfserver_t struct
    exit(1);
  }

  struct gfcontext_t *context;
  if((context = malloc(sizeof(struct gfcontext_t))) == NULL) {
    fprintf(stderr, "%s @ %d: [SERVER] Failure to malloc() the context\n", __FILE__, __LINE__);
    free(path); //free the path variable
    free(gfs); //free the gfserver_t struct
    exit(1);
  }

  // Loop for Accepted clients
  while(1) {
    sin_size = sizeof their_addr;
    context->socket = accept(serverSocket, (struct sockaddr *)&their_addr, &sin_size);
    if (context->socket == -1) 
    {
      fprintf(stderr, "%s @ %d: [SERVER] Failure at accept()\n", __FILE__, __LINE__);
      continue;
    }

    // Receive Request
    if ((recvMsgSize = recv(context->socket, messageBuffer, BUFFERLEN-1, 0)) == -1)
    {
      fprintf(stderr, "%s @ %d: [SERVER] Failure at recv()\n", __FILE__, __LINE__);
      free(context); //free the gfcontext_t struct
      free(path); //free the path variable
      free(gfs); //free the gfserver_t struct
      exit(1);
    }

    messageBuffer[recvMsgSize] = '\0'; //null terminate to prevent read errors

    // Parse Request
    char scheme[8];
    memset(&scheme, 0, sizeof scheme);
    char method[4];
    memset(&method, 0, sizeof method);
    char marker[5];
    memset(&marker, 0, sizeof marker);
    char *token = strtok(messageBuffer, " "); //First token is constant scheme: "GETFILE"
    strncpy(scheme,token,sizeof scheme);
    token = strtok(NULL, " "); //Second token is constant method: "GET"
    strncpy(method,token,sizeof method);
    token = strtok(NULL, " "); //Third token is our path + MARKER
    sscanf( token+sscanf( token, "%[^\r]", path ), "%[\r\n]", marker ); //Parse out path + MARKER; if this doesn't work, consider tokenizing by "\r" and comparing marker to "\n\r\n"
    printf("[debug-server] The path is %s\n",path);
    if ((strcmp(scheme,SCHEME) != 0) || (strcmp(method,METHOD) != 0) || (strcmp(marker,MARKER) != 0)) {
      fprintf(stderr, "%s @ %d: [SERVER] Client sent malformed header\n", __FILE__, __LINE__);
      gfs_sendheader(context,GF_INVALID,0); //indicate to client malformed header
      continue;
    }

    //start the handler with the client connection
    gfs->handler(context,path,gfs->handler_arg); 

    //Reset
    memset(&messageBuffer, 0, sizeof messageBuffer);
    
  }
  close(serverSocket);
  free(gfs); //free the gfserver_t struct
  free(path); //free the path variable
}

/*
 * Sets the third argument for calls to the handler callback.
 */
void gfserver_set_handlerarg(gfserver_t *gfs, void* arg){
  gfs->handler_arg = arg;
}

/*
 * Sets the handler callback, a function that will be called for each each
 * request.  As arguments, this function receives:
 * - a gfcontext_t handle which it must pass into the gfs_* functions that 
 * 	 it calls as it handles the response.
 * - the requested path
 * - the pointer specified in the gfserver_set_handlerarg option.
 * The handler should only return a negative value to signal an error.
 */
void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void*)){
  gfs->handler = handler;
}

/*
 * Sets the maximum number of pending connections which the server
 * will tolerate before rejecting connection requests.
 */
void gfserver_set_maxpending(gfserver_t *gfs, int max_npending){
  gfs->max_npending = max_npending;
}

/*
 * Sets the port at which the server will listen for connections.
 */
void gfserver_set_port(gfserver_t *gfs, unsigned short port){
  gfs->port = port;
}

