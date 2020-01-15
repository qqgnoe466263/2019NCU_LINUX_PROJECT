# linux_survey_TT

## 開發環境和版本資訊
+ Virtual Machine: Virtual Box
+ Linux Release: Ubuntu 16.04 (32-bit)
+ Kernel Version: Linux-3.9.1 (32-bit no PAE)

## 加入 system call
```shell=
sudo apt-get install libncurses5-dev
cd ~/linux-3.9.1
mkdir hello
cd hello
vim hello.c並寫入自訂的system call function
vim Makefile 並寫入 obj-y := hello.o
cd ~/linux-3.9.1/arch/x86/syscalls
vim syscall_32.tbl 裡加入在最後一行 351 i386 linux_survey_TT sys_linux_survey_TT
cd ~/linux-3.9.1/include/linux
vim syscalls.h 裡加入在最後一行 asmlinkage int sys_linux_survey_TT(int result[]);
重啟動電腦後選擇 3.9.1 kernel version
```

## 編譯linux kernel步驟
```shell=
cd ~/linux-3.9.1
sudo make menuconfig
sudo make -j8
sudo make modules_install
sudo make install
sudo update-grub
```

## 測試結果

### 第一題、第二題、第三題

### result_1.txt

[result1.txt](https://drive.google.com/open?id=1Jv6q74uzN1wNzZydBggwdqwJnJeG9zla)

### result_2.txt
[result2.txt](https://drive.google.com/open?id=1FzKNm1Tj90Y1k5_XPoka_822JNTmFFbB)

### result_3.txt
[result3.txt](https://drive.google.com/open?id=17REvQHIV36HuD85j0bALs0mNMgjVSUDM)

### 第四題、第五題
```
[+] PID = 2723 (parent)
[+] map count = 14 
[+] total virtual mem  = 2060 
[+] total physical mem = 276 

[+] PID = 2724 (child)
[+] map count = 14 
[+] total virtual mem  = 2060 
[+] total physical mem = 60 


[+] PID = 2724 (child-COW)
[+] map count = 15 
[+] total virtual mem  = 2192 
[+] total physical mem = 296 

由結果分析 :

    The percentages of the virtual addresses that have physical memory assigned to the program at location 1 : 13%
    The percentages of the virtual addresses that have physical memory assigned to the program at location 2 : 3%
    The percentages of the virtual addresses that have physical memory assigned to the program at location 3 : 13%

    location1跟location2的shared memory大約為74%。
    location1跟location3的shared memory大約為40%。
```

## Problem : Copy-On-Write

### 實驗結果

本來預期在child process修改result1那邊發生COW，但由實驗結果我們會發現發生COW的位置跟我們預期的不一樣，在parent process就發生了，所以result2及result3的結果相同，如下圖。

![](https://i.imgur.com/LSmQ2zj.png)

### 原因

我們知道local變數的位置是放在stack上的，而main又接近stack的底部，所以在這支程式被載入到memory的時候由於程式一開始會將程式所需的資訊放入stack中，例如環境變數，所以當我們將result_1定義在比較靠近stack底部的時候，result_1因為PAGE是4k對齊的關係，有一些部分被包含到stack前段的PAGE中，所以我們在parent那邊寫入result1時就發生COW。

```cpp=
int main()
{
	int result_1[MEMORY_SIZE]; //這裡已經跟stack底部的一些初始化東西的分到一個PAGE
	int result_2[MEMORY_SIZE];
	int result_3[MEMORY_SIZE];

	if(fork()){ // fork後child跟parent的PAGE還是一樣的
		a = syscall(351,result_1); //parent會因為要寫入result_1發生COW
		write2file(result_1,"result_1.txt");
}
```

### 正確的COW

若將程式改成如下，則可觀察到不一樣的結果。
```cpp=
int main()
{
	int result_3[MEMORY_SIZE]; 
	int result_2[MEMORY_SIZE];
	int result_1[MEMORY_SIZE];
```

![](https://i.imgur.com/10ShTGG.png)


## 程式碼
+ kernel function code
```cpp=
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<linux/mm.h>
#include<linux/mm_types.h>
#include<asm/io.h>
#include<linux/highmem.h>

int result_count;
int vma_total;
unsigned long total_virtual_memory;

void write2result(int result[],unsigned long addr)
{
	/*
	 * index  0         1   2         3        4        5        6      7         8
	 * result vma_total pid map_count total_vm total_pm vm_start vm_end phy_start phy_end .....
	 */
	result[result_count] = (int)addr;
	result_count++;
}


static unsigned long virual2physical(struct task_struct *task,unsigned long va);
void print_virual_memory(struct task_struct *task,int result[])
{
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long pa = 0;
	int tmp,tmp1 = 0;

	mm = task->mm;

	/* map_count [2] */
	write2result(result,mm->map_count); 
	
	/* total virtual memory [3] */
	write2result(result,(mm->total_vm << (PAGE_SHIFT - 10))); 
	
	/* total physical memory [4] */
	write2result(result,(get_mm_rss(mm) << (PAGE_SHIFT - 10))); 
	
	for(vma = mm->mmap; vma; vma=vma->vm_next){
		tmp = (vma->vm_end - vma->vm_start) / 0x1000 ;
		while(tmp1 < tmp){
			write2result(result,vma->vm_start+tmp1*0x1000);
			write2result(result,vma->vm_start+tmp1*0x1000+0x1000);
			pa = virual2physical(task,vma->vm_start + tmp1*0x1000);
			if (pa != -1){
				write2result(result,pa);
				write2result(result,pa+0x1000);
			} else {
				write2result(result,0);
				write2result(result,0);
			}
			tmp1++;
		}
		tmp1 = 0;
		vma_total += tmp;
		tmp = 0;
	}
}

static unsigned long virual2physical(struct task_struct *task,unsigned long va)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long pa = 0;
	unsigned long page_addr = 0;
	unsigned long page_offset = 0;

	pgd = pgd_offset(task->mm,va);
	if(pgd_none(*pgd)){
		return -1;
	}

	pud = pud_offset(pgd,va);
	if(pud_none(*pud)){
		return -1;
	}

	pmd = pmd_offset(pud,va);
	if(pmd_none(*pmd)){
		return -1;
	}

	pte = pte_offset_kernel(pmd,va);
	if(pte_none(*pte)){
		return -1;
	}

	page_addr = pte_val(*pte) & PAGE_MASK;
	page_offset = va & ~PAGE_MASK;
	pa = page_addr | page_offset;

	return pa;
}


asmlinkage void sys_linux_survey_TT(int result[])
{
	struct task_struct *task;
	
	result_count = 1;
	vma_total = 0;
	task = current;

	/* task pid [1] */
	write2result(result,task->pid);

	print_virual_memory(task,result);
	result[0] = vma_total;

}
```
+ user program
```cpp=
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
		write2file(result_1,"result_1.txt");
		wait(&exit_status);
	} else {
		a = syscall(351,result_2);
		write2file(result_2,"result_2.txt");
		result_1[0]=123;
		a = syscall(351,result_3);
		write2file(result_3,"result_3.txt");
	}
}
```
+ analysis.py (第五題)
``` cpp=
#!/usr/bin/env python3


f  = open('result_1.txt')
f1 = open('result_2.txt')
f2 = open('result_3.txt')

tp1 = 0
tp2 = 0
tp3 = 0

tpbuf1 = []
tpbuf2 = []
tpbuf3 = []

def analysis(f):
	tp = 0
	tpbuf = []
	lines = f.readline()
	while lines:
		if lines[6] == 'p':
			if "nil" not in lines[21:26]:
				tp = tp + 1
				tpbuf.append(lines[16:26])
		lines = f.readline()

	f.close()
	return tp ,tpbuf



tp1 ,tpbuf1 = analysis(f)
tp2 ,tpbuf2 = analysis(f1)
tp3 ,tpbuf3 = analysis(f2)

num  = 0
num1 = 0

for i in range(1,tp1):
	for j in range(1,tp2):
		if tpbuf1[i] == tpbuf2[j]:
			num = num + 1

for i in range(1,tp1):
	for j in range(1,tp3):
		if tpbuf1[i] == tpbuf3[j]:
			num1 = num1 + 1


print('percent: {:.0%}'.format(num/tp2))
print('percent: {:.0%}'.format(num1/tp3))

```
