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
    perror("socket");
    return 1;
  }

  int port = 6969;

  addr.sin_family = AF_INET; 
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  int opt = 1;
  int server_opt = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (server_opt == -1) {
    fprintf(stderr, "error setting socket options\n");
    return 1;
  }

  int binded_addr = bind(server, (struct sockaddr*) &addr, sizeof(addr));
  if (binded_addr == -1) {
    perror("bind");
    return 1;
  }

  int is_listening = listen(server, 10);
  if (is_listening == -1) {
    perror("listen");
    return 1;
  }
  printf("server running on port %d\n", port);

  FILE *fp = fopen("todo.db", "r");
  if (fp == NULL)
    printf("no database file found\n");
  if (fp) {
    while(fgets(task_list[task_count], MAX_TASK_LEN, fp) != NULL) {
      task_list[task_count][strcspn(task_list[task_count], "\n")] = '\0';
      task_count++;
    } 
    fclose(fp);
  }
  
  while(1) {
    int client = accept(server, NULL, NULL);
    if (client == -1) {
      perror("accept");
      break;
    }

    char request_buf[2048] = {0};
    int request = read(client, request_buf, sizeof(request_buf) - 1);
    if (request <= 0) {
      fprintf(stderr, "failed to allocate buffer\n");
      return 1;
    }
    else {
      request_buf[request] = '\0';
    }
    printf("request:\n%s\n", request_buf);

    if (strncmp(request_buf, "POST /add", 9) == 0) {
      char *body = strstr(request_buf, "\r\n\r\n");
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
          if (task_count < MAX_TASKS && strlen(task) > 0) {
            strncpy(task_list[task_count], task, sizeof(task_list[task_count]));
            task_list[task_count][MAX_TASK_LEN - 1] = '\0';
            task_count++;
          }
        }
      }
    }

    if (strncmp(request_buf, "POST /delete", 12) == 0) {
      char *body = strstr(request_buf, "\r\n\r\n");
      if (body) {
        body += 4;
        char *equal_sign = strstr(body, "=");
        if (equal_sign) {
          char *task = equal_sign + 1; 
          char *tmp;
          int task_to_delete = (int) strtol(task, &tmp, 10) ;
          if (task_to_delete >= 0 && task_to_delete < task_count) {
            for (int i = task_to_delete; i < task_count - 1; i++) {
              strcpy(task_list[i], task_list[i+1]) ;
            }
            task_list[task_count-1][0] = '\0';
            task_count--;
          }
        }
      }
    }

    if (strncmp(request_buf, "POST /quit", 10) == 0) {
      break;
    }

    FILE *fp;
    fp = fopen("index.html", "r");
    if (fp == NULL) {
      perror("fopen");
      return 1;
    }

    fseek(fp, 0, SEEK_END);
    long file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *html = malloc(file_len + 1);
    if (html == NULL) {
      fprintf(stderr, "failed to allocate memory\n");
      return 1;
    }

    fread(html, 1, file_len, fp);
    html[file_len] = '\0';

    char task_list_html[35000] = {0};
    for (int i = 0; i < task_count; i++) {
      char task_line[350] = {0};
      if (task_list[i][0] != '\0') {
        snprintf(task_line, sizeof(task_line),
              "<li>\n"
              "  %s\n"
              "  <form method=\"POST\" action=\"/delete\" style=\"display:inline\">\n"
              "    <input type=\"hidden\" name=\"index\" value=\"%d\">\n"
              "    <button type=\"submit\">X</button>\n"
              "  </form>\n"
              "</li>\n"
               , task_list[i], i);
        strncat(task_list_html, task_line, sizeof(task_list_html) - strlen(task_list_html) - 1);
      }
    }

    char *insert_pos = strstr(html, "  </ul>");
    if (!insert_pos) {
      fprintf(stderr, "error parsing html\n");
      free(html);
      fclose(fp);
      close(client);
      return 1;
    }
    int offset = insert_pos - html;
    size_t task_list_len = strlen(task_list_html);

    char *temp = realloc(html, file_len + 1 + strlen(task_list_html));
    if (html == NULL) {
      fprintf(stderr, "failed to reallocate memory\n");
      free(html);
      fclose(fp);
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
    free(html);
    fclose(fp);
    close(client);
  }

  FILE *todo_fp;
  todo_fp = fopen("todo.db", "w");
  if (todo_fp == NULL) {
    perror("fopen");
    fclose(fp);
    return 1;
  }
  for (int i = 0; i < task_count; i++) {
    fprintf(todo_fp, "%s\n", task_list[i]);
  }
  fclose(todo_fp);

  return 0;
}

