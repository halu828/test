#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define SIZE 32768

int strrep(char *buf, char *front, char *behind);


int main(int argc, char *argv[]) {
  struct sockaddr_in serverHost;
  struct hostent *ServerInfo;
  int serverSocket = 0;
  int port = 80;
  char ipaddr[15] = "localhost";
  char buf[SIZE] = "GET /stream.html HTTP/1.1\r\n"
                  "Host: localhost\r\n"
                  "Accept: text/html\r\n"
                  "User-Agent: myhttp1client\r\n\r\n";

  if (argc >= 2) strcpy(ipaddr, argv[1]);
  if (argc == 3) port = atoi(argv[2]);
  printf("ipaddr:\t%s\nport:\t%d\n\n", ipaddr, port);

  printf("[request]\n%s", buf);

  /* ホストからIPアドレスを求める */
  if ((ServerInfo = gethostbyname(ipaddr)) == NULL) {
    fprintf(stderr, "Failed to find host!\n");
    exit(1);
  }

  /* サーバとの通信用のソケットを作成 */
  if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Failed to make a serverSocket!\n");
    exit(1);
  }

  serverHost.sin_family = AF_INET;
  serverHost.sin_port = htons(port); /* webサーバならば通常は80を代入(well-known port) */
  memcpy((char *)&serverHost.sin_addr, (char *)ServerInfo->h_addr_list[0], ServerInfo->h_length);

  /* webサーバと接続 */
  if (connect(serverSocket, (struct sockaddr *)&serverHost, sizeof(serverHost)) == -1) {
    fprintf(stderr, "Failed to connect with the web server!\n");
    exit(1);
  }

  /* サーバへリクエストを転送 */
  if (send(serverSocket, buf, sizeof(buf), 0) == -1) {
    fprintf(stderr, "Failed to send!\n");
    exit(1);
  }


  printf("[responce]\n");
  /* 最初のレスポンス */
  recv(serverSocket, buf, sizeof(buf), 0);
  printf("%s", buf);



  /* 以下はgifファイルをもらう */
  char request[SIZE] = "GET /image/01.gif HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Accept: image/gif\r\n"
                      "User-Agent: myhttp1client\r\n\r\n";
  char file[15] = "";
  char temp[15] = "";
  int i;

  for (i = 1; i <= 10; i++) {
    printf("[i = %d]\n", i);
    /* リクエスト編集 */
    sprintf(file, "%02d.gif", i);
    strrep(request, temp, file);
    strcpy(temp, file);

    if (send(serverSocket, request, sizeof(request), 0) == -1) {
      fprintf(stderr, "Failed to send!\n");
      exit(1);
    }

    memset(buf, '\0', sizeof(buf));
    if (recv(serverSocket, buf, sizeof(buf), 0) <= 0) {
      fprintf(stderr, "--\nrecvSizeが0以下\n");
      break;
    }
    // recv(serverSocket, buf, sizeof(buf), 0);
    printf("%s\n", buf);
  }

  close(serverSocket);
  return 0;
}


/* 置換する. buf の中の front を behind にする．成功=1 失敗=0 */
int strrep(char *buf, char *front, char *behind) {
  char *ch;
  size_t frontlen, behindlen;

  frontlen = strlen(front);
  behindlen = strlen(behind);
  if (frontlen == 0 || (ch = strcasestr(buf, front)) == NULL) return 0;
  memmove(ch + behindlen, ch + frontlen, strlen(buf) - (ch + frontlen - buf ) + 1);
  memcpy(ch, behind, behindlen);
  return 1;
}