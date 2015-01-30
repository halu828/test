#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <err.h>

#define SIZE 50000

/*
GET / HTTP/1.1
Host: server.example.com
Connection: Upgrade, HTTP2-Settings
Upgrade: h2c
HTTP2-Settings: <base64url encoding of HTTP/2 SETTINGS payload>
*/

// GET / HTTP/1.1
// Host: localhost:10080
// Connection: Upgrade, HTTP2-Settings
// Upgrade: h2c-14
// HTTP2-Settings: AAMAAABkAAQAAP__
// Accept: */*
// User-Agent: nghttp2/0.7.1


int main(int argc, char *argv[]) {
  struct sockaddr_in serverHost;
  struct hostent *ServerInfo;
  int serverSocket = 0;
  int recvSize;
  char buf[SIZE] = "GET / HTTP/1.1\r\nHost: nghttp2.org\r\nConnection: Upgrade, HTTP2-Settings\r\nUpgrade: h2c-14\r\nHTTP2-Settings: AAMAAABkAAQAAP__\r\nAccept: */*\r\nUser-Agent: myclient\r\n\r\n";
  // char buf[SIZE] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n\r\n";

  printf("%s", buf);

  /* hostNameからwebサーバのIPアドレスを求める */
  if ((ServerInfo = gethostbyname("nghttp2.org")) == NULL) {
    fprintf(stderr, "Failed to find host!\n");
    exit(1);
  }

  /* サーバとの通信用のソケットを作成 */
  if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Failed to make a serverSocket!\n");
    exit(1);
  }

  serverHost.sin_family = AF_INET;
  serverHost.sin_port = htons(80); /* webサーバならば通常は80を代入(well-known port) */
  memcpy((char *)&serverHost.sin_addr, (char *)ServerInfo->h_addr_list[0], ServerInfo->h_length);

  /* webサーバと接続 */
  if (connect(serverSocket, (struct sockaddr *)&serverHost, sizeof(serverHost)) == -1) {
    fprintf(stderr, "Failed to connect with the web server!\n");
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  /* サーバへリクエストを転送 */
  if (send(serverSocket, buf, sizeof(buf), 0) == -1) {
    fprintf(stderr, "Failed to send!\n");
    exit(1);
  }


  /* サーバからレスポンスをもらう */
  while (1) {
    memset(buf, '\0', sizeof(buf));
    if ((recvSize = recv(serverSocket, buf, sizeof(buf), 0)) <= 0) {
      fprintf(stderr, "--\nrecvSizeが0以下\n");
      break;
    }
    printf("%s", buf);
  }
  close(serverSocket);
  return 0;
}
