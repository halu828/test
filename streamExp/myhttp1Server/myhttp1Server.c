/* このままだと，同時接続には耐えられないが，コンテナ1つにつき1ユーザと考えるとこれでも良い．
 * もしコンテナ1つにつき1ユーザでなくなった場合は，マルチプロセスに変更する．
 * コンパイル: $ gcc -o webserver webserver.c
 * usage: $ sudo ./webserver 80
 	$ sudo ./webserver
 		この場合，勝手に80番ポートが設定される．
 	ブラウザでhttp://localhost/index.htmlにアクセス
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

/*#define DOCUMENT_ROOT "./htdocs"*/
#define BUF_SIZE 1024
#define SIZE 32768

#define DEBUG

int sockfd;

/* Ctrl+Cの割り込みで行う処理 */
void SigHandler(int SignalName) {
	fprintf(stderr, "\nWebサーバを終了します．\n");
	close(sockfd);
	exit(0);
}


int main(int argc, char *argv[]) {
	struct sockaddr_in server;
	int port = (argc == 2) ? atoi(argv[1]) : 80;
	char path[512] = "";
	time_t timer; /* 年月日と時刻 */
	struct tm *local; /* 年月日と時刻 */
	unsigned char responce[SIZE] = "";
	FILE *fp;

	/* SIGINTにSigHandler()を登録 */
	signal(SIGINT, SigHandler);

	/* カレントディレクトリ取得 */
	getcwd(path, 512);

	/* socket作成 */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	/* Port, IPアドレスを設定(IPv4) */
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	/* bind()失敗対策 */
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

	/* ディスクリプタとポート番号の設定 */
	if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	
	/* listen準備 */
	if (listen(sockfd, SOMAXCONN) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* clientからの接続待ちとレスポンスを返すループ */
	while (1) {
		struct sockaddr_in client;
		int newfd;

		char buf[SIZE] = "";
		char method[BUF_SIZE] = "";
		char url[BUF_SIZE] = "";
		char protocol[BUF_SIZE] = "";
		int len, giflen;

		int filefd;
		char file[BUF_SIZE] = "";


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
		/* 現在時刻を取得 */
		timer = time(NULL);
		local = localtime(&timer); /* 地方時に変換 */

		/* bufからメソッドとURLとプロトコルを取得 */
		sscanf(buf, "%s %s %s", method, url, protocol);

		/* 年月日と現在時刻を表示 */
		printf("%4d/%02d/%02d %02d:%02d:%02d %s\n", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, url);










			/* htmlファイルを送るヘッダファイルを送る */
			char *header = "HTTP/1.1 200 OK\r\n"
						"Content-type: text/html\r\n\r\n";
			send(newfd, header, strlen(header), 0);

			fp = fopen("stream.html", "rb");
			if(fp == NULL){
				printf("読込用ファイルが開けません\n");
				return -1;
			}
			len = fread(responce, sizeof(unsigned char), SIZE, fp);
			fclose(fp);


			/* HTMLデータを送信 */
			if (send(newfd, responce, len, 0) == -1) {
				fprintf(stderr, "Failed to send!\n");
				exit(1);
			}

			printf("HTMLデータを送信\n");




			/* このあと，gifのファイルのリクエストがくるので，そのリクエストに対してそのファイルをバイナリで開いてDATAフレーム作って送る．クライアントから送られてくるリクエストは全部アップグレードにする？ */
			// fp = fopen("gifsize.txt", "rb");
			// len = fread(responce, sizeof(unsigned char), SIZE, fp);
			// fclose(fp);
			// printf("%d\n", len);

			header = "HTTP/1.1 200 OK\r\n"
						"Content-type: image/gif\r\n\r\n";

			int i;
			unsigned char gifbuf[SIZE] = "";

			for (i = 1; i <= 10; i++) {
				/* 受け取ってすぐ返さない．そこで，iが1と2のときは返さずに一定の秒数経ってからfor文抜けて，whileで回して一定の秒数経ってたら送る． */
				/* http1だったら，このままの感じで同じ秒数だけsleepさせる． */
				// printf("recv request\n%s\n",buf);
				recv(newfd, buf, sizeof(buf), 0);

				/* iが1と2のときは単純に2秒待つ */
				if (i == 1) sleep(2);
				else if (i == 2) sleep(2);

				send(newfd, header, strlen(header), 0);
				// send(newfd, responce, len, 0);
				sprintf(file, "./image/%02d.gif", i);
				printf("%s\n", file);
				fp = fopen(file, "rb");
				giflen = fread(gifbuf, sizeof(unsigned char), SIZE, fp);
				fclose(fp);
				// printf("%d\n", giflen);
				send(newfd, gifbuf, giflen, 0);

			}



		close(newfd);
	} /* while (1) end */


} /* main end */
