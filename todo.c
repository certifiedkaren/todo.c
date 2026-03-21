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

#define MAX_TASKS 100
#define MAX_TASK_LEN 128

struct sockaddr_in addr;

char task_list[MAX_TASKS][MAX_TASK_LEN];
int task_count = 0;

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
      printf("error with accepting request\n");
      break;
    }

    char buf[2048] = {0};
    int request = read(client, buf, sizeof(buf) - 1);
    if (request <= 0) {
      printf("failed to allocated buffer\n");
      return 1;
    }
    else {
      buf[request] = '\0';
    }
    printf("request:\n%s\n", buf);

    int is_post = 0;
    char task_buf[128];
    if (strncmp(buf, "POST /add", 9) == 0) {
      is_post = 1;
      char *body = strstr(buf, "\r\n\r\n");
      if (body) {
        body += 4;
        char *equal_sign = strstr(body, "=");
        if (equal_sign) {
          char *task = equal_sign + 1;
          char *ptr = task;
          while (*ptr) {
            if (*ptr == '+')
              *ptr = ' '; 
            ptr++;
          }
          if (task_count < MAX_TASKS) {
            strncpy(task_list[task_count], task, sizeof(task_list[task_count]));
            task_list[task_count][MAX_TASK_LEN - 1] = '\0';
            task_count++;
          }
        }
      }
    }

    FILE *fp;
    fp = fopen("index.html", "r");
    if (fp == NULL) {
      printf("failed to open file\n");
      return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *html = malloc(file_len + 1);
    if (html == NULL) {
      printf("failed to allocate memory\n");
      return 1;
    }

    fread(html, 1, file_len, fp);
    html[file_len] = '\0';

    char task_list_html[2048] = {0};
    for (int i = 0; i < task_count; i++) {
      char task_line[256];
      snprintf(task_line, sizeof(task_line), "    <li>%s</li>\n", task_list[i]);
      strncat(task_list_html, task_line, sizeof(task_list_html) - strlen(task_list_html) - 1);
    }

    char *insert_pos = strstr(html, "  </ul>");
    if (!insert_pos) {
      printf("error parsing html\n");
      free(html);
      close(client);
      return 1;
    }
    int offset = insert_pos - html;
    size_t task_list_len = strlen(task_list_html);

    char *temp = realloc(html, file_len + 1 + strlen(task_list_html));
    if (html == NULL) {
      printf("failed to reallocate memory\n");
      free(html);
      close(client);
      return 1;
    }
    html = temp;
    insert_pos = html + offset;

    memmove(insert_pos + task_list_len, insert_pos, strlen(insert_pos) + 1);
    memcpy(insert_pos, task_list_html, task_list_len);

    char header[256];
    sprintf(header, 
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "Content-Length: %zu\r\n"
          "\r\n", strlen(html));

    write(client, header, strlen(header));
    write(client, html, strlen(html));
    fclose(fp);
    free(html);
    close(client);
  }

  return 0;
}

