#include <linux/init.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Hello_world");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cheng-Wei Lin");

static int __init hello_init(void) {
	printk(KERN_INFO "Greeting from a linux kernel module.\n");
	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO "Bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);
