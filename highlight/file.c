#include <stdio.h>

int main() {
  FILE *fp;
  char buf[256];

  fp = fopen("html.txt", "r");
  if (fp == NULL) {
    printf("ファイルが開けません.\n");
    return 1;
  }
  while (fgets(buf, 256, fp) != NULL) {
    /* ここではfgets()により１行単位で読み出し */
    printf("%s", buf);
  }
  fclose(fp);

  return 0;
}