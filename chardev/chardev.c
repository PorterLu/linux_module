/*
 * chardev.c: Creates a read-only char device that says how many times
 * you have read from the dev file
 */
#include <linux/atomic.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> /* for sprintf() */
#include <linux/module.h> 
#include <linux/types.h>
#include <linux/uaccess.h> /* for get_user and put_user */

#include <asm/errno.h>

/* Prototypes - this would normally go in a .h file */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 80

static int major; /* major number assigned to our device driver */

enum {
  CDEV_NOT_USED = 0,
  CDEV_EXCLUSIVE_OPEN = 1,
};

/* Is device open? Used to prevent multiple access to device */
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static char msg[BUF_LEN + 1];

static struct class *cls;

static struct file_operations chardev_fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release,
};

static int __init chardev_init(void) {
  /* register a major device number */
  major = register_chrdev(0, DEVICE_NAME, &chardev_fops);

  if (major < 0) {
    pr_alert("Registering char device failed with %d\n", major);
    return major;
  }

  pr_info("I was assigned major number %d.\n", major);

  /* create a device under /dev */
  cls = class_create(THIS_MODULE, DEVICE_NAME);
  device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

  return SUCCESS;
}

static void __exit chardev_exit(void) {
  device_destroy(cls, MKDEV(major, 0));
  class_destroy(cls);

  unregister_chrdev(major, DEVICE_NAME);
}

static int device_open(struct inode *inode, struct file *file) {
  static int counter = 0;

  /* If already_open == CDEV_NOT_USED, then set it to CDEV_EXCLUSIVE_OPEN */
  if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
    return -EBUSY;

  sprintf(msg, "I already told you %d times Hello world!\n", counter++);
  /* increase module's reference number */
  try_module_get(THIS_MODULE);
  
  return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file) {
  atomic_set(&already_open, CDEV_NOT_USED);

  /* decrease module's reference number */
  module_put(THIS_MODULE);
  
  return SUCCESS;
}

static ssize_t device_read(struct file *filp, /* see include /linux/fs.h */
                            char __user *buffer, /* buffer to fill with data */
                            size_t length, /* length of th buffer */
                            loff_t *offset)
{
  int bytes_read = 0;
  const char *msg_ptr = msg;

  if (!*(msg_ptr + *offset)) {
    *offset = 0;
    return 0;
  }

  msg_ptr += *offset;
  while (length && *msg_ptr) {
    put_user(*(msg_ptr++), buffer++);
    length--;
    bytes_read++;
  }

  *offset += bytes_read;

  return bytes_read;
} 


static ssize_t device_write(struct file *filp, const char __user *buff,
                            size_t len, loff_t *off)
{
  pr_alert("Sorry, this operation is not supported.\n");
  return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");

