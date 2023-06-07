#include <arpa/inet.h>
#include <netinet/in.h>
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
const char ready_msg[ERR_SIZE] = "READY";

int connection(int c_sock, const char path[], int path_len);
int recvAndWrite(int c_sock, const char[], int);

int main(int argc, char *argv[]) {
  // if (argc != 2) {
  //   printf("주소를 입력하세요.");
  //   return 1;
  // }

  // 클라이언트 소켓 생성
  int c_sock = socket(AF_INET, SOCK_STREAM, 0);

  // 서버 주소 설정
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVERPORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // 서버와 연결
  if (connect(c_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("[Client] connect");
    exit(1);
  }

  char path[] = "data/sample3.txt";
  connection(c_sock, path, sizeof(path) + 1);
  close(c_sock);

  return 0;
}

int connection(int c_sock, const char path[], int path_len) {
  char buf[BUFFSIZE];
  memset(buf, 0, BUFFSIZE);

  // 요청할 파일명 전송하기
  if (send(c_sock, path, path_len, 0) == -1) {
    perror("[Client] send");
    exit(1);
  }
  // 서버로 부터 파일 존재 여부 받아오기
  if (recv(c_sock, buf, BUFFSIZE, 0) == -1) {
    perror("[Client] recv");
    exit(1);
  }
  printf("[Server]: %s\n", buf);

  // 파일이 없으면 함수 종료
  if (strcmp(buf, err_msg) == 0) {
    printf("No Item\n");
    return 1;
  }

  // 수신 가능 상태 전달
  if (send(c_sock, ready_msg, ERR_SIZE, 0) == -1) {
    perror("[Client] send");
    exit(1);
  }

  return recvAndWrite(c_sock, path, path_len);
}

int recvAndWrite(int c_sock, const char path[], int path_len) {
  FILE *fp;
  char buf[BUFFSIZE] = {0};

  // 서버에서 데이터를 받아 파일 쓰기
  fp = fopen(path, "w");
  int count = 0;
  while (1) {
    memset(buf, 0, BUFFSIZE);

    int recv_count = recv(c_sock, buf, BUFFSIZE, 0);

    if (recv_count == -1) {
      perror("[Client] recv");
      exit(1);
    }

    if (recv_count == 0) {
      printf("End!!\n");
      break;
    }

    printf("%d:%s", count++, buf);
    fwrite(buf, 1, recv_count, fp);
  }

  fclose(fp);

  return 0;
}