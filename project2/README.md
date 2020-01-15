## get_process_zero_session_group && get_process_session_group

## Result

### get_process_zero_session_group
* 左邊是syscall輸出的結果，右邊是用`ps -afej`來驗證結果
* [result.txt](https://drive.google.com/file/d/1YDtbSBjT5bW3-obmdMOdNefxaVcqpfJm/view?usp=sharing)
![sys_get_process_zero_session_group](https://i.imgur.com/cnFalx2.jpg)

### get_process_session_group
* 在執行get_process_session_group這個syscall的那支process跟他相同login session的process只有我的shell(zsh)，又兩個沒有在第二層之後的namespace，所以local pid = global pid
* 圖左上是user program執行結果
* 圖左下是用demsg直接看由printk輸出的資料
* 圖右邊是`ps -afej`來驗證結果
* [result2.txt](https://drive.google.com/file/d/16N2DylFHt5vIeHoTP7VvlQgPDduAReot/view?usp=sharing)
![sys_get_process_session_group](https://i.imgur.com/3y8WKVk.jpg)


### get_process_session_group (a namespace test)
* 由於單純使用`get_process_session_group`看不出namespace結果，所以我自己寫了一個簡單驗證local pid的program驗證`get_process_session_group`是否正確，這支program會建一個namespace然後這個namespace底下會有四個process
* 圖左上是user program執行結果，建了一個pid namespace，有四個process在這個namespace中
* 圖左下是用`ps -afej`來驗證是否在同個login session底下
* 圖右邊是用printk直接看syscall輸出的資料
![sys_get_process_session_group](https://i.imgur.com/mcu6Z6E.jpg)

## kernel function code
+ syscall : sys_get_process_zero_session_group
```cpp=
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<asm/io.h>
#include<linux/init_task.h>

asmlinkage int sys_get_process_zero_session_group(unsigned int *result,int size)
{
	struct pid_link *first;
	struct pid_link *lnext;
	
	int *ptr2pid = 0;
	int tmp = 0;
	int n = 0;

	printk("[*] init_task PID : %d\n",init_task.pid);

	first = (struct pid_link *)(init_task.pids[0].pid->tasks[2].first);
	tmp = first;
	tmp = tmp - 80;
	ptr2pid = tmp;
	printk("[+] PID : %d \n",*ptr2pid);
	//result[n] = ptr2pid;
	//n++;

	lnext = first->node.next;
	while(lnext)
	{
		tmp = lnext;
		tmp = tmp - 80;
		ptr2pid = tmp;
		printk("[+] PID : %d \n",*ptr2pid);
		
		if(n < size){
			result[n] = *ptr2pid;
			n++;
		}

		lnext = lnext->node.next;
	}

	return n;

}

```
+ syscall : sys_get_process_session_group

```cpp=
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<asm/io.h>
#include<linux/init_task.h>

asmlinkage int sys_get_process_session_group(unsigned int *result,int size)
{   

	struct pid_link *first;
	struct pid_link *lnext;

	struct pid *find_pid;
	struct task_struct *find_task;
	pid_t find_pid_t = 0;
	struct pid_namespace *ns;

	int *ptr2pid = 0;
	int tmp = 0;
	int n = 0;

	printk("[*] current PID : %d\n",current->pid);
	find_task = pid_task(find_vpid(current->pid),0);
	find_pid = task_pid(find_task);
	find_pid_t = pid_nr_ns(find_pid,task_active_pid_ns(find_task));
	printk("[*] current local PID : %d\n",find_pid_t);

	first = (struct pid_link *)(current->pids[2].pid->tasks[2].first);
	
	tmp = first;
	tmp = tmp - 80;
	ptr2pid = tmp;
	printk("[+] PID : %d \n",*ptr2pid);
	find_task = pid_task(find_vpid(*ptr2pid),0);
	find_pid = task_pid(find_task);
	find_pid_t = pid_nr_ns(find_pid,task_active_pid_ns(find_task));
	printk("[+] local PID : %d\n",find_pid_t);

	lnext = first->node.next;
	while(lnext)
	{
		tmp = lnext;
		tmp = tmp - 80;
		ptr2pid = tmp;
		printk("[+] PID : %d \n",*ptr2pid);
		find_task = pid_task(find_vpid(*ptr2pid),0);
		find_pid = task_pid(find_task);
		find_pid_t = pid_nr_ns(find_pid,task_active_pid_ns(find_task));
		printk("[+] local PID : %d\n",find_pid_t);

		lnext = lnext->node.next;
	}

	return 0;

}


```

## Test Program
+ sys_get_process_zero_session_group test program
```cpp=
#include<stdio.h>
#include<syscall.h>
#include<sys/types.h>
#include<unistd.h>

#define SIZE 100

int main()
{
	unsigned int results[SIZE];
	int j,k;

	k = syscall(351,results, SIZE);
	
	if(k){
		printf("What follows are the PIDs of the processes that are in the same login sesson of process 0\n");
		for(j=0;j<k && j<SIZE;j++) {
			printf("[%d] %u \n",j,results[j]);
		}
	} else {
		printf("There is an error when executing this system call.\n");
	}
	
	return 0;
}
```
+ sys_get_process_session_group test program
```cpp=
#include<stdio.h>
#include<syscall.h>
#include<sys/types.h>
#include<unistd.h>

#define SIZE 100

int main()
{
	unsigned int results[SIZE];
	int j,k;

	k = syscall(352,results, SIZE);
	
	if(k){
		printf("What follows are the local PIDs of the processes that are in the same login sesson of this process.\n");
		for(j=0;j<k && j<SIZE;j++) {
			printf("[%d] %u \n",j,results[j]);
		}
	} else {
		printf("There is an error when executing this system call.\n");
	}
	
	return 0;
}
```

+ PID namespace test
```cpp=
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

#define SIZE 100

char child_stack[5000];

int child(void* arg)
{
    int pid;
    pid = fork();
    if(fork()){
        printf("clone parent PID : %d\n", getpid());
    } else {
        printf("clone child PID : %d\n", getpid());
    }

    while(1);

    return 1;
}

int main()
{
    unsigned int results[SIZE];
    int k;
    int pid;
    printf("Main Parent PID : %d\n",getpid());
    pid = clone(child, child_stack+5000, CLONE_NEWPID, NULL);
    if (pid == -1) {
        perror("clone:");
        exit(1);
    }
    waitpid(pid, NULL, 0);

    printf("Main child PID : %d\n", pid);

    sleep(1);
    k = syscall(352,results, SIZE); // sys_get_process_session_group

    while(1);

    return 0;
}

```
