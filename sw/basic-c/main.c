#include <stdlib.h>
#include <stdio.h>

int sum(int end);

int x = 5;
int y;

int main() {
	int a = 6;
	int b = 7;
	a = a + b + x + y;
	a = sum(a);

puts("bla-bla-bla");

	return a;
}
