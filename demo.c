#include <stdio.h>
#include <unistd.h>
int main(int argc, char const *argv[])
{
    int i = 0;
    unsigned long Gn=0,ret=0,ap=0x11111111,pac=0;
    for (i = 0; i < 1000; ++i)
    {
	asm(
	"mov x8,#430;"//syscall number
	"ldr x0,=0x1234;"//x0:fid
	"svc #0;"
	"mov %[ret],x0;"
	"mov %[pac],%[ap];"
	"paciza %[pac];"
	:[ret]"+r"(ret),[pac]"+r"(pac)
	:[ap]"r"(ap)
	:"x0","x1","x8"
	);
        sleep(1);
	Gn=ret;
        printf("i=%d Gn=0x%llx pac=0x%llx\n", i,Gn,pac);
	asm(
	"mov x8,#431;"//syscall number
	"ldr x0,=0x1234;"//x0:fid
	"mov x1,%[Gn];"//x1:Gn
	"svc #0;"
	"mov %[ret],x0;"
	"mov %[ap],%[pac];"
	"autiza %[ap];"
	:[ret]"+r"(ret),[ap]"+r"(ap)
	:[Gn]"r"(Gn),[pac]"r"(pac)
	:"x0","x1","x8"
	);
	Gn=ret;
	printf("i=%d Gn=0x%llx ap=0x%llx\n", i,Gn,ap);
    }
    return 0;
}
