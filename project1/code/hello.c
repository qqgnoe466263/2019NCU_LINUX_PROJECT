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
	 * index  0            1   2         3        4        5        6      7         8
	 * result result_count pid map_count total_vm total_pm vm_start vm_end phy_start phy_end .....
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
	//pgd = task->mm->pgd;
	if(pgd_none(*pgd)){
		//printk("pgd not mapped\n");
		return -1;
	}

	pud = pud_offset(pgd,va);
	if(pud_none(*pud)){
		//printk("pud not mapped\n");
		return -1;
	}

	pmd = pmd_offset(pud,va);
	if(pmd_none(*pmd)){
		//printk("pmd not mapped\n");
		return -1;
	}

	pte = pte_offset_kernel(pmd,va);
	if(pte_none(*pte)){
		//printk("pte not mapped\n");
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
