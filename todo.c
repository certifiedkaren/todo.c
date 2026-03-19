/* 
todo app in c, later build it in asm
open a webserver for a basic todo application
persistent storage with a text file
*/

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

struct sockaddr_in addr;
int main(void) {
  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    printf("error creating socket\n");
    return 1;
  }

  addr.sin_family = AF_INET; 
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(6969);

  int binded_addr = bind(server, (struct sockaddr*) &addr, sizeof(addr));
  if (binded_addr == -1) {
    printf("error binding socket to address %d\n", addr.sin_port);
    return 1;
  }

  listen(server, 10);
  if (server == -1) {
    printf("failed to listen on port %d\n", addr.sin_port);
    return 1;
  }
  printf("server running on port %d\n", addr.sin_port);

  while(1) {
    int client = accept(server, NULL, NULL);
    if (client == -1) {
      printf("error with accepting request");
      break;
    }
    char buf[512] = {0};
    int request = read(client, buf, sizeof(buf));
    if (request == -1) {
      printf("failed to read request");
      break;
    }
    printf("request:\n%s\n", buf);

    char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Hello World\n";
    write(client, response, strlen(response));
    close(client);
  }
  return 0;
}
