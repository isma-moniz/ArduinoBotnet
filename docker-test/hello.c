# include <stdio.h>

int main(void) {
#if defined(__x86_64__)
	printf("Hello from __x86_64__!\n");
#elif defined(__aarch64__)
	printf("Hello from ARM64 (__aarch64__)!\n");
#else
	printf("Hello from unknown architecture!\n");
#endif
}
