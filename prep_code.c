int main(void)
{
    char *hostname = LOCALHOST;
    char messageBuffer[MESSAGEBUFFER];
    int serverSocket, clientSocket;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *serverinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes=1;
    char conn_addr[INET6_ADDRSTRLEN];
    int returnvalue,recvMsgSize;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((returnvalue = getaddrinfo(hostname, portno, &hints, &serverinfo)) != 0) 
    {
        fprintf(stderr, "%s @ %d: [SERVER] Failure at getaddrinfo() (%s)\n", __FILE__, __LINE__, gai_strerror(returnvalue));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = serverinfo; p != NULL; p = p->ai_next) {
        if ((serverSocket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
        {
            fprintf(stderr, "%s @ %d: [SERVER] Failure at socket()\n", __FILE__, __LINE__);
            continue;
        }

        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) 
        {
            fprintf(stderr, "%s @ %d: [SERVER] Failure at setsockopt()\n", __FILE__, __LINE__);
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
        exit(1);
    }

    if (listen(serverSocket, BACKLOG) == -1) 
    {
        fprintf(stderr, "%s @ %d: [SERVER] Failure at listen() with backlog (%d)\n", __FILE__, __LINE__, BACKLOG);
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) 
    {  // main accept() loop
        sin_size = sizeof their_addr;
        clientSocket = accept(serverSocket, (struct sockaddr *)&their_addr, &sin_size);
        if (clientSocket == -1) 
        {
            fprintf(stderr, "%s @ %d: [SERVER] Failure at accept()\n", __FILE__, __LINE__);
            continue;
        }

        inet_ntop(their_addr.ss_family,(struct sockaddr_in*)&their_addr->sin_addr,conn_addr, sizeof conn_addr);
        printf("server: got connection from %s\n", conn_addr);

        if ((recvMsgSize = recv(clientSocket, messageBuffer, MESSAGEBUFFER-1, 0)) == -1)
        {
            fprintf(stderr, "%s @ %d: [SERVER] Failure at recv()\n", __FILE__, __LINE__);
            exit(1);
        }

        messageBuffer[recvMsgSize] = '\0';

        printf("server: received %d bytes: '%s'\n",recvMsgSize,messageBuffer);

        if (send(new_fd, "Hello, world!", 13, 0) == -1)
        {
            fprintf(stderr, "%s @ %d: [SERVER] Failure at send()\n", __FILE__, __LINE__);
            exit(1);
        }

        close(clientSocket);
        break;
    }

    close(serverSocket);

    return 1;
}
