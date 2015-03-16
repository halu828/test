#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define SIZE 32768

int strrep(char *buf, char *front, char *behind); /* 置換 */
int dump(); /* dumpする */


int main(int argc, char *argv[]) {
  struct sockaddr_in serverHost;
  struct hostent *ServerInfo;
  int recvSize = 0, serverSocket = 0;
  int port = 80;
  char ipaddr[15] = "localhost";
  char requestHeader[] = "GET / HTTP/1.1\r\n"
                          "Host: localhost\r\n"
                          "Connection: Upgrade, HTTP2-Settings\r\n"
                          "Upgrade: h2c-14\r\n"
                          "HTTP2-Settings: AAMAAABkAAQAAP__\r\n"
                          "Accept: */*\r\n"
                          "User-Agent: myclient\r\n\r\n";
  unsigned char buf[SIZE] = "";
  unsigned char prism[128] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
                            "dÿÿ";
  FILE *fpw, *fpr;

  int i;
  char file[1024] = "";


  if (argc >= 2) strcpy(ipaddr, argv[1]);
  if (argc == 3) port = atoi(argv[2]);
  printf("ipaddr:\t%s\nport:\t%d\n\n", ipaddr, port);

  strrep(requestHeader, "nghttp2.org", ipaddr);
  printf("[request]\n%s", requestHeader);

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
  serverHost.sin_port = htons(port);
  memcpy((char *)&serverHost.sin_addr, (char *)ServerInfo->h_addr_list[0], ServerInfo->h_length);

  /* webサーバと接続 */
  if (connect(serverSocket, (struct sockaddr *)&serverHost, sizeof(serverHost)) == -1) {
    fprintf(stderr, "Failed to connect with the web server!\n");
    exit(1);
  }

  /* サーバへリクエストを転送 */
  if (send(serverSocket, requestHeader, sizeof(requestHeader), 0) == -1) {
    fprintf(stderr, "Failed to send!\n");
    exit(1);
  }


  /* サーバからレスポンスをもらう */
  printf("[responce]\n");
  fpw = fopen("binary.txt", "wb");
  if (fpw == NULL) {
    printf("書込用ファイルが開けません\n");
    return -1;
  }

  recvSize = recv(serverSocket, buf, sizeof(buf), 0);
  fwrite(buf, sizeof(unsigned char), recvSize, fpw);
  fclose(fpw);

  /* GOWAYを送る */
  // memset(buf, '\0', sizeof(buf));
  // fpr = fopen("goaway.txt", "rb");
  // if(fpr == NULL){
  //   printf("読込用ファイルが開けません\n");
  //   return -1;
  // }
  // fread(buf, sizeof(unsigned char), 17, fpr);
  // fclose(fpr);
  // if (send(serverSocket, buf, 17, 0) == -1) {
  //   fprintf(stderr, "Failed to send!\n");
  //   exit(1);
  // }
  // fclose(fpr);

  /* どこかでPRIを送らないといけないかもしれない */

  /* dump表示処理 */
  memset(buf, '\0', sizeof(buf));
  dump();

  printf("dump\n");

 

 /* 作ったヘッダを送る */
  memset(requestHeader, '\0', sizeof(requestHeader));
  fpr = fopen("headers.txt", "rb");
  fread(requestHeader, sizeof(unsigned char), SIZE, fpr);
  fclose(fpr);
  printf("send HEADERS\n");

  for (i = 1; i <= 10; i++) {
    send(serverSocket, requestHeader, sizeof(requestHeader), 0); /* 適当ヘッダフレーム送信 */
  }

  for (i = 1; i <= 10; i++) {
    recv(serverSocket, buf, 9, 0); /* フレーム冒頭部分受信 */
    recv(serverSocket, buf, 327, 0); /* フレームペイロード部分受信 */
    sprintf(file, "%02d.gif", i);
    printf("%s\n", file);
    fpw = fopen(file, "wb");
    fwrite(buf, sizeof(unsigned char), recvSize, fpw);
    fclose(fpw);
  }

  // printf("owari\n");

  close(serverSocket);
  return 0;
} // main()終了


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

int dump() {
  FILE *fpr;
  int len, data, i, index = 0;
  unsigned char buf[SIZE];
  unsigned char temp[SIZE];
  unsigned long addr;

  fpr = fopen("binary.txt", "rb");
  if(fpr == NULL){
    printf("読込用ファイルが開けません\n");
    exit(1);
  }
  len = fread(buf, sizeof(unsigned char), SIZE, fpr);
  fclose(fpr);

  for (addr = 0; ; addr += 16) {
    printf("%08lX  ", addr);
    for (i = 0; i < 16; i++) {
      data = buf[index++];
      if (index > len) {
        temp[i] = '\0';
        for (;i < 16; i++) printf("   ");
        printf("  %s\n", temp);
        return 0;
      }
      /* ASCIIコード0x20から0x7Eを表示し，それ以外は表示できないので'.'を表示する */
      if(data < 0x20 || data >= 0x7F) temp[i] = '.';
      else temp[i] = data;
      printf("%02X ", data);
    }
    temp[i] = '\0';
    printf("  %s\n", temp);
  }
}
