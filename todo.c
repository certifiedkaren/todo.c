/* 
todo app in c, later build it in asm
open a webserver for a basic todo application
persistent storage with a text file
*/

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

struct sockaddr_in addr;

typedef struct {
  int id;
  char task[256];
  int completed;
} TODO_ITEM;

TODO_ITEM db[128];
int todo_count = 0;

int main(void) {
  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server == -1) {
    printf("error creating socket\n");
    return 1;
  }

  int port = 6969;

  addr.sin_family = AF_INET; 
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  int binded_addr = bind(server, (struct sockaddr*) &addr, sizeof(addr));
  if (binded_addr == -1) {
    printf("error binding socket to address %d\n", port);
    return 1;
  }

  int is_listening = listen(server, 10);
  if (is_listening == -1) {
    printf("failed to listen on port %d\n", port);
    return 1;
  }
  printf("server running on port %d\n", port);

  while(1) {
    int client = accept(server, NULL, NULL);
    if (client == -1) {
      printf("error with accepting request");
      break;
    }

    char buf[2048] = {0};
    int request = read(client, buf, sizeof(buf));
    if (request == -1) {
      printf("failed to read request");
      break;
    }
    printf("request:\n%s\n", buf);

    FILE *fp;
    fp = fopen("index.html", "r");
    if (fp == NULL) {
      printf("failed to open file");
      return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *contents = malloc(file_len + 1);
    if (contents == NULL) {
      printf("failed to allocate memory");
      return 1;
    }
    fread(contents, 1, file_len, fp);
    contents[file_len] = '\0';

    char header[256];
    sprintf(header, 
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "Content-Length: %ld\r\n"
          "\r\n", file_len);
      
    write(client, header, strlen(header));
    write(client, contents, strlen(contents));
    fclose(fp);
    free(contents);
    close(client);
  }

  return 0;
}

