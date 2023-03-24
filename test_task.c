#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/rwsem.h>

#define PROC_NAME "test_task"

struct data_node {
    struct list_head list;
    char data[64];
};

static struct list_head data_list;
static struct rw_semaphore data_list_sem;

static ssize_t test_task_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    struct data_node *node;
    size_t len = 0;
    ssize_t read_count;
    char *tmp_buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);

    if (!tmp_buffer) {
        return -ENOMEM;
    }

    down_read(&data_list_sem);
    list_for_each_entry(node, &data_list, list) {
        len += scnprintf(tmp_buffer + len, PAGE_SIZE - len, "%s\n", node->data);
    }
    up_read(&data_list_sem);

    read_count = simple_read_from_buffer(buf, count, pos, tmp_buffer, len);
    kfree(tmp_buffer);

    return read_count;
}

static ssize_t test_task_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {
    struct data_node *node;

    if (count > 63) {
        count = 63;
    }

    node = kmalloc(sizeof(struct data_node), GFP_KERNEL);
    if (!node) {
        return -ENOMEM;
    }

    if (copy_from_user(node->data, buf, count)) {
        kfree(node);
        return -EFAULT;
    }
    node->data[count] = '\0';

    down_write(&data_list_sem);
    list_add_tail(&node->list, &data_list);
    up_write(&data_list_sem);

    return count;
}

static const struct proc_ops test_task_fops = {
    .proc_read = test_task_read,
    .proc_write = test_task_write,
    .proc_lseek = no_llseek,
};

static int __init test_task_init(void) {
    INIT_LIST_HEAD(&data_list);
    init_rwsem(&data_list_sem);

    if (!proc_create(PROC_NAME, 0666, NULL, &test_task_fops)) {
        return -ENOMEM;
    }

    return 0;
}

static void __exit test_task_exit(void) {
    struct data_node *node, *tmp_node;

    down_write(&data_list_sem);
    list_for_each_entry_safe(node, tmp_node, &data_list, list) {
        list_del(&node->list);
        kfree(node);
    }
    up_write(&data_list_sem);

    remove_proc_entry(PROC_NAME, NULL);
}

module_init(test_task_init);
module_exit(test_task_exit);


MODULE_LICENSE("GNU");
MODULE_AUTHOR("Nikita");
MODULE_DESCRIPTION("task");

