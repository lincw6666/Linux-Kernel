#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX_INPUT_LEN (128)

void show_val(const unsigned long long test1, const unsigned long long test2, const unsigned long long test3) {
	printf("test1 = %#llx, ", test1);
	printf("test2 = %#llx, ", test2);
	printf("test3 = %#llx\n", test3);
}

int main(void) {
	FILE *fp;
	int now = 0;
	char ch, input[MAX_INPUT_LEN+1];
	unsigned long long test1 = 0x11, test2 = 0x22, test3 = 0x33;

	if (!(fp = fopen("/proc/mtest", "w"))) {
		printf("Error! Can't open file: /proc/mtest!\n");
		return -1;
	}

	show_val(test1, test2, test3);
	printf("Addr of test1: %p\n", &test1);
	printf("Addr of test2: %p\n", &test2);
	printf("Addr of test3: %p\n", &test3);

	input[0] = '\0';
	while (true) {
		if (ch = getchar()) {
			// Write to /proc/mtest.
			if (ch == '\n') {
				input[now] = '\0';
				fprintf(fp, "%s", input);
				fflush(fp);
				// Show test1-3's values if user enters "showval".
				if (strncmp(input, "showval", 8) == 0)
					show_val(test1, test2, test3);
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
