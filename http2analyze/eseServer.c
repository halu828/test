#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define BUF_SIZE 4096

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
  int port;

  if (argc != 2) {
    fprintf(stderr, "usage: ./eseserver [port]\n");
    exit(1);
  }
  port = atoi(argv[1]);

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
      exit(EXIT_FAILURE);
    }

    /* request line読み込み */
    if (recv(newfd, buf, sizeof(buf), 0) < 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }

    printf("%s\n", buf);

    close(newfd);
  }
}
