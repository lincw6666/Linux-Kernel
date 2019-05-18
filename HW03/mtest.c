#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_DESCRIPTION("Linux Kernel HW03: Memory Management");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cheng-Wei Lin");

static int my_proc_show(struct seq_file *m, void *v) {
	seq_printf(m, "[HW03] Read mtext.\n");
	return 0;
}

static int my_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, my_proc_show, NULL);
}

static struct file_operations my_fops ={
	.open	= my_proc_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
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
