#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_DESCRIPTION("Hello_world");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cheng-Wei Lin");

static int myProcShow(struct seq_file *m, void *v) {
	seq_printf(m, "Message from a linux kernel module~.~\n");
	return 0;
}

static int myProcOpen(struct inode *inode, struct file *file) {
	return single_open(file, myProcShow, NULL);
}

static struct file_operations myFops ={
	.owner	= THIS_MODULE,
	.open	= myProcOpen,
	.release = single_release,
	.read	= seq_read,
	.llseek	= seq_lseek
};

static int __init hello_init(void) {
	struct proc_dir_entry *entry;

	entry = proc_create("Task1", 0444, NULL, &myFops);
	if (!entry) return -1;
	else printk(KERN_INFO "Proc_entry created successfully.\n");

	return 0;
}

static void __exit hello_exit(void) {
	remove_proc_entry("Task1", NULL);
}

module_init(hello_init);
module_exit(hello_exit);
