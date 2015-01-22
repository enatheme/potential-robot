#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/ptrace.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mm_types.h>
#include <linux/list.h>
#include <linux/sys.h>


void unstacking_eip(void)
{
	//variables
	struct mm_struct *mm;
	struct Alt_stack *remove, *tmp;
	struct pt_regs *my_regs;
	unsigned long eip = 0;
	unsigned char temp[4];

	//allocation
	mm = current->mm;
	my_regs = task_pt_regs(current);

	//we take the value of eip stacked
	//it should be the top of the stack
	access_remote_vm(current->mm, my_regs->sp, temp, 4, 0);
	eip += temp[0];
	eip += temp[1] * 0x100;
	eip += temp[2] * 0x10000;
	eip += temp[3] * 0x1000000;
	
	//we check if eip stacked is the same than the value stored into the linked list
	//we loop the linked list, the last element will be the last stacked !
	list_for_each_entry(remove, &(mm->alt_eip_stack.mylist), mylist)
	{
		//we do nothing, indeed we just want to travel the linked list
		tmp = remove;
	}	
	//here the structure remove got the last element of the linked list
	//and we compare his eip value with the value on the stack
	if((tmp->eip - eip - 0x2) != 0x0)
	{
		//if it's not the same value, we stop the program and delete the entire linked list
		//SIGKILL is sended because the program is consided as corrupted and
		//a SIGQUIT give the control back to the program and it should be avoided
		//so => SIGKILL
		printk("STOP THE SOFT !\n");
		
		//and we clean the alt eip
		list_for_each_entry_safe(remove, tmp, &(mm->alt_eip_stack.mylist), mylist)
		{
//			list_def(remove->mylist);
			kfree(remove);
		}
		kill(current->pid, 9);
	}
	else
	{
		//else, the program is safe, we remove the last element (struct remove)
		//and let the program running
		list_del(&tmp->mylist);
		kfree(tmp);
		printk("START THE SOFT !\n");
	}
}
	
void Pre_Handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
}

void Post_Handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	if(current_uid() != 0)
	{
		unstacking_eip();
	}
}

static struct kprobe kp;

int myinit(void)
{
	printk("module inserted\n");
	kp.pre_handler = Pre_Handler;
	kp.post_handler = Post_Handler;
	kp.addr = (kprobe_opcode_t *)0xc1045cc0;
	register_kprobe(&kp);
	return(0);
}

void myexit(void)
{
	unregister_kprobe(&kp);
	printk("module removed\n");
}

module_init(myinit);
module_exit(myexit);
MODULE_AUTHOR("BEN");
MODULE_DESCRIPTION("KPROBE TEST");
MODULE_LICENSE("GPL");
