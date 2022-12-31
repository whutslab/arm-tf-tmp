#include <stdio.h>
#include <unistd.h>
int foo(int a,int b,int c,int d,int e)
{
    sleep(1);
    printf("a=%d b=%d c=%d d=%d e=%d \n", a, b, c, d, e);
    return 0;
}
int main(int argc, char const *argv[])
{
    int i;
    for (i = 0; i < 10; ++i)
    {
	foo(i,2*i,3*i,4*i,5*i);
    }
    return 0;
}
