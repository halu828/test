#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define SIZE 32768
int dump();

int main() {
	FILE *fp;
	char *bp;
	int len, i = 0, flags, bufindex, framelength, padlength, contentslength;
	unsigned char buf[SIZE] = "";
	unsigned char contents[SIZE] = "";

	fp = fopen("binary.txt", "rb");
	if(fp == NULL){
		printf("読込用ファイルが開けません\n");
		exit(1);
	}
	len = fread(buf, sizeof(unsigned char), SIZE, fp);
	fclose(fp);

	// bp = strstr(buf, "\r\n\r\n")+4;
	while (1) {
		if (buf[i] == 0x0D) i++;
			if (buf[i] == 0x0A) i++;
				if (buf[i] == 0x0D) i++;
					if (buf[i] == 0x0A) {
						i++;
						break;
					}
		i++;
	}
	bufindex = i;
	// printf("%02x\n", buf[bufindex]);

	while (1) {
		/* フレームの長さ */
		framelength = buf[bufindex]*0x10000 + buf[bufindex+1]*0x100 + buf[bufindex+2];
		printf("Frame Length = %02x %02x %02x\n", buf[bufindex], buf[bufindex+1], buf[bufindex+2]);
		printf("Frame Length = %d\n", framelength);
		bufindex += 3;
		printf("Frame Type = %02x\n", buf[bufindex]);
		/* フレームの種類を判断 DATA frames (type=0x0) */
		if (buf[bufindex] == 0x0) {
			// printf("DATA frames\n");
			// printf("index:%02x value:%02x\n", bufindex, buf[bufindex]);
			flags = ++bufindex; /* Flags 1バイト */
			bufindex += 4; /* Stream Identifier  4バイト (32ビット)*/
			bufindex++;
			if (flags == 0x8) padlength = buf[bufindex]; /* DATAフレームでflagsが0x8ならPadが設定されてる */
			else padlength = 0;
			if (flags == 0x1) printf("END_STREAM\n");
			contentslength = framelength - padlength - 1; //- padlength -1; /* 1はPad Length自体のバイト数 */
			bufindex++;
			printf("%d %d\n", framelength, padlength);
			for (i = 0; i < contentslength; i++) {
				contents[i] = buf[bufindex++];
				// printf("%02x ", buf[bufindex]);
			}
			break;
		} else {
			// printf("NOT DATA frames\n");
			bufindex += 5 + framelength; /* 1+4バイト (8+32ビット)*/
			bufindex++;
			// printf("index:%02x value:%02x\n", bufindex, buf[bufindex]);
		}

	}


	/* dump */
	int data, index = 0;
	char temp[SIZE];
	unsigned long addr;

	len = contentslength;

  for (addr = 0; ; addr += 16) {
    printf("%08lX  ", addr);
    for (i = 0; i < 16; i++) {
      data = contents[index++];
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



	// printf("%d\n", framelength);

}

/*
Frame Format
 +-----------------------------------------------+
 |                 Length (24)                   |
 +---------------+---------------+---------------+
 |   Type (8)    |   Flags (8)   |
 +-+-------------+---------------+-------------------------------+
 |R|                 Stream Identifier (31)                      |
 +=+=============================================================+
 |                   Frame Payload (0...)                      ...
 +---------------------------------------------------------------+

Flgas
DATA フレームは以下のフラグを定義します:
END_STREAM (0x1)
ビット0は、エンドポイントが特定のストリームに送信する最後のフレームであることを示します。このフラグを設定することで、ストリームはいずれかの "half closed" 状態または "closed" 状態 (5.1節) に遷移します。
PADDED (0x8)
ビット3は、Pad Length フィールドとパディングが設定されていることを示します。

*/

/*
DATA frame type = 0x0
 +---------------+
 |Pad Length? (8)|
 +---------------+-----------------------------------------------+
 |                            Data (*)                         ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+
*/

int dump(char *buf) {
  // FILE *fpr;
  int len, data, i, index = 0;
  // unsigned char buf[SIZE];
  unsigned char temp[SIZE];
  unsigned long addr;

  // fpr = fopen("binary.txt", "rb");
  // if(fpr == NULL){
  //   printf("読込用ファイルが開けません\n");
  //   exit(1);
  // }
  // len = fread(buf, sizeof(unsigned char), SIZE, fpr);
  // fclose(fpr);
  len = strlen(buf);

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
