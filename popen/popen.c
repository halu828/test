#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#define BUF 256

int main (int argc, char *argv[]) {
	FILE *fp;
	char buf[BUF];
	char *cmdline = "/usr/bin/ruby popen.rb";

	/*
	if ((fp = (FILE*)fopen("tmp.txt","w")) == NULL) {
		err(EXIT_FAILURE, "%s", "tmp.txt");
	}
	fprintf(fp, "test1\ntest2\n");
	fclose(fp);
	*/

	if ((fp = popen(cmdline, "r")) == NULL) {
		err(EXIT_FAILURE, "%s", cmdline);
	}
	while (fgets(buf, BUF, fp) != NULL) {
		printf("%s", buf);
	}
	pclose(fp);

	exit (EXIT_SUCCESS);
}
