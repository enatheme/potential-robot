#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/ptrace.h>
#include <linux/list.h>
#include <linux/slab.h>


void stacking_eip(void)
{
	//variables
	struct mm_struct *mm;
	struct Alt_stack *insert;
	struct pt_regs *my_regs;

	//allocation
	mm = current->mm;
	insert = kmalloc(sizeof(*insert), GFP_KERNEL);
	my_regs = task_pt_regs(current);

	//we pick up eip
	//the current eip is on the int 0x80
	//we have to add size of call to the current eip
	//sizeof(call) = 5
	insert->eip = my_regs->ip + 7;

	//we create the node
	INIT_LIST_HEAD(&insert->mylist);

	//we add it at the end of the linked list
	list_add_tail(&insert->mylist, &mm->alt_eip_stack.mylist);
}

int Pre_Handler(struct kprobe *p, struct pt_regs *regs)//, unsigned long flags)
{
	//we hook just the kernel space
	if(current_uid() != 0)
	{
		stacking_eip();
		printk("CALL\n");
	}
	return(0);
}

void Post_Handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
}

static struct kprobe kp;

int myinit(void)
{
	printk("module inserted\n");
	kp.pre_handler = Pre_Handler;
	kp.post_handler = Post_Handler;
	kp.addr = (kprobe_opcode_t *)0xc1045cac;
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
MODULE_DESCRIPTION("my_call hook");
MODULE_LICENSE("GPL");
