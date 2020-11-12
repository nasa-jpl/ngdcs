#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "../framebuf.h"


int main(int argc, char** argv)
{
    unsigned char *data;
    unsigned short first, second;
    int count;

    AlphaDataFrameBuffer *adfb = new AlphaDataFrameBuffer();
    if (adfb->failed)
	exit(1);

    count = 0;
    for (;;) {
	if (adfb->frameIsAvailable()) {
	    count++;
	    data = (unsigned char *)adfb->getFrame();
	    if (data == NULL) {
		printf("No data!\n");
		break;
	    }
	    data += 640 * 2;
	    first = (*(data+1) << 6) | (*(data+0) >> 2);
	    second = (*(data+3) << 6) | (*(data+2) >> 2);
	    printf("%04d: %03d %03d\n", count, first, second);
	    Sleep(1);
	}
    }

    delete adfb;
    exit(0);
}
