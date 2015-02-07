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

#define SIZE 32768

int strrep(char *buf, char *front, char *behind);
int decode_data(char *s, int len);


void process1(int clientSocket, char *ipaddress) {
	struct sockaddr_in serverHost;
	struct hostent *ServerInfo;
	int serverSocket = 0;
	int maxSock;
	int recvSize;					/* 受信したサイズを入れておく変数 */
	char buf[SIZE] = "";	/* リクエストやレスポンスを入れておく変数 */
	char hostName[128];		/* ホスト名 */
	char *front;					/* リクエスト編集に使用 */
	char *behind;					/* リクエスト編集に使用 */
	fd_set def;						/* 初期値を保持 */
	fd_set rfds;					/* select()に必要 */
	FILE *fp;
	// char *tp;							/* htmlコード保存に使用 */
	// int flag = 0;					/* htmlコード保存に使用 */
	char tmp[SIZE] = "";
	char method[4] = "";
	char url[1024] = "";
	char protocol[16] = "";


	/* ブラウザからのリクエストを受信 */
	if ((recvSize = recv(clientSocket, buf, sizeof(buf), 0)) == -1) {
	#if defined(DEBUG)
		fprintf(stderr, "ブラウザからの受信に失敗．プロセス終了．\n");
	#endif
		close(clientSocket);
		close(serverSocket);
		exit(EXIT_SUCCESS);
	}

	/* bufからメソッドとURLとプロトコルを取得 */
	sscanf(buf, "%s %s %s", method, url, protocol);

	/* ホスト名の抽出とリクエストの編集 */
	front = strtok(buf, " ");
	strtok(NULL, "/");
	behind = strtok(NULL, "/");
	strcpy(hostName, behind);
#if defined(DEBUG)
	printf("ホスト名: %s\n", hostName);
#endif
	behind += strlen(behind) + 1;
	sprintf(buf, "%s /%s",front, behind);
	strrep(buf, "Proxy-", ""); /* "Proxy-Connection"を"Connection"に変換 */

	/* チャンクモードでレスポンスが返ってこないように */
	strrep(buf, " HTTP/1.1\r\n", " HTTP/1.0\r\n");
	strrep(buf, "Connection: keep-alive\r\n", "");

	/* Googleにアクセスしようとしていたらヘッダを作り直す */
	if (strstr(url, "www.google.co.jp") != NULL) {
		// strrep(buf, " HTTP/1.1\r\n", " HTTP/1.0\r\n");
		/* Chrome専用になってる */
		if (strstr(url, "?%23q=") != NULL) {
			strrep(url, "http://www.google.co.jp/?%23q=", "");
			sprintf(buf, "GET ?gws_rd=ssl#newwindow=1&safe=off&q=%s HTTP/1.0\r\nHost: www.google.co.jp\r\n\r\n", url);
			/* 検索ワードをword2vecにかける */
			decode_data(url, strlen(url));
			// printf("\n\n\n\n\n%s\n\n\n\n\n\n", url);
			if ((fp = fopen("output.txt", "w")) == NULL) {
				fprintf(stderr, "ファイルのオープンに失敗しました.\n");
				exit(EXIT_FAILURE);
			}
			fprintf(fp, "%s\n", url);
			fclose(fp);
			/* バックグラウンドでword2vecを起動 */
			if ((fp = popen("./mydistance jawikisep.bin &", "r")) == NULL) {
				err(EXIT_FAILURE, "%s", "./mydistance jawikisep.bin &");
			}
			pclose(fp);
		}
	}



#if defined(DEBUG)
	printf("--------[ブラウザからのリクエスト]--------\n%s",buf);
#endif


	/* hostNameからwebサーバのIPアドレスを求める */
	if ((ServerInfo = gethostbyname(hostName)) == NULL) {
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


	/* nghttp2にアクセスしようとしていたら，ヘッダを作り直す */
	if (strstr(url, "nghttp2.org") != NULL) {
		char upgradeHeader[] = "GET / HTTP/1.1\r\n"
													"Host: nghttp2.org\r\n"
													"Connection: Upgrade, HTTP2-Settings\r\n"
													"Upgrade: h2c-14\r\n"
													"HTTP2-Settings: AAMAAABkAAQAAP__\r\n"
													"Accept: */*\r\n"
													"User-Agent: myclient\r\n\r\n";
		if (send(serverSocket, upgradeHeader, sizeof(upgradeHeader), 0) == -1) {
			fprintf(stderr, "Failed to send!\n");
			exit(1);
		}
	} else {

		/* サーバへリクエストを転送 */
		if (send(serverSocket, buf, recvSize, 0) == -1) {
			fprintf(stderr, "Failed to send!\n");
			exit(1);
		}

	}


	/* 監視対象をセット */
	FD_ZERO(&def);
	FD_SET(clientSocket, &def);
	FD_SET(serverSocket, &def);

	if (clientSocket > serverSocket) maxSock = clientSocket;
	else maxSock = serverSocket;


	/* 以下, クライアントかサーバから待ち受け */
	while(1) {
		/* bufの中身を初期化 */
		memset(buf, '\0', sizeof(buf));

		/* 初期値をコピーする */
		memcpy(&rfds, &def, sizeof(fd_set));

		/* clientSocketまたはserverSocketが読み取り可能になるまで待機 */
		if (select(maxSock+1, &rfds, NULL, NULL, NULL) <= 0) {
			continue; /* select()に失敗するとwhile文をやり直す */
		}

		/* ブラウザから受信できるなら */
		if (FD_ISSET(clientSocket, &rfds)) {
		#if defined(DEBUG)
			printf("ブラウザから受信.\n");
		#endif
			if ((recvSize = recv(clientSocket, buf, sizeof(buf), 0)) <= 0) {
			#if defined(DEBUG)
				fprintf(stderr, "ブラウザからのrecv()の返り値が0以下. プロセス終了.\n");
			#endif
				break; /* 受信に失敗したらbreak */
			}

			/* ホスト名の抽出とリクエストの編集 */
			if (strncmp(buf, "GET", 3) == 0 || strncmp(buf, "POST", 4) == 0) {
				front = strtok(buf, " ");
				strtok(NULL, "/");
				behind = strtok(NULL, "/");
				strcpy(hostName, behind);
			#if defined(DEBUG)
				printf("ホスト名: %s\n", hostName);
			#endif
				behind += strlen(behind) + 1;
				sprintf(buf, "%s /%s",front, behind);
				strrep(buf, "Proxy-", ""); /* "Proxy-Connection"を"Connection"に変換 */
			}

			/* チャンクモードでレスポンスが返ってこないように */
			strrep(buf, " HTTP/1.1\r\n", " HTTP/1.0\r\n");
			strrep(buf, "Connection: keep-alive\r\n", "");

			/* サーバへリクエストを転送 */
			if (send(serverSocket, buf, recvSize, 0) == -1) {
				fprintf(stderr, "Failed to send!\n");
				exit(1);
			}

		#if defined(DEBUG)
			printf("--------[リクエスト]--------\n%s\n", buf);
		#endif

		} /* if (FD_ISSET(clientSocket, &rfds)) end */

		/* サーバから受信できるなら */
		if (FD_ISSET(serverSocket, &rfds)) {
		#if defined(DEBUG)
			printf("サーバから受信.\n");
		#endif
			if ((recvSize = recv(serverSocket, buf, sizeof(buf), 0)) <= 0) {
			#if defined(DEBUG)
				fprintf(stderr, "サーバからのrecv()の返り値が0以下. プロセス終了.\n");
			#endif
				break; /* 受信に失敗したらbreak */
			}

			/* esegoogleのための処理. レスポンスにhttpsがついてたらsをはずす. */
			if (strstr(buf, "Location: https://www.google.co.jp/") != NULL)
				strrep(buf, "Location: https", "Location: http");

			if (strstr(buf, "Upgrade: h2c-14") != NULL) {
				// strstr(buf, "\r\n\r\n")+1
				// printf("Upgrade: h2c-14\n");
				/* デコードして, htmlのコンテンツの部分だけを持ってくる. */
			}

			/* Googleの検索結果にHTTP/2対応サーバへのリンクを表示 */
			// if (strstr(buf, "Host: www.google.co.jp") != NULL && strcasestr(buf, "<body") != NULL) {
			// 	if ((fp = (FILE*)fopen("addhttp2serverlink.txt","w")) == NULL) {
			// 		err(EXIT_FAILURE, "%s", "html.txt");
			// 	}
			// 	fprintf(fp, "%s", buf);
			// 	fclose(fp);
			// 	if ((fp = popen("/usr/bin/ruby addhttp2serverlink.rb", "r")) == NULL) {
			// 		err(EXIT_FAILURE, "%s", "/usr/bin/ruby addhttp2serverlink.rb");
			// 	}
			// 	while (fgets(tmp, SIZE, fp) != NULL) {
			// 		strcat(buf, tmp);
			// 	}
			// 	printf("%s", buf);
			// }


			/* Ruby起動してその出力をもらう */
			// if (strcasestr(buf, "Content-Type: text/html") != NULL) { /* http/2だと処理が違う. コンテンツだけをうまいこと取り出してくる */
			// 	// flag = 1;
			// 	char *cmdline = "/usr/bin/ruby highlight.rb";
			// 	char tmp[SIZE] = "";
			// 	char *header = "HTTP/1.1 200 OK\r\n"
			// 			"Content-type: text/html\r\n\r";
			// 	if ((fp = (FILE*)fopen("html.txt","w")) == NULL) {
			// 			err(EXIT_FAILURE, "%s", "html.txt");
			// 	}
			// 	while (1) {
			// 		if ((tp = strstr(buf, "\r\n\r\n")) != NULL) fprintf(fp, "%s", tp+1);
			// 		else fprintf(fp, "%s", buf);
			// 		if (recvSize <= 0) break;
			// 		recvSize = recv(serverSocket, buf, sizeof(buf), 0);
			// 	}
			// 	fclose(fp);

			// 	memset(buf, '\0', sizeof(buf));
			// 	if ((fp = popen(cmdline, "r")) == NULL) {
			// 		err(EXIT_FAILURE, "%s", cmdline);
			// 	}
			// 	while (fgets(tmp, SIZE, fp) != NULL) {
			// 		strcat(buf, tmp);
			// 	}
			// 	pclose(fp);
			// 	send(clientSocket, header, strlen(header), 0);
			// 	send(clientSocket, buf, recvSize, 0);
			// } else {
			// 	if (send(clientSocket, buf, recvSize, 0) == -1) {
			// 		fprintf(stderr, "Failed to send!\n");
			// 		exit(1);
			// 	}
			// }
			if ((fp = fopen("resultWord.txt", "r")) == NULL) {
				fprintf(stderr, "ファイルのオープンに失敗しました.\n");
				exit(EXIT_FAILURE);
			}
			char resultWord[128];
			fgets(resultWord, 128, fp);
			resultWord[strlen(resultWord) - 1] = '\0';
			if (strcasestr(buf, "Content-Type: text/html") != NULL && strstr(buf, resultWord) != NULL) {/* http/2だと処理が違う. コンテンツだけをうまいこと取り出してくる */
			// 	flag = 1;
			// if (flag == 1) {
				char *cmdline = "/usr/bin/ruby highlight.rb";
				char *header = "HTTP/1.0 200 OK\r\n"
						"Content-type: text/html\r\n\r";
				if ((fp = (FILE*)fopen("html.txt","w")) == NULL) {
					err(EXIT_FAILURE, "%s", "html.txt");
				}
				fprintf(fp, "%s", strstr(buf, "\r\n\r\n")+1);
				while (1) {
					memset(buf, '\0', sizeof(buf));
					recvSize = recv(serverSocket, buf, sizeof(buf), 0);
					fprintf(fp, "%s", buf);
					if (strstr(buf, "</html") != NULL) break;
				}
				fclose(fp);
				memset(buf, '\0', sizeof(buf));
				if ((fp = popen(cmdline, "r")) == NULL) {
					err(EXIT_FAILURE, "%s", cmdline);
				}
				send(clientSocket, header, strlen(header), 0);
				while (fgets(tmp, SIZE, fp) != NULL) {
					if (send(clientSocket, tmp, strlen(tmp), 0) < 0) {
						perror("send2");
					}
				}
				pclose(fp);
				// send(clientSocket, buf, strlen(buf), 0);
				// printf("%s\n", buf);
			} else {
				if (send(clientSocket, buf, recvSize, 0) == -1) {
					fprintf(stderr, "Failed to send!\n");
					exit(1);
				}
				printf("---------[レスポンス]---------\n%s\n", buf);
			#if defined(DEBUG)
				printf("---------[レスポンス]---------\n%s\n", buf);
			#endif
			}
			// if (strcasestr(buf, "</html") != NULL) flag = 0;

			/* ハイライト(body部分だけ) */
			// if (strcasestr(buf, "<body") != NULL) flag = 1;
			// if (flag == 1) {
			// 	strrep(buf, "情報", "<span style=\"background-color: #ffff00\"><b>情報</b></span>");
			// }
			// if (strcasestr(buf, "</body") != NULL) flag = 0;

			// strrep(buf, "Transfer-Encoding: chunked\r\n", "");

			// int count = 0;
			// while (1) {
			// 	if (strrep(buf, "情報", "<span style=\"background-color: #ffff00\"><b>情報</b></span>") == 0) break;
			// 	count++;
			// }

			// if (strrep(buf, "情報", "<span style=\"background-color: #ffff00\"><b>情報</b></span>") == 0)
			// recvSize += 54;
			// strrep(buf, "情報", "<span style=\"background-color: #ffff00\"><b>情報</b></span>");

			// printf("%d\n", recvSize);
		// 	if (strstr(buf, "情報") != NULL) {
		// 	if (strrep(buf, "情報", "<span style=\"background-color: #ffff00\"><b>情報</b></span>") == 1) {
		// 		printf("置換\n");
		// 		int num_chunked = 0;
		// 		char ch_chunked[8];
		// 		front = strstr(buf, "\r\n\r\n");
		// 		behind = strstr(front, "\r\n");
		// 		// printf("%d", atoi(behind));
		// 		// behind += strlen(behind) + 1;
		// 		// sprintf(buf, "%s%d%s",front, chunked, behind);
		// 		sprintf(ch_chunked, "%d", atoi(behind)+54);
		// 		printf("%s %s\n", behind, ch_chunked);
		// 		strrep(buf, behind, ch_chunked);
		// 		recvSize+=54;
		// 	}
		// }

			// printf("%d %d\n", recvSize, strlen(buf));
			// printf("---------[レスポンス]---------\n%s\n", buf);
		// #if defined(DEBUG)
		// 	printf("---------[レスポンス]---------\n%s\n", buf);
		// #endif
			/* ブラウザへレスポンスを転送 */
			// if (send(clientSocket, buf, recvSize, 0) == -1) {
			// 	fprintf(stderr, "Failed to send!\n");
			// 	exit(1);
			// }
		// #if defined(DEBUG)
		// 	printf("[ブラウザへレスポンスを転送]\n");
		// #endif



			/* htmlコードだけをhtml.txtに保存 */
			// if (strcasestr(buf, "Content-Type: text/html") != NULL &&
			// 	strcasestr(buf, "<body") != NULL) flag = 1;
			// if (flag == 1) {
			// 	if ((fp = fopen("html.txt", "a")) == NULL) {
			// 		err(EXIT_FAILURE, "%s", "html.txt");
			// 	}
			// 	if ((tp = strstr(buf, "\r\n\r\n")) != NULL) fprintf(fp, "%s", tp+1);
			// 	else fprintf(fp, "%s", buf);
			// 	fclose(fp);
			// }
			// if (strcasestr(buf, "</body") != NULL) flag = 0;

		} /* if (FD_ISSET(serverSocket, &rfds)) end */

	} /* while(1) end */

	close(serverSocket);
#if defined(DEBUG)
	printf("サーバソケットを閉じました.\n");
#endif
	exit(EXIT_SUCCESS);
}


/* 置換する. buf の中の front を behind にする. 成功=1 失敗=0
 * keep-aliveのヘッダ編集, パケット改変に使用
 * 置換は1回だけ
 */
int strrep(char *buf, char *front, char *behind) {
	char *chp;
	size_t frontlen, behindlen;

	frontlen = strlen(front);
	behindlen = strlen(behind);
	if (frontlen == 0 || (chp = strcasestr(buf, front)) == NULL) return 0;
	memmove(chp + behindlen, chp + frontlen, strlen(buf) - (chp + frontlen - buf ) + 1);
	memcpy(chp, behind, behindlen);
	return 1;
}

int decode_data(char *s, int len)  { /* len はs が保持するデータのサイズ*/
   int i, j, k;     /* カウンタ類*/
   char buf, *s1;   /* 作業領域*/

   s1 = malloc (len + 1); /* エラー処理は省略*/

   for( i = 0, j = 0; i < len; i++, j++ ){
     if( s[i] == '+' ){
      s1[j] = ' ';
       continue;
     }
     if( s[i] != '%' ){
       s1[j] = s[i];
       continue;
     }
     buf = '\0';
     for( k = 0 ; k < 2 ; k++){
       buf *= 16;
       if( s[++i] >= 'A' )
         buf += (s[i] - 'A' + 10);
       else
         buf += (s[i] - '0');
     }
     s1[j] = buf;
   }

   for( i = 0; i < j; i++ ) /* 書き戻し*/
     s[i] = s1[i];
   s[i] = '\0';

   free( s1 );
   return 0;
}
