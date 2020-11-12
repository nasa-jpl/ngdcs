#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>

/* code is based on 
   http://www.fpga-faq.com/FAQ_Pages/0026_Tell_me_about_bit_files.htm */


void print_fpga_version(char *filename);
void print_fpga_date(char *filename);
int read_length(FILE *fp);
int read_and_check_length(FILE *fp, int expectation);
int skip(FILE *fp, int bytes);
int read_and_check_key(FILE *fp, int expectation);
int read_string(FILE *fp, char *buf, int length);

#define INSTALLDIR	"/usr/local/lib/"
#define BITDIR		"xrm-clink-mfb-adb3/"
#define BITFILE		"xrm-clink-mfb-adb3-db-adpexrc6t-6vlx240t.bit"


main(int argc, char *argv[])
{
    char buf[10];
    int c;

	/* check invocation */

    printf("Current FPGA bitfile is version ");
    print_fpga_version(INSTALLDIR BITDIR BITFILE);
    printf(" (");
    print_fpga_date(INSTALLDIR BITDIR BITFILE);
    printf(").\n");
    printf("Suggested bitfile is version ");
    print_fpga_version(BITDIR BITFILE);
    printf(" (");
    print_fpga_date(BITDIR BITFILE);
    printf(").\n");
    for (;;) {
	printf("Replace current bitfile with suggested bitfile (y/n)? ");
	if (fgets(buf, sizeof(buf), stdin) == NULL ||
		strcasecmp(buf, "n\n") == 0 || strcasecmp(buf, "no\n") == 0) {
	    printf("Leaving installed bitfile as is.\n");
	    exit(1);
	}
	else if (strcasecmp(buf, "y\n") == 0 || strcasecmp(buf, "yes\n") == 0) {
	    printf("Replacing installed bitfile.\n");
	    exit(0);
	}
	else if (strchr(buf, '\n') == NULL) {
	    while ((c=getchar()) != '\n' && c != EOF) ;
	}
    }
}

void print_fpga_version(char *filename)
{
    struct stat statbuf;
    FILE *fp;
    int size, version;
    char buf[132];

    if (stat(filename, &statbuf) == -1) {
	printf("?");
	return;
    }
    if ((fp=fopen("fpgaversions", "r")) == NULL) {
	printf("?");
	return;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if (sscanf(buf, "%d %x", &size, &version) == 2 &&
		size == statbuf.st_size) {
	    printf("hx%x", version);
	    fclose(fp);
	    return;
	}
    }
    fclose(fp);
    printf("?");
}

void print_fpga_date(char *filename)
{
    int c, len;
    char outfile[MAXPATHLEN];
    struct stat statbuf;
    FILE *fp_in, *fp_out;
    char buf[512];

	/* open the input file */

    fp_in = fopen(filename, "rb");
    if (fp_in == NULL) {
	printf("?");
	return;
    }

	/* skip header */

    if ((len=read_and_check_length(fp_in, 9)) == -1) return;
    if (skip(fp_in, len) == -1) return;

	/* look for 'a' key */

    if ((len=read_and_check_length(fp_in, 1)) == -1) return;
    if (read_and_check_key(fp_in, 'a') == -1) return;

	/* look for design name */

    if ((len=read_length(fp_in)) == -1) return;
    if (skip(fp_in, len) == -1) return;

	/* look for part name */

    if (read_and_check_key(fp_in, 'b') == -1) return;
    if ((len=read_length(fp_in)) == -1) return;
    if (skip(fp_in, len) == -1) return;

	/* look for date */

    if (read_and_check_key(fp_in, 'c') == -1) return;
    if ((len=read_and_check_length(fp_in, 11)) == -1) return;
    if (read_string(fp_in, buf, len) == -1) return;
    strcat(buf, " ");

	/* look for time */

    if (read_and_check_key(fp_in, 'd') == -1) return;
    if ((len=read_and_check_length(fp_in, 9)) == -1) return;
    if (read_string(fp_in, buf+strlen(buf), len) == -1) return;
    printf("%s", buf);

	/* look for bit stream */

    if (read_and_check_key(fp_in, 'e') == -1) return;
	
	/* we're done! */

    fclose(fp_in);
}


int read_length(FILE *fp)
{
    int upper, lower, length;

    upper = fgetc(fp);
    if (upper == EOF) {
	printf("?");
	return -1;
    }
    lower = fgetc(fp);
    if (lower == EOF) {
	printf("?");
	return -1;
    }
    upper &= 0xff;
    lower &= 0xff;
    length = ((upper) << 8) | lower;
    return length;
}


int read_and_check_length(FILE *fp, int expectation)
{
    int length;

    length = read_length(fp);
    if (length == -1) return -1;
    else if (length != expectation) {
	printf("?");
	return -1;
    }
    else return length;
}


int skip(FILE *fp, int bytes)
{
    if (fseek(fp, bytes, 1) == -1) {
	printf("?");
	return -1;
    }
    else return 0;
}


int read_and_check_key(FILE *fp, int expectation)
{
    int c;

    c = fgetc(fp);
    if (c == EOF) {
	printf("?");
	return -1;
    }
    if (c != expectation) {
	printf("?");
	return -1;
    }
    return 0;
}


int read_string(FILE *fp, char *buf, int length)
{
    if (fread(buf, length, 1, fp) != 1) {
	printf("?");
	return -1;
    }
    return 0;
}
