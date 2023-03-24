#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define PROC_NAME "test_task"
#define BUFFER_SIZE 128

static char *data_buffer;
static int data_size;

static ssize_t test_task_read(struct file *file, char __user *buf, size_t count,
                              loff_t *pos) {
  return simple_read_from_buffer(buf, count, pos, data_buffer, data_size);
}

static ssize_t test_task_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *pos) {
  if (count > BUFFER_SIZE - data_size - 1) {
    count = BUFFER_SIZE - data_size - 1;
  }

  if (copy_from_user(data_buffer + data_size, buf, count)) {
    return -EFAULT;
  }

  data_size += count;
  data_buffer[data_size] = '\n';
  data_size++;

  return count;
}

static const struct proc_ops test_task_fops = {
    .proc_read = test_task_read,
    .proc_write = test_task_write,
};

static int __init test_task_init(void) {
  data_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
  if (!data_buffer) {
    return -ENOMEM;
  }

  data_size = 0;

  if (!proc_create(PROC_NAME, 0666, NULL, &test_task_fops)) {
    kfree(data_buffer);
    return -ENOMEM;
  }

  return 0;
}

static void __exit test_task_exit(void) {
  remove_proc_entry(PROC_NAME, NULL);
  kfree(data_buffer);
}

module_init(test_task_init);
module_exit(test_task_exit);

MODULE_LICENSE("GNU");
MODULE_AUTHOR("Nikita");
MODULE_DESCRIPTION("task");
