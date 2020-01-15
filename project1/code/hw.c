#include<stdio.h>
#include<string.h>
#include<unistd.h>
#define MEMORY_SIZE 10000

void write_str(FILE *fp,char *str,int result,int result2,int result3)
{
	char buf[60];
	memset(buf,0,sizeof(buf));
	sprintf(buf,str,result,result2,result3);
	fwrite(buf,sizeof(char),strlen(buf),fp);
}

void write2file(int result[],char *name)
{
	FILE *fp;
	int i;

	fp = fopen(name,"w");
	write_str(fp,"[+] PID = %d \n",result[1],0,0);
	write_str(fp,"[+] map count = %d \n",result[2],0,0);
	write_str(fp,"[+] total virtual mem  = %d \n",result[3],0,0);
	write_str(fp,"[+] total physical mem = %d \n",result[4],0,0);
	for(i=0;i<result[0];i++){
		write_str(fp,"[%2d] v_start : %10p \t, v_end : %10p \n",i+1,result[5+i*4],result[6+i*4]);
		write_str(fp,"[%2d] p_start : %10p \t, p_end : %10p \n",i+1,result[7+i*4],result[8+i*4]);
	}		

	fclose(fp);
}


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
		printf("[%2d] v_start : %10p \t, v_end : %10p \n",i+1,result[5+i*4],result[6+i*4]);
		printf("[%2d] p_start : %10p \t, p_end : %10p \n",i+1,result[7+i*4],result[8+i*4]);
	}
}

int main()
{
	int result_1[MEMORY_SIZE];
	int result_2[MEMORY_SIZE];
	int result_3[MEMORY_SIZE];
	int exit_status;
	int a;

	if(fork()){
		a = syscall(351,result_1);
		//show_process_mem_info(result_1);
		write2file(result_1,"result_1.txt");
		wait(&exit_status);
	} else {
		a = syscall(351,result_2);
		//show_process_mem_info(result_2);
		write2file(result_2,"result_2.txt");
		result_1[0]=123;
		a = syscall(351,result_3);
		//show_process_mem_info(result_3);
		write2file(result_3,"result_3.txt");
	}
}
