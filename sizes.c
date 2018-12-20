#include <stdio.h>

int main()
{
int si, sl, sll, sp;
si = sizeof(int);
sl = sizeof(long);
sll = sizeof(long long);
sp = sizeof(int *);
printf("int: %d, long: %d, long long: %d, ptr: %d\n", si, sl, sll, sp );
fflush(stdout);
return 0;
}

