#include <stdio.h>
#include <stdbool.h>

#define MAX_INPUT_LEN (128)

int main(void) {
	FILE *fp;
	int now = 0;
	char ch, input[MAX_INPUT_LEN+1];

	if (!(fp = fopen("/proc/mtest", "w"))) {
		printf("Error! Can't open file: /proc/mtest!\n");
		return -1;
	}

	printf("Addr of now:   %p\n", &now);
	printf("Addr of ch:    %p\n", &ch);
	printf("Addr of input: %p\n", input);

	input[0] = '\0';
	while (true) {
		if (ch = getchar()) {
			// Write to /proc/mtest.
			if (ch == '\n') {
				input[now] = '\0';
				fprintf(fp, "%s", input);
				fflush(fp);
				now = 0, input[0] = '\0';
			}
			// Keep getting user input.
			else {
				input[now++] = ch;
			}
		}
		else break;
	}

	fclose(fp);
}
