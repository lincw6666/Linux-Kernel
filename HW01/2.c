#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_DESCRIPTION("Hello_world");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cheng-Wei Lin");

#define ARR_MAX_LEN 3

// Static variables.
static int int_param;
static char *string_param = "";
static int arr_len;
static int array_param[ARR_MAX_LEN];

module_param(int_param,		int,	S_IRUSR | S_IWUSR);
module_param(string_param,	charp,	S_IRUSR | S_IWUSR);
module_param_array(array_param, int, &arr_len, S_IRUSR | S_IWUSR);

static int __init hello_init(void) {
	int arr_index = 0;

	printk(KERN_INFO "Param: int_param: %d;\n", int_param);
	printk(KERN_INFO "\tstring_param: %s;\n", string_param);
	printk(KERN_INFO "\tarray_param: ");
	while (arr_index < arr_len && arr_index < ARR_MAX_LEN) {
		printk(KERN_CONT "%d,", *(array_param+arr_index));
		++arr_index;
	}
	printk(KERN_CONT "\n");

	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO "Bye!\n");
}

module_init(hello_init);
module_exit(hello_exit);
