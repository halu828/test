#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <getopt.h>

int ProxySocket; /* listen中のソケット */

extern void process0(int clientSocket);
extern void process1(int clientSocket, char *ipaddress);

/* 子プロセス終了 */
void CatchChild(int sig) {
	int retval, status;
#if defined(DEBUG)
	printf("子プロセス終了.\n");
#endif
	do {
		retval = waitpid(-1, &status, WNOHANG); 
	} while (retval > 0);
}

/* Ctrl+Cの割り込みで行う処理 */
void SigHandler(int SignalName) {
	fprintf(stderr, "\nプロキシを終了します.\n");
	close(ProxySocket);
	exit(0);
}


int main(int argc, char *argv[]) {
	struct sockaddr_in ProxyAddr;
	struct sockaddr_in BrowserAddr;
	struct hostent *HostInfo;
	int opt;
	int httpNumber;
	int portNumber;
	int bl = 1; /* bind()失敗対策 */
	int clientSocket;
	int browserlength;
	char ipaddress[15];

	if (argc != 4) {
		fprintf(stderr, "usage: ./proxy [-0][-1] [ipaddress] [port]\n");
		exit(1);
	}

	/* オプションの取得*/
	if((opt = getopt(argc,argv,"01")) != -1) {
		switch(opt) {
			case '0':
				printf("Select HTTP/1.0\n");
				httpNumber = 0;
				break;
			case '1':
				printf("Select HTTP/1.1\n");
				httpNumber = 1;
				break;
			default:
				printf("Please Select one of [-0][-1]\n");
				exit(1);
			break;
		}
	}

	strcpy(ipaddress, argv[2]);
	portNumber = atoi(argv[3]);

	printf("Port Number: %d\n", portNumber);
	printf("IP address : %s\n", ipaddress);

	/* SIGINTにSigHandler()を，SIGCHLDにCatchChild()を登録 */
	signal(SIGINT, SigHandler);
	signal(SIGCHLD, CatchChild);

	/* ブラウザとの通信用のソケットを作成 */
	if ((ProxySocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Failed to make a Proxysocket!\n");
		exit(1);
	}

	/* ホスト名を得る */
	/*if ((HostInfo = gethostbyname("localhost")) == NULL) {*/
	if ((HostInfo = gethostbyname(ipaddress)) == NULL) {
		fprintf(stderr, "Failed to find host!\n");
		exit(1);
	}

	ProxyAddr.sin_family = AF_INET;
	ProxyAddr.sin_port = htons(portNumber);
	memcpy((char *)&ProxyAddr.sin_addr, (char *)HostInfo->h_addr_list[0], HostInfo->h_length);

	/* bind()失敗対策 */
	setsockopt(ProxySocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&bl, sizeof(bl));

	/* IPアドレスとポート番号の設定 */
	if (bind(ProxySocket, (struct sockaddr *)&ProxyAddr, sizeof(ProxyAddr)) == -1) {
		fprintf(stderr, "Failed to assign a name to the Proxysocket!\n");
		exit(1);
	}

	/* listen */
	if (listen(ProxySocket, 5) == -1) {
		fprintf(stderr, "Failed to listen ProxySocket!\n");
		exit(1);
	}


	/* ブラウザから要求があったらプロセスを作成するループ */
	while (1) {
		int pid; /* プロセスの作成 */
		struct timeval waitval; /* タイムアウト */
		fd_set rfds;	/* select()に必要 */

		/* 監視対象をセット */
		FD_ZERO(&rfds);
		FD_SET(ProxySocket, &rfds);

		/* 監視のタイムアウトの時間を設定 */
		waitval.tv_sec  = 5;
		waitval.tv_usec = 0;

		if (select(ProxySocket+1, &rfds, NULL, NULL, &waitval) <= 0) {
			continue; /* select()に失敗するとwhile文をやり直す */
		}

		/* clientSocketをプロセスに渡す */
		if (FD_ISSET(ProxySocket, &rfds)) {

			/* ブラウザからの接続要求をaccept() */
			browserlength = sizeof(BrowserAddr);
			if ((clientSocket = accept(ProxySocket, (struct sockaddr *)&BrowserAddr, (socklen_t *)&browserlength)) == -1) {
				fprintf(stderr, "Failed to accept clientSocket!\n");
				exit(1);
			}
			pid = fork(); /* プロセスの作成 */
			if (pid == 0) {
			#if defined(DEBUG)
				printf("子プロセス開始.\n");
			#endif
				/* 子プロセス */
				if(httpNumber == 0) process0(clientSocket);
				if(httpNumber == 1) process1(clientSocket, ipaddress);
			} else if (pid > 0) { /* fork()失敗 */
				close(clientSocket);
			} else { /* アクセプトソケットクローズ */
				close(clientSocket);
			#if defined(DEBUG)
				printf("ブラウザのソケットを閉じました.\n");
			#endif
			}
			/* 親プロセス処理なし */
		}

	}
	return 0;
}
