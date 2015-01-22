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
#include <linux/kmod.h>
#include <linux/string.h>
#include <linux/buffer_head.h>
#include <linux/delay.h>
struct file* file_open(const char *path, int flags, int mode)
{
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, mode);
	set_fs(oldfs);

	if(IS_ERR(filp))
	{
		err = PTR_ERR(filp);
		return(NULL);
	}
	return(filp);
}

void file_close(struct file* the_file)
{
	filp_close(the_file, NULL);
}

int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, size, &offset);

	set_fs(old_fs);
	return ret;
}


static int uhm(void)
{
	char *argv[] = {"/home/user/wrappertest/a.out\0", "/home/user/poc/vuln4\0", NULL};
	static  char *envp[] =
	{
		"HOME=/",
		"TERM=linux",
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
		NULL
	};

	return call_usermodehelper_fns(argv[0], argv, envp, 1, NULL, NULL, NULL);
}


void create_list_node(void)
{
	//we initialize the linked list for this task
	struct mm_struct *mm = current->mm;
	INIT_LIST_HEAD(&mm->alt_eip_stack.mylist);
}

//unsigned int *disass[2] is an 2D array, first cell is the address
//second cell is the type (0 = call, 1 = ret, 2 = jmp)
int read_memory_process(char *path1, char *path2)
{
	//variables
	unsigned char buf[1];
	unsigned int i = current->mm->end_code;
	unsigned char patch_call[7] = {0xb8, 0x5f, 0x01, 0x00, 0x00, 0xcd, 0x80};
	unsigned char patch_ret[7] = {0xb8, 0x60, 0x01, 0x00, 0x00, 0xcd, 0x80};
	unsigned int shift = 0;
	unsigned int j = 0, k = 0;
	unsigned int disass[90][2];
	unsigned char tmp_read[180];
	unsigned char tmp_add[8];
	unsigned int nb_read = 0;
   	unsigned int nb_elem = 0;
   	unsigned int real_start;
   	unsigned int real_end;
	struct file *f = NULL;
	unsigned int cmp = 0;
	unsigned char add_read[4];
	unsigned char temp_int_add = 0;
	unsigned int temp_shift = 0;
	
	//we read the second document
	f = file_open(path2, O_RDONLY, 0);
	if (f == NULL)
	{
		//ERROR
		printk("FIRST\n");
		return(-1);
	}
	//we read the entire
	nb_read = file_read(f, 0, tmp_read, 20);
	//we copy the first addresse and the second
	strncpy(tmp_add, tmp_read, 8);
	sscanf(tmp_add, "%08x", &real_start);
	strncpy(tmp_add, tmp_read + 9, 8);
	sscanf(tmp_add, "%08x", &real_end);

	file_close(f);
	f = NULL;

	//we read the first document
	f = file_open(path1, O_RDONLY, 0);
	if (f == NULL)
	{
		//ERROR
		printk("SECOND");
		return(-1);
	}
	//we read the entire file
	nb_read = file_read(f, 0, tmp_read, 2000);
	
	//we loop to put everything at the good place
	while(j < nb_read)
	{
		//we read the addresse 8 bits
		strncpy(tmp_add, tmp_read + j, 8);
		sscanf(tmp_add, "%08x", &disass[k][0]);
		//we shift and drop the space
		j += 9;
		// x - 48 value in ascii to decimal
		disass[k][1] = tmp_read[j] - 48;

		//we shift to align to the first char of the next line
		j += 2;

		//we inc the nb_elem
		nb_elem++;
		cmp++;
		k++;
	}
	//we start from 0 each time so we dec the cmp
	cmp--;

	file_close(f);

	j = 0;
	k = 0;

	//we calculate the shift
	for (j = 0 ; j < nb_elem ; j++)
	{
		//if the type != 2 we have a shift
		if (disass[j][1] != 2)
		{
			shift += 7;
		}
	}

	//we start to copy from the end_code - shift, the add of nop gave us space
	i = real_end - shift - 17;
	
	while(i > real_start)
	{
		if(cmp + 1 > 0 && i <= disass[cmp][0])
		{
			//we have something to do and the instruction is already wrote
			if(disass[cmp][1] == 0)
			{
				//call part
				//is the call point to the text segment ?
				//if not, no patch
				if((disass[cmp][0] > real_start) && (disass[cmp][0] < real_end))
				{
					//we change the add
					//we copy the prefix of the call first
					access_remote_vm(current->mm, i, buf, 1, 0);
					access_remote_vm(current->mm, i + shift , buf, 1, 1);
					
					//we read args of the call
					access_remote_vm(current->mm, i + 1, add_read, 4, 0);
					temp_int_add = 0x000000 + add_read[0];
					//we check if the call is in the range of the real text segment
					//if not we do nothing, the rest of the patch will copy the old value
					if(((i + shift + temp_int_add + 5) >= real_start) && ((i + shift + temp_int_add + 5) <= real_end))
					{
						temp_shift = 1; //we start from one to do not forget the actual injection
						//we loop the array to find the injection BETWEEN the call and the function called
						for (j = cmp ; disass[j][0] > i && j >= 0 ; j--)
						{
							if ((disass[j][0] < (disass[cmp][0] + temp_int_add + 5))) //&& (disass[j][0] < (disass[cmp][0] - temp_int_add +5)))
							{
								if(disass[j][1] != 2)
								{
									temp_shift++;
								}
							}
						}
						//now we recalculate the shift
						printk("add %02x%02x%02x%02x\n", add_read[0], add_read[1], add_read[2], add_read[3]);
						add_read[0] = temp_int_add + (temp_shift * 7);
						//and we rewrite the call
						access_remote_vm(current->mm, i + 1 + shift, add_read, 4, 1);
						//printk("SHIFT %d %d\n", shift, shift-6);
						if ((shift - 6) >= 0)	/* // */
						{
							shift -= 7;
						}
						//we inject the patch
						access_remote_vm(current->mm, i + shift, patch_call, 7, 1);
						i--;
						
					}
				}
			}
			else if(disass[cmp][1] == 1)
			{
				//ret part
				//first we copy the ret op code
				access_remote_vm(current->mm, i, buf, 1, 0);
				access_remote_vm(current->mm, i + shift, buf, 1, 1);
				//we inject the patch
				if ((shift - 6) >= 0)	/* // */
				{
					shift -= 7;
				}
				access_remote_vm(current->mm, i + shift, patch_ret, 7, 1);
				i--;
			}
			else if(disass[cmp][1] == 2)
			{
				//jmp part
				//is the jump point to the text segment ?
				//if not, no patch
				if((disass[cmp][0] > real_start) && (disass[cmp][0] < real_end))
				{
					//we change the add
					
				}
			}
			else
			{
				//ERROR !
			}
			cmp--;
		}
		else
		{
			//we copy the rest of the program if no modification is needed
			access_remote_vm(current->mm, i, buf, 1, 0);
			access_remote_vm(current->mm, i + shift, buf, 1, 1);
			i--;
		}
	}
	access_remote_vm(current->mm, 0x0804823e, buf, 1, 0);
	return(0);	
}


int Pre_Handler(struct kprobe *p, struct pt_regs *regs)
{
	//do nothing
	return(0);
}

void Post_Handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	if(current_uid() != 0)
	{
		struct mm_struct *mm = get_task_mm(pid_task(find_get_pid(current->pid), PIDTYPE_PID));
		down_read(&mm->mmap_sem);
		uhm();
		if(read_memory_process("/tmp/1", "/tmp/12") == -1)
		{
			printk("ERROR at the file reading.\n");
		}	
		create_list_node();
		up_read(&mm->mmap_sem);
	}
}

static struct kprobe kp;

int myinit(void)
{
	printk("module inserted\n");
	kp.pre_handler = Pre_Handler;
	kp.post_handler = Post_Handler;
	kp.addr = (kprobe_opcode_t *)0xc1001596;
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
