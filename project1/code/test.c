#include<stdio.h>
#include<syscall.h>
#include<sys/types.h>
#include<unistd.h>


void show_process_mem_info(int result[])
{
	int i;
	/*
		[0] result count
		[1] pid
		[2] map count
		[3] total virtual memory 
		[4] total physical memory
		[5] vm start
		[6] vm end
		[7] pm start
		[8] pm end
	*/
	printf("[+] PID = %d \n",result[1]);
	printf("[+] map count = %d \n",result[2]);
	printf("[+] total virtual mem  = %d \n",result[3]);
	printf("[+] total physical mem = %d \n",result[4]);
	for(i=0;i<result[0];i++){
		printf("[%2d] v_start : 0x%x \t, v_end : 0x%x \n",i+1,result[5+i*4],result[6+i*4]);
		printf("[%2d] p_start : 0x%x \t, p_end : 0x%x \n",i+1,result[7+i*4],result[8+i*4]);
	}
}



int main()
{
	int result[10000];
	int a = syscall(351,result);
	show_process_mem_info(result);

	while(1);
	return 0;
}
