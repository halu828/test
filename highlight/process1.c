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
int dump(unsigned char *buf, int len);
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
	char urlslash[1024] = "";
	char protocol[16] = "";
	unsigned char http2buf[SIZE] = ""; /* http/2通信に使用 */
	int http2flag = 0; /* http/2通信に使用 */



	/* クライアントからのリクエストを受信 */
	if ((recvSize = recv(clientSocket, buf, sizeof(buf), 0)) == -1) {
	#if defined(DEBUG)
		fprintf(stderr, "クライアントからの受信に失敗．プロセス終了．\n");
	#endif
		close(clientSocket);
		close(serverSocket);
		exit(EXIT_SUCCESS);
	}
	printf("[request]\n");

	/* bufからメソッドとURLとプロトコルを取得 */
	sscanf(buf, "%s %s %s", method, url, protocol);

	/* ホスト名の抽出とリクエストの編集 */
	front = strtok(buf, " ");
	strtok(NULL, "/");
	behind = strtok(NULL, "/");
	strcpy(hostName, behind);
#if defined(DEBUG)
	printf("host name: %s\n", hostName);
#endif
	behind += strlen(behind) + 1;
	sprintf(buf, "%s /%s",front, behind);
	strrep(buf, "Proxy-", ""); /* "Proxy-Connection"を"Connection"に変換 */

	/* URLのスラッシュの後ろの部分を取得. http/2通信で使用. */
	strcpy(urlslash, url);
	strrep(urlslash, "http://", "");
	strrep(urlslash, hostName, "");
	// printf("%s\n", urlslash);


	/* チャンクモードでレスポンスが返ってこないように */
	strrep(buf, " HTTP/1.1\r\n", " HTTP/1.0\r\n");
	strrep(buf, "Connection: keep-alive\r\n", "");

	/* Googleにアクセスしようとしていたらヘッダを作り直す */
	if (strstr(url, "www.google.co.jp") != NULL) {
		/* Chrome専用になってる */
		if (strstr(url, "?%23q=") != NULL) {
			strrep(url, "http://www.google.co.jp/?%23q=", "");
			sprintf(buf, "GET ?gws_rd=ssl#safe=off&q=%s HTTP/1.0\r\nHost: www.google.co.jp\r\n\r\n", url);
			/* 検索ワードをword2vecにかける */
			decode_data(url, strlen(url));
			if ((fp = fopen("output.txt", "w")) == NULL) {
				fprintf(stderr, "ファイルのオープンに失敗しました.\n");
				exit(EXIT_FAILURE);
			}
			// fprintf(fp, "%s\n", url);
			fclose(fp);
			/* バックグラウンドでword2vecを起動 */
			if ((fp = popen("./mydistance jawikisep.bin &", "r")) == NULL) {
				err(EXIT_FAILURE, "%s", "./mydistance jawikisep.bin &");
			}
			pclose(fp);
		}
	}



#if defined(DEBUG)
	printf("--------[リクエスト]--------\n%s",buf);
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
	// if (strstr(url, "nghttp2.org") != NULL) {
	if (strstr(url, "localhost/http2") != NULL) {
		http2flag = 1;
		char upgradeHeader[2048] = "";
		// char upgradeHeader[] = "GET / HTTP/1.0\r\n"
		// 											"Host: nghttp2.org\r\n"
		// 											"Connection: Upgrade, HTTP2-Settings\r\n"
		// 											"Upgrade: h2c-14\r\n"
		// 											"HTTP2-Settings: AAMAAABkAAQAAP__\r\n"
		// 											"Accept: */*\r\n"
		// 											"User-Agent: myclient\r\n\r\n";
		// front = strtok(buf, " ");
		// strtok(NULL, "/");
		// behind = strtok(NULL, "/");
		sprintf(upgradeHeader, "GET %s HTTP/1.1\r\n"
													"Host: %s\r\n"
													"Connection: Upgrade, HTTP2-Settings\r\n"
													"Upgrade: h2c-14\r\n"
													"HTTP2-Settings: AAMAAABkAAQAAP__\r\n"
													"Accept: */*\r\n"
													"User-Agent: myclient\r\n\r\n", urlslash, hostName);
		printf("%s", upgradeHeader);
		if (send(serverSocket, upgradeHeader, strlen(upgradeHeader), 0) == -1) {
			fprintf(stderr, "Failed to send!\n");
			exit(1);
		}
		// printf("--------[クライアントからのリクエスト]--------\n%s",upgradeHeader);

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



		/* クライアントから受信できるなら */
		if (FD_ISSET(clientSocket, &rfds)) {
		#if defined(DEBUG)
			printf("クライアントから受信.\n");
		#endif
			if ((recvSize = recv(clientSocket, buf, sizeof(buf), 0)) <= 0) {
			#if defined(DEBUG)
				fprintf(stderr, "クライアントからのrecv()の返り値が0以下. プロセス終了.\n");
			#endif
				break; /* 受信に失敗したらbreak */
			}
			printf("[request]\n");

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
			if (http2flag == 1) { /* http2flagが立っていたら, unsigned char の型で受ける */
				if ((recvSize = recv(serverSocket, http2buf, sizeof(http2buf), 0)) <= 0) {
					break; /* 受信に失敗したらbreak */
				}
				printf("[recv http2responce]\n");
			} else {
				if ((recvSize = recv(serverSocket, buf, sizeof(buf), 0)) <= 0) {
				#if defined(DEBUG)
					fprintf(stderr, "サーバからのrecv()の返り値が0以下. プロセス終了.\n");
				#endif
					break; /* 受信に失敗したらbreak */
				}
				printf("[recv http1responce]\n");
			}


			/* esegoogleのための処理. レスポンスにhttpsがついてたらsをはずす. */
			if (strstr(buf, "Location: https://www.google.co.jp/") != NULL)
				strrep(buf, "Location: https", "Location: http");

			// printf("---HTTP/2---\n%s", http2buf);
			/* http/2サーバから"Upgrade: h2c-14"が返ってきたら, http/2→http/1 */
			if (strstr(http2buf, "Upgrade: h2c-14") != NULL) {
				printf("Upgrade\n");
				// dump(http2buf, recvSize);
				int i = 0, bufindex, framelength, padlength, contentslength, flags;
				unsigned char contents[SIZE] = "";
				// unsigned char pri[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

				/* http2サーバにPRIを送信 */
				// send(serverSocket, pri, sizeof(pri), 0);
				// printf("%s\n", buf);

				/* 一度レスポンスを全て受け取ってファイルに保存する */
				fp = fopen("binary.txt", "wb");
				if (fp == NULL) {
					printf("書込用ファイルが開けません\n");
					exit(1);
				}
				while (1) {
					fwrite(http2buf, sizeof(unsigned char), recvSize, fp);
					memset(http2buf, '\0', sizeof(http2buf));
					recvSize = recv(serverSocket, http2buf, sizeof(http2buf), 0);
					if (recvSize <= 0) break;
				}
				fclose(fp);

				/* 全部受け取ったらGOAWAYフレームを送る */
				/*
				char goaway[] = {"00", "00", "08", "07", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00", "00"};
				if (send(serverSocket, goaway, 17, 0) == -1) {
					fprintf(stderr, "Failed to send!\n");
					exit(1);
				}
				printf("GOAWAY send\n");
				*/

				/* HTTP/1.1でヘッダ送信 */
				char http2header[] = "HTTP/1.1 200 OK\r\n"
														"Content-type: text/html\r\n\r\n";
				if (send(clientSocket, http2header, sizeof(http2header), 0) == -1) {
					fprintf(stderr, "Failed to send!\n");
					exit(1);
				}
				// printf("header送信\n");
				memset(http2buf, '\0', sizeof(http2buf));
				fp = fopen("binary.txt", "rb");
				if(fp == NULL){
					printf("読込用ファイルが開けません\n");
					exit(1);
				}
				fread(http2buf, sizeof(unsigned char), SIZE, fp);
				fclose(fp);

				while (1) { /* "\r\n\r\n"まで飛ばす */
					if (http2buf[i] == 0x0D) i++;
						if (http2buf[i] == 0x0A) i++;
							if (http2buf[i] == 0x0D) i++;
								if (http2buf[i] == 0x0A) {
									i++;
									break;
								}
					i++;
				}
				bufindex = i;
				// printf("Frame Length (16) = %02x %02x %02x\n", http2buf[bufindex], http2buf[bufindex+1], http2buf[bufindex+2]);

				/* HTTP/2レスポンスをデコードしてクライントへコンテンツを送る */
				while (1) {
					/* フレームの長さ */
					framelength = http2buf[bufindex]*0x10000 + http2buf[bufindex+1]*0x100 + http2buf[bufindex+2];
					// printf("Frame Length = %02x %02x %02x\n", buf[bufindex], buf[bufindex+1], buf[bufindex+2]);
					printf("Frame Length (2) = %d\n", framelength);
					bufindex += 3; /* Lengthの長さ分だけ進める */
					// printf("Frame Type = %02x\n", http2buf[bufindex]);
					/* フレームの種類を判断 DATA frames (type=0x0) */
					if (http2buf[bufindex] == 0x00) {
						printf("DATA frames\n");
						printf("index:%x Type:%02x\n", bufindex, http2buf[bufindex]);
						flags = ++bufindex; /* Flags 1バイト */
						bufindex += 4; /* Stream Identifier  4バイト (32ビット)*/
						// bufindex++;
						if (flags == 0x8) padlength = http2buf[bufindex]; /* DATAフレームでflagsが0x8ならPadが設定されてる */
						else padlength = 0;
						if (flags == 0x1) printf("END_STREAM\n"); /* DATAフレームでflagsが0x1ならEND_STREAMが設定されてる */
						contentslength = framelength - padlength - 1; //1はPad Length自体のバイト数 
						bufindex++;
						// printf("%d %d\n", framelength, padlength);
						for (i = 0; i < contentslength; i++) {
							contents[i] = http2buf[bufindex++];
							// printf("%02d %02x ", bufindex, http2buf[bufindex]);
						}
						dump(contents, i);
						// printf("%s\n", contents);
						/* contentsをsend */
						if (send(clientSocket, contents, i, 0) == -1) {
							fprintf(stderr, "Failed to send!\n");
							exit(1);
						}
						bufindex++;
						// break;
					} else if (http2buf[bufindex] == 0x07) {
						printf("recv GOAWAY\n");
						break;
					} else {
						printf("NOT DATA frames\n");
						printf("index:%02x Type:%02x\n", bufindex, http2buf[bufindex]);
						bufindex += 5 + framelength; /* 1+4バイト (8+32ビット)*/
						bufindex++;
					}
					// bufindex = 0;
					memset(contents, '\0', sizeof(contents));
				} /* while (1) end */

				// while (1) {
				// 	memset(http2buf, '\0', sizeof(http2buf));
				// 	recvSize = recv(serverSocket, http2buf, sizeof(http2buf), 0);
				// 	if (recvSize <= 0) {
				// 		http2flag = 0;
				// 		break;
				// 	}
				// 	dump(http2buf, recvSize);
				// 	if (send(clientSocket, http2buf, recvSize, 0) == -1) {
				// 			fprintf(stderr, "Failed to send!\n");
				// 			exit(1);
				// 	}
				// } /* while (1) end */

			} /* if (strstr(http2buf, "Upgrade: h2c-14") != NULL) end */



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


			/* word2vecの結果の文字をハイライト */
			if ((fp = fopen("resultWord.txt", "r")) == NULL) {
				fprintf(stderr, "ファイルのオープンに失敗しました.\n");
				exit(EXIT_FAILURE);
			}
			char resultWord[128];
			fgets(resultWord, 128, fp);
			resultWord[strlen(resultWord) - 1] = '\0';
			if (strcasestr(buf, "Content-Type: text/html") != NULL && strstr(buf, resultWord) != NULL) { /* http/2だと処理が違う. コンテンツだけをうまいこと取り出してくる */
				printf("[highlight]\n");
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

			} else {
				if (send(clientSocket, buf, recvSize, 0) == -1) {
					fprintf(stderr, "Failed to send!\n");
					exit(1);
				}
			#if defined(DEBUG)
				printf("---------[レスポンス]---------\n%s\n", buf);
			#endif
			} /* (strcasestr(buf, "Content-Type: text/html") != NULL && strstr(buf, resultWord) != NULL) end */


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

int dump(unsigned char *buf, int len) {
  // FILE *fpr;
  int data, i, index = 0;
  // unsigned char buf[SIZE];
  unsigned char temp[SIZE];
  unsigned long addr;

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

/* url デコード */
int decode_data(char *s, int len) {
	int i, j, k;
	char buf, *ch;

	ch = malloc (len + 1);

	for(i = 0, j = 0; i < len; i++, j++) {
		if(s[i] == '+') {
			ch[j] = ' ';
			continue;
		}
		if(s[i] != '%'){
			ch[j] = s[i];
			continue;
		}
		buf = '\0';
		for(k = 0 ; k < 2 ; k++){
			buf *= 16;
			if(s[++i] >= 'A' ) buf += (s[i] - 'A' + 10);
			else buf += (s[i] - '0');
		}
		ch[j] = buf;
	}

	for(i = 0; i < j; i++)
		s[i] = ch[i];
	s[i] = '\0';

	free(ch);
	return 0;
}
