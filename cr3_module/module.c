#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>	
#include <linux/mm.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <asm/io.h>

#define PROC_NAME	"cr3"

MODULE_AUTHOR("VUSec");
MODULE_DESCRIPTION("provides access to the CPU's CR3 register");
MODULE_LICENSE("GPL");
MODULE_INFO(intree, "Y"); 

static void *my_seq_start(struct seq_file *s, loff_t *pos) {
	static unsigned long counter = 0;

	if ( *pos == 0 ) {	
		return &counter;
	} else {	
		*pos = 0;
		return NULL;
	}
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos) {

	return NULL;
}
 
static void my_seq_stop(struct seq_file *s, void *v) {}

static int my_seq_show(struct seq_file *s, void *v) {
	
    uint64_t cr3;

	// bitmask [38:12] to extract address of PTL4: 0x7FFFFFF000
	__asm__ __volatile__(
		"mov %%cr3, %%rax\n\t"
		"mov $0x7FFFFFF000, %%rbx\n\t"
		"and %%rbx, %%rax\n\t"
		"mov %%rax, %0\n\t"
		: "=m"(cr3)   // outputs
		:                  // inputs
		: "%rax", "%rbx"  // clobbers
		);
    seq_printf(s, "%llx\n", cr3);
	return 0;
}

static struct seq_operations my_seq_ops = {
	.start = my_seq_start,
	.next  = my_seq_next,
	.stop  = my_seq_stop,
	.show  = my_seq_show
};

static int my_open(struct inode *inode, struct file *file) {
	return seq_open(file, &my_seq_ops);
};

static struct file_operations my_file_ops = {
	.owner   = THIS_MODULE,
	.open    = my_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
	
int init_module(void) {
	struct proc_dir_entry *entry;

	entry = proc_create(PROC_NAME, 0, NULL, &my_file_ops);

	return 0;
}

void cleanup_module(void) {
	remove_proc_entry(PROC_NAME, NULL);
}
