#include <syscalls.h>
#include <i386.h>

int main () {

	unsigned int pid = fork();

	breakpoint();

	printf("pid = %d\n", pid);	

	while (1) ;

}
