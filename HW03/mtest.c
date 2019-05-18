#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>	// copy_from_user
#include <linux/mm.h>		// vm_flags: VM_READ, VM_WRITE...

MODULE_DESCRIPTION("Linux Kernel HW03: Memory Management");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cheng-Wei Lin");

static int my_proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "[HW03] Read mtext.\n");
	return 0;
}

/*
 * Things to do wile opening the proc file.
 */
static int my_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, my_proc_show, NULL);
}

/*
// Open file in kernel module.
struct file *file_open(const char *path, int flags) {
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, 0);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

// Close file in kernel module.
void file_close(struct file *file) {
	filp_close(file, NULL);
}

// Read from file in kernel module.
int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size) {
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = kernel_read(file, data, size, &offset);

	set_fs(oldfs);
	return ret;
}
*/

#ifdef __HAVE_ARCH_GATE_AREA
static const char *my_gate_vma_name(struct vm_area_struct *vma) {
	return "[vsyscall]";
}
static const struct vm_operations_struct my_gate_vma_ops = {
	.name = my_gate_vma_name,
};
static struct vm_area_struct my_gate_vma = {
	.vm_start		= VSYSCALL_ADDR,
	.vm_end			= VSYSCALL_ADDR + PAGE_SIZE,
	.vm_page_prot	= PAGE_READONLY_EXEC,
	.vm_flags		= VM_READ | VM_EXEC,
	.vm_ops			= &my_gate_vma_ops,
};
#else
static struct vm_area_struct my_gate_vma = NULL;
#endif

static struct vm_area_struct *my_next_vma(struct vm_area_struct *tail_vma, struct vm_area_struct *vma) {
	if (vma == tail_vma)
		return NULL;
	return vma->vm_next ?: tail_vma;
}

/* Print all vma blocks used by current process. */
static void my_show_vma(void) {
	// Get current task.
	struct task_struct *task = current;
	struct vm_area_struct *vma = NULL, *tail_vma = NULL;

	if (!task) return ;

	// Traverse the whole vma.
	printk(KERN_INFO "-----------------------------------------\n");
	printk(KERN_CONT "listvma for current process (pid: %d)\n", task->pid);
	if (task->mm && task->mm->mmap) {
		tail_vma = &my_gate_vma;
		for (vma = task->mm->mmap; vma; vma = my_next_vma(tail_vma, vma)) {
			printk(KERN_CONT "%#lx %#lx ", vma->vm_start, vma->vm_end);
			// Show flag.
			printk(KERN_CONT "%c", vma->vm_flags & VM_READ ? 'r' : '-');
			printk(KERN_CONT "%c", vma->vm_flags & VM_WRITE ? 'w' : '-');
			printk(KERN_CONT "%c", vma->vm_flags & VM_EXEC ? 'x' : '-');
			printk(KERN_CONT "%c\n", vma->vm_flags & VM_MAYSHARE ? 's' : 'p');
		}
	}
}

/*
 * Write something to the proc file then get the output.
 * 
 * Input/Output:
 * 1. listvma: print all vma of current process in the format
 *         of "start-addr end-addr permission".
 * 2. findpage addr: find virtual addr -> physical addr tran-
 *         slation of dddr in current proccess's mm.
 * 3. writeval addr val: change an unsigned long size content
 *         in current process's virtual addr into val.
 */
static ssize_t my_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
	char kbuf[64], *pos;
	ssize_t ret;

	// Only allow 64-bits of string to be written.
	ret = -EINVAL;
	if (count >= sizeof(kbuf))
		goto out;
	
	// What was written?
	ret = -EFAULT;
	if (copy_from_user(kbuf, buf, count))
		goto out;
	kbuf[count] = '\0';
	pos = kbuf;

	// What is being requested?
	ret = -EINVAL;
	if (strncmp(pos, "listvma", 7) == 0) {
		pos += 7;
		my_show_vma();
	}
	else if (strncmp(pos, "findpage", 8) == 0) {
		pos += 8;
		printk(KERN_INFO "[mtest] Get findpage.\n");
	}
	else if (strncmp(pos, "writeval", 8) == 0) {
		pos += 8;
		printk(KERN_INFO "[mtest] Get writeval.\n");
	}
	else
		goto out;

	// Verify there is not trailing junk on the line.
	pos = skip_spaces(pos);
	if (*pos != '\0')
		goto out;

	// Report a successful write.
	*ppos = 0;	// Always write to the beginning of the file.
	ret = count;

out:
	return ret;
}
/***********************************************************/

static struct file_operations my_fops ={
	.open		= my_proc_open,
	.write		= my_proc_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/* First called function. */
static int __init hello_init(void) {
	struct proc_dir_entry *entry;

	// Create /proc/mtest.
	printk(KERN_INFO "mtest is loaded!\n");
	printk(KERN_INFO "Try to create /proc/mtest...\n");
	entry = proc_create("mtest", 0666, NULL, &my_fops);
	if (!entry) return -1;
	else printk(KERN_INFO "/proc/mtest is created.\n");

	return 0;
}

/* Last called function. */
static void __exit hello_exit(void) {
	remove_proc_entry("mtest", NULL);
	printk(KERN_INFO "mtest is removed!\n");
}

module_init(hello_init);
module_exit(hello_exit);
