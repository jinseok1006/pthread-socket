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
int readAndSend(int, const char[]);
void *run(void *);

int main(void) {
  int s_sock, c_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t c_addr_size;
  pthread_t tid;

  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s_sock == -1) {
    perror("socket");
    exit(1);
  }

  // 포트 재사용
  int option = 1;
  setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  // 서버 주소 설정
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

  c_addr_size = sizeof(client_addr);

  // 클라이언트 accept 무한 대기
  while (1) {
    c_sock = accept(s_sock, (struct sockaddr *)&client_addr, &c_addr_size);
    if (c_sock == -1) {
      perror("accept");
      exit(1);
    }
    int *new_c_sock = malloc(sizeof(int));
    *new_c_sock = c_sock;

    // 연결 수립시 쓰레드로 생성후 소켓 넘김
    pthread_create(&tid, NULL, run, new_c_sock);
    pthread_detach(tid);
  }

  close(s_sock);

  return 0;
}

// 연결이 수립하면 클라이언트로 부터 파일명을 받고 있는지 확인하는 작업
int connection(int c_sock) {
  char path[BUFFSIZE] = "./data/";
  char filename[BUFFSIZE] = {0};
  char buf[BUFFSIZE] = {0};

  // 파일명 받아오기
  if (recv(c_sock, filename, BUFFSIZE, 0) == -1) {
    perror("filename recv");
    exit(1);
  }

  strcat(path, filename);

  // 클라이언트에게 파일이 존재하면 OK, 없으면 NO ITEM 발송
  if (access(path, F_OK) != 0) {
    printf("%d: 클라이언트에서 요청한 파일 %s가 서버에 없어요\n", c_sock, path);
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

  // 클라이언트 수신 가능 여부 받기(한번에 send방지)
  if (recv(c_sock, buf, BUFFSIZE, 0) == -1) {
    perror("ok recv");
    exit(1);
  }
  printf("%d: Client says %s\n", c_sock, buf);

  // 파일이 존재하고 클라이언트가 수신하는데 문제없다면 파일 읽고 전송
  return readAndSend(c_sock, path);
}

// 파일을 열고 읽은다음 클라이언트에게 전송
int readAndSend(int c_sock, const char path[]) {
  FILE *fp;
  int read_count;
  char buf[BUFFSIZE + 1] = {0};
  int count = 0;

  fp = fopen(path, "rb");
  while (1) {
    memset(buf, 0, BUFFSIZE);
    read_count = fread(buf, 1, BUFFSIZE, fp);

    // 더 이상 읽은 값이 없으면 종료
    if (read_count <= 0)
      break;

    // 클라이언트에게 읽은 만큼 전달
    if (send(c_sock, buf, read_count, 0) == -1) {
      perror("file send");
      exit(1);
    }
  }

  printf("%d: %s send successful\n", c_sock, path);

  fclose(fp);

  return 0;
}

// 소켓 연결 수립시 실제 통신을 담당하는 쓰레드
void *run(void *args) {
  int c_sock = *(int *)args;
  printf("New Thread c_sock:%d\n", c_sock);

  connection(c_sock);

  printf("close Thread c_sock:%d\n", c_sock);
  close(c_sock);
  free(args);
  pthread_exit(NULL);
}