#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void) {
	printk(KERN_INFO ">> Hello, Kernel World!\n");
	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO ">> Goodbye, Kernel World!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Basic Hello World Kernel Module");
