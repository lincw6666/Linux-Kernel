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

static unsigned long long my_va2pa(const unsigned long long va) {
	struct task_struct *task = current;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	struct page *page;
	unsigned long long pa;

	if (!task) return 0;
	
	pgd = pgd_offset(task->mm, va);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return 0;
	pud = pud_offset((p4d_t *)pgd, va);
	if (pud_none(*pud) || pud_bad(*pud))
		return 0;
	pmd = pmd_offset(pud, va);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return 0;
	pte = pte_offset_map(pmd, va);
	if (pte_none(*pte))
		return 0;
	if (!(page = pte_page(*pte)))
		return 0;
	pa = page_to_phys(page) | (va & ~PAGE_MASK);
	pte_unmap(pte);
	
	return pa;
}

/* Translate virtual addr (va) to physical addr (pa). */
static void my_findpage(const unsigned long long va) {
	// Get current task.
	unsigned long long pa = my_va2pa(va);

	printk(KERN_INFO "-----------------------------------------\n");
	printk(KERN_CONT "findpage for addr=%#llx:\n", va);
	
	if (pa)
		printk(KERN_CONT "vma %#llx -> pma %#llx\n", va, pa);
	else
		printk(KERN_INFO "Translation not found.\n");
}

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

static bool is_valid_hex(char hex) {
	if (('0' <= hex && hex <= '9') || ('a' <= hex && hex <= 'f'))
		return true;
	return false;
}
static unsigned long hex2uint(char hex) {
	if ('0' <= hex && hex <= '9')
		return hex - '0';
	else if ('a' <= hex && hex <= 'f')
		return hex - 'a' + 10;
	return 0;
}
static unsigned long long get_hex(char **str) {
	unsigned long long ret = 0;

	while (is_valid_hex(**str))
		ret = (ret<<4) + hex2uint(*((*str)++));
	return ret;
}
static bool my_get_val(char **str, unsigned long long *val) {
	// Hex value must start with '0x'.
	if (strncmp(*str, "0x", 2) == 0) {
		*str += 2;
		// Get the hex value..
		*val = get_hex(str);
		return true;
	}
	else return false;
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
	unsigned short actions = 0;	// 1: listvma
								// 2: findpage
								// 3: writeval
	unsigned long long va = 0;	// For findpage && writeval
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
	// listvma
	if (strncmp(pos, "listvma", 7) == 0) {
		pos += 7;
		actions = 1;
	}
	// findpage
	else if (strncmp(pos, "findpage", 8) == 0) {
		pos += 8;
		actions = 2;
		pos = skip_spaces(pos);
		if (!my_get_val(&pos, &va))
			goto out;
	}
	// writeval
	else if (strncmp(pos, "writeval", 8) == 0) {
		pos += 8;
		actions = 3;
	}
	else {
		printk(KERN_INFO "Invalid input!!\n");
		goto out;
	}

	// Verify there is not trailing junk on the line.
	pos = skip_spaces(pos);
	if (*pos != '\0')
		goto out;

	// Do the action.
	if (actions == 1) my_show_vma();
	else if (actions == 2) my_findpage(va);

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
