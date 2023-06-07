#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFFSIZE 256
#define SERVERPORT 7799
#define ERR_SIZE 16

const char err_msg[ERR_SIZE] = "FILE_NO";
const char ok_msg[ERR_SIZE] = "FILE_OK";

int connection(int);
int readAndPass(int, const char[]);

int main(void) {
  int s_sock, c_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t c_addr_size;

  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s_sock == -1) {
    perror("socket");
    exit(1);
  }

  int option = 1;
  setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVERPORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (bind(s_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    exit(1);
  }

  listen(s_sock, 1);

  // 클라이언트 소켓 통신 성공
  c_addr_size = sizeof(client_addr);
  c_sock = accept(s_sock, (struct sockaddr *)&client_addr, &c_addr_size);
  if (c_sock == -1) {
    perror("accept");
    exit(1);
  }

  // 디버그용
  printf("s_sock=%d, c_sock=%d\n", s_sock, c_sock);
  printf("client IP addr=%s port=%d\n", inet_ntoa(client_addr.sin_addr),
         ntohs(client_addr.sin_port));

  connection(c_sock);

  close(c_sock);
  close(s_sock);

  return 0;
}

int connection(int c_sock) {
  char path[BUFFSIZE];
  char buf[BUFFSIZE];

  // 파일명 받아오기
  if (recv(c_sock, path, BUFFSIZE, 0) == -1) {
    perror("recv");
    exit(1);
  }

  // 파일이 존재하면 OK, 없으면 NO ITEM 발송
  if (access(path, F_OK) != 0) {
    printf("클라언트에서 요청한 파일 %s가 서버에 없어요\n", path);
    if (send(c_sock, err_msg, sizeof(err_msg), 0) == -1) {
      perror("err send");
      exit(1);
    }
    return 1;
  } else {
    if (send(c_sock, ok_msg, sizeof(ok_msg), 0) == -1) {
      perror("ok send");
      exit(1);
    }
  }

  // 클라이언트 수신 가능 여부 받기
  if (recv(c_sock, buf, BUFFSIZE, 0) == -1) {
    perror("recv");
    exit(1);
  }
  printf("[Client]: %s\n", buf);

  readAndPass(c_sock, path);

  return 0;
}

int readAndPass(int c_sock, const char path[]) {
  FILE *fp;
  int read_count;
  char buf[BUFFSIZE + 1] = {0};
  int count = 0;

  fp = fopen(path, "rb");
  while (1) {
    memset(buf, 0, BUFFSIZE);
    read_count = fread(buf, 1, BUFFSIZE, fp);

    if (read_count <= 0)
      break;

    if (send(c_sock, buf, read_count, 0) == -1) {
      perror("file send");
      exit(1);
    }
    // printf("%d:%s", count++, buf);
  }

  printf("End!!\n");
  fclose(fp);

  return 0;
}