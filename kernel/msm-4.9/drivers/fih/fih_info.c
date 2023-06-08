#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include "fih_ramtable.h"

static unsigned int my_proc_addr = FIH_MODEM_RF_NV_BASE;
static unsigned int my_proc_size = FIH_MODEM_RF_NV_SIZE;
static unsigned int my_proc_len = 0;

/* This function is called at the beginning of a sequence.
 * ie, when:
 * - the /proc file is read (first time)
 * - after the function stop (end of sequence)
 */
static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
	if (((*pos)*PAGE_SIZE) >= my_proc_len) return NULL;
	return (void *)((unsigned long) *pos+1);
}

/* This function is called after the beginning of a sequence.
 * It's called untill the return is NULL (this ends the sequence).
 */
static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
	return my_seq_start(s, pos);
}

/* This function is called at the end of a sequence
 */
static void my_seq_stop(struct seq_file *s, void *v)
{
	/* nothing to do, we use a static value in start() */
}

/* This function is called for each "step" of a sequence
 */
static int my_seq_show(struct seq_file *s, void *v)
{
        /*int n = (int)v-1;*/
        long n = (long)v-1;

	char *buf = (char *)ioremap(my_proc_addr, my_proc_size);

	if (buf == NULL) {
		return 0;
	}

	if (my_proc_len < (PAGE_SIZE*(n+1))) {
		seq_write(s, (buf+(PAGE_SIZE*n)), (my_proc_len - (PAGE_SIZE*n)));
	} else {
		seq_write(s, (buf+(PAGE_SIZE*n)), PAGE_SIZE);
	}

	iounmap(buf);

	return 0;
}

/* This structure gather "function" to manage the sequence
 */
static struct seq_operations my_seq_ops = {
	.start = my_seq_start,
	.next  = my_seq_next,
	.stop  = my_seq_stop,
	.show  = my_seq_show
};

/* This function is called when the /proc file is open.
 */
static int my_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &my_seq_ops);
};

static struct file_operations mdm_rf_nv_file_ops = {
	.owner   = THIS_MODULE,
	.open    = my_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

static struct {
		char *name;
		struct file_operations *ops;
} *p, fih_info[] = {
	{"mdm_rf_nv", &mdm_rf_nv_file_ops},
	{NULL}, };

static int __init fih_info_init(void)
{
	my_proc_len = my_proc_size;

	for (p = fih_info; p->name; p++) {
		if (proc_create(p->name, 0, NULL, p->ops) == NULL) {
			pr_err("fail to create proc/%s\n", p->name);
		}
	}

	return (0);
}

static void __exit fih_info_exit(void)
{
	for (p = fih_info; p->name; p++) {
		remove_proc_entry(p->name, NULL);
	}
}

module_init(fih_info_init);
module_exit(fih_info_exit);
