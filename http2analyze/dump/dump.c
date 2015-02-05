#include <stdio.h>

int main() {
  FILE *fp;
  int data, i;
  unsigned long addr;
  char buf[100];

  fp = fopen("binary.txt", "rb");
  if (fp == NULL) {
    printf("ファイルが開けません.\n");
    return 1;
  }

  for (addr = 0; ; addr += 16) {
    printf("%08lX  ", addr);

    for (i = 0; i < 16; i++) {
      if ((data = getc(fp)) == EOF) {
        buf[i] = '\0';

        for (;i < 16; i++) printf("   ");
        printf("  %s\n", buf);
        fclose(fp);
        return 1;
      }
      if(data < 0x20 || data >= 0x7F) buf[i] = '.';
      else buf[i] = data;
      printf("%02X ", data);
    }
    buf[i] = '\0';
    printf("  %s\n", buf);
  }

  return 0;
}