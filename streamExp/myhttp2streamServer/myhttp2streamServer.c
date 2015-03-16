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

		char *header;

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










		/* http/2通信の処理 */
		if (strstr(buf, "Upgrade: h2c-14\r\n") != NULL) {
			fp = fopen("upgradeheader.txt", "rb");
			if(fp == NULL){
				printf("読込用ファイルが開けません\n");
				return -1;
			}
			len = fread(responce, sizeof(unsigned char), SIZE, fp);
			fclose(fp);
			// printf("%s\n", responce);
			if (send(newfd, responce, len, 0) == -1) {
				fprintf(stderr, "Failed to send!\n");
				exit(1);
			}


			/* DATAフレームの長さを送信 */
			fp = fopen("streamsize.txt", "rb");
			if(fp == NULL){
				printf("読込用ファイルが開けません\n");
				return -1;
			}
			len = fread(responce, sizeof(unsigned char), SIZE, fp);
			fclose(fp);
			if (send(newfd, responce, len, 0) == -1) {
				fprintf(stderr, "Failed to send!\n");
				exit(1);
			}
			/* ここまでで, HTTP/1.1のupgradeとHEADERフレーム, DATAフレームのペイロード前までが送れた */


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
			fp = fopen("gifsize.txt", "rb");
			len = fread(responce, sizeof(unsigned char), SIZE, fp);
			fclose(fp);
			// printf("%d\n", len);

			int i;
			unsigned char gifbuf[SIZE] = "";
			clock_t start1, start2;

			recv(newfd, buf, sizeof(buf), 0); /* リクエストを全部受け取る */
			for (i = 1; i <= 10; i++) {
				/* 受け取ってすぐ返さない．そこで，iが1と2のときは返さずに一定の秒数経ってからfor文抜けて，whileで回して一定の秒数経ってたら送る． */
				/* http1だったら，このままの感じで1と2のときだけ同じ秒数だけsleepさせる． */
				// printf("recv request\n%s\n",buf);
				if (i == 1) {
				/* タイマーをセット．for文抜けてから2秒経ってたら送る． */
					start1 = clock();
				} else if (i == 2) {
					start2 = clock();
				} else {
					send(newfd, responce, len, 0);
					sprintf(file, "./image/%02d.gif", i);
					printf("%s\n", file);
					fp = fopen(file, "rb");
					giflen = fread(gifbuf, sizeof(unsigned char), SIZE, fp);
					fclose(fp);
					// printf("%d\n", giflen);
					send(newfd, gifbuf, giflen, 0);
				}
			}

			/* iが1. 01.gifを2秒経ってたら送信 */
			i = 1;
			sprintf(file, "./image/%02d.gif", i++);
			printf("%s\n", file);
			fp = fopen(file, "rb");
			giflen = fread(gifbuf, sizeof(unsigned char), SIZE, fp);
			fclose(fp);
			// sleep(2);
			while (1) {
				if ((double)(clock() - start1) / CLOCKS_PER_SEC >= 2) {
					send(newfd, gifbuf, giflen, 0);
					break;
				}
			}
			

			/* iが2. 02.gifを2秒経ってたら送信 */
			sprintf(file, "./image/%02d.gif", i);
			printf("%s\n", file);
			fp = fopen(file, "rb");
			giflen = fread(gifbuf, sizeof(unsigned char), SIZE, fp);
			// sleep(2);
			fclose(fp);
			while (1) {
				if ((double)(clock() - start2) / CLOCKS_PER_SEC >= 2) {
					send(newfd, gifbuf, giflen, 0);
					break;
				}
			}




/* データフレームの最初. ??はペイロードの長さ */
/*00 00??
0000 0000 0001*/

/* 最後のところ */
/* 0000 0807 0000 0000 0000 0000 0100 0000
04*/









		} else { /* HTTP/1の通信 */
			printf("%s\n", buf);
			/* request headerの終わりまで読み飛ばし */
			/* bodyは無視	 */
			do {
				if (strstr(buf, "\r\n\r\n")) {
					break;
				}
				if (strlen(buf) >= sizeof(buf)) {
					memset(&buf, 0, sizeof(buf));
				}
			} while (recv(newfd, buf+strlen(buf), sizeof(buf) - strlen(buf), 0) > 0);



			/* ファイルパス生成（脆弱性有り）	 */
			sprintf(file, path, 512);
			strcat(file, url);

			/* index.html補完 */
			if (file[strlen(file)-1] == '/') {
				strcat(file, "index.html" );
			}

			/* debug */
			#ifdef DEBUG
				sleep(1);
			#endif

			/* body送信
			 * ファイルが開けなかった場合は404のヘッダと404.htmlを送る
			 * 無事ファイルが開けた場合は200のヘッダと要求されたファイルを送る
			 */
			// char *header;
			if ((filefd = open(file, O_RDONLY)) < 0 || strstr(file, "..") != NULL) {
				perror("open");
				fprintf(stderr, "file: %s\n", file);
				sprintf(file, "%s/404.html", path); /* [カレントディレクトリ]/404.html */
				filefd = open(file, O_RDONLY);
				header = "HTTP/1.1 404 Not Found\n"
							"Content-type: text/html\n"
							"\n";
			} else {
				header = "HTTP/1.0 200 OK\n"
							"Content-type: text/html\n"
							"\n";
			}

			send(newfd, header, strlen(header), 0);
			while ((len = read(filefd, buf, sizeof(buf))) > 0) {
				if (send(newfd, buf, len, 0) < 0) {
					perror("send2");
				}
			}

		} /* else end */


		close(newfd);
	} /* while (1) end */


} /* main end */
