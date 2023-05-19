#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

/* __initdata works similarly to __init but for init variables */
static int hello3_data __initdata = 3;

static int __init init_hello_3(void) {
  pr_info("Hello, world %d\n", hello3_data);
  return 0;
}

static void __exit cleanup_hello_3(void) {
  pr_info("Goodbye, world 3\n");
}

module_init(init_hello_3);
module_exit(cleanup_hello_3);

MODULE_LICENSE("GPL");