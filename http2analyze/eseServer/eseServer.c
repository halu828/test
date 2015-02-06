#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define RES_SIZE 32768

int sockfd;

/* Ctrl+Cの割り込みで行う処理 */
void SigHandler(int SignalName) {
  fprintf(stderr, "\nWebサーバを終了します．\n");
  close(sockfd);
  exit(0);
}


int main(int argc, char *argv[]) {
  struct sockaddr_in server;
  int opt = 1;
  int port = 80;
  unsigned char responce[RES_SIZE];
  char *httpResponce;
  FILE *fpr;

  if (argc > 2) {
    fprintf(stderr, "usage: ./eseserver [port]\n");
    exit(1);
  }
  if (argc == 2) port = atoi(argv[1]);

  /* SIGINTにSigHandler()を登録 */
  signal(SIGINT, SigHandler);

  /* socket作成 */
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  /* Port, IPアドレスを設定(IPv4) */
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  /* bind()失敗対策 */
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

  /* ディスクリプタとポート番号の設定 */
  if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
    perror("bind");
    exit(1);
  }

  /* listen準備 */
  if (listen(sockfd, SOMAXCONN) < 0) {
    perror("listen");
    exit(1);
  }




  /* clientからの接続待ちとレスポンスを返すループ */
  while (1) {
    struct sockaddr_in client;
    int newfd;
    char buf[BUF_SIZE] = "";
    int len;

    /* clientからの接続を受け付ける */
    memset(&client, 0, sizeof(client));
    len = sizeof(client);
    if ((newfd = accept(sockfd, (struct sockaddr *)&client, (socklen_t *)&len)) < 0) {
      perror("accept");
      exit(1);
    }

    /* request 受付 */
    if (recv(newfd, buf, sizeof(buf), 0) < 0) {
      perror("recv");
      exit(1);
    }
    printf("[request]\n");
    printf("%s\n", buf);

    /* レスポンス */
    printf("[responce]\n");
    if (strstr(buf, "Upgrade: h2c-14\r\n") != NULL) {
      fpr = fopen("upgrade1.txt", "rb");
      if(fpr == NULL){
        printf("読込用ファイルが開けません\n");
        return -1;
      }
      len = fread(responce, sizeof(unsigned char), RES_SIZE, fpr);
      fclose(fpr);
      printf("%s\n", responce);
      if (send(newfd, responce, len, 0) == -1) {
        fprintf(stderr, "Failed to send!\n");
        exit(1);
      }
      /* PRIを受け取る */
      // recv(newfd, buf, sizeof(buf), 0);
      // recv(newfd, buf, sizeof(buf), 0);
      // recv(newfd, buf, sizeof(buf), 0);
      /* 何か2回目送るやつ */
      memset(responce, '\0', sizeof(responce));
      fpr = fopen("upgrade2.txt", "rb");
      if(fpr == NULL){
        printf("読込用ファイルが開けません\n");
        return -1;
      }
      len = fread(responce, sizeof(unsigned char), 9, fpr);
      fclose(fpr);
      if (send(newfd, responce, 9, 0) == -1) {
        fprintf(stderr, "Failed to send!\n");
        exit(1);
      }
      /* GOAWAYを受け取る */
      // if (recv(newfd, buf, sizeof(buf), 0) < 0) {
      //   perror("recv");
      //   exit(1);
      // }
    } else if (strstr(buf, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n") != NULL) {
      fpr = fopen("pri.txt", "rb");
      if (fpr == NULL) {
        printf("読込用ファイルが開けません\n");
        return -1;
      }
      len = fread(responce, sizeof(unsigned char), 48, fpr);
      fclose(fpr);
      // printf("%s\n", responce);
      if (send(newfd, responce, len, 0) == -1) {
        fprintf(stderr, "Failed to send!\n");
        exit(1);
      }
    } else {
      httpResponce = "HTTP/1.1 200 OK\r\n"
              "Content-type: text/html\r\n\r\n"
              "<html>Not HTTP/2 Upgrade!</html>";
      printf("%s\n", httpResponce);
      if (send(newfd, httpResponce, strlen(httpResponce), 0) == -1) {
        fprintf(stderr, "Failed to send!\n");
        exit(1);
      }
    }

    close(newfd);
  }
  return 0;
}
