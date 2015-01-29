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

//GET / HTTP/1.1\r\nUser-Agent: myClient\r\nHost: localhost\r\nAccept: */*\r\nContent-type: text/plain\r\nTransfer-Encoding: chunked\r\nConnection: Keep-Alive\r\nExpect: 100-continue
/*
GET / HTTP/1.1
Host: server.example.com
Connection: Upgrade, HTTP2-Settings
Upgrade: h2c
HTTP2-Settings: <base64url encoding of HTTP/2 SETTINGS payload>
*/

int main(int argc, char *argv[]) {
	struct sockaddr_in serverHost;
	struct hostent *ServerInfo;
	int serverSocket = 0;
	char buf[SIZE] = "GET / HTTP/1.1\r\nHost: nghttp2.org\r\nConnection: Upgrade, HTTP2-Settings\r\nUpgrade: h2c\r\nHTTP2-Settings: <base64url encoding of HTTP/2 SETTINGS payload>\r\n\r\n";


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

	memset(buf, '\0', sizeof(buf));

	/* サーバからレスポンスをもらう */
	while (1) {
		if ((recv(serverSocket, buf, sizeof(buf), 0)) <= 0) {
			break; /* 受信に失敗したらbreak */
		}
		printf("%s", buf);
	}
	close(serverSocket);
	return 0;
}
