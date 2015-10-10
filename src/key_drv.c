#include<linux/module.h>
#include<linux/version.h>
#include<linux/sched.h>
#include<linux/init.h>
#include<linux/ioport.h>
#include<asm/uaccess.h>
#include<linux/fs.h>
#include<asm/io.h>
#include<asm/types.h>
#include<asm/setup.h>
#include<asm/memory.h>
#include<asm/mach-types.h>
#include<asm/hardware.h>
#include<asm/irq.h>

#include<asm/mach/arch.h>
#include<asm/mach/map.h>
#include<asm/mach/irq.h>

#include<asm/arch/irq.h>
#include<asm/hardware/sa1111.h>
#include<linux/delay.h>

/* operations(fops���� �Լ���)*/
int key_open(struct inode *, struct file *);
int key_release(struct inode *, struct file *);
ssize_t key_read(struct file *, char *, size_t, loff_t *);


/* ���ͷ�Ʈ ó�� ���� �Լ��� */
int key_init(void);
void key_handler(int irq, void *dev_id, struct pt_regs *regs);

#define DEVICE_NAME	"keypad"
#define KEY_MAJOR	233		/* Ű ��ư�� MAJOR# */
#define KEY_BASE	0x14000000	/* key button address */
#define KEY_RANGE	1

static struct file_operations key_fops =
{
	open:		key_open,
	read:		key_read,
	release:		key_release,
}; // fops�� ����̹��� ����ϱ� ���� ����ü ����.

static int key_usage = 0;		// ���� ����̹��� ������ΰ�? open�Ǿ��� ��...
static int Major;			// �� ����̹��� MAJOR#
unsigned short *keyport;		// Ŀ�ο��� ����ϱ� ���� Ű ����̽��� �ٷ�� ���� �ּ�

static unsigned char key_value;	// ����̽� ����̹����� ���� ���� �����.

DECLARE_WAIT_QUEUE_HEAD(WaitQ_keypad);		// ��� ť ���� ����

// Ű ��ư�� �ʱ�ȭ�Ѵ�. 		: initialize key button
// - Ű ��ư�� ���� GPIO ��Ʈ 		: -GPIO port for key button interrupt
int key_init(void)
{
	int return_val;

	printk("keypad_init\n");
	Major = register_chrdev(KEY_MAJOR, DEVICE_NAME, &key_fops);
	// ���� MAJOR#233, ����̽� �̸�, key_fops�� �Ķ���ͷ� ����̽� ���� 
//����Ѵ�.
	if(Major < 0){
		printk("KEY init_module: failed with %d\n", Major);
		return Major;
	}
	if(Major==0)
		Major = KEY_MAJOR;
	printk("KEY driver registered: major = %d\n", Major);

	GPDR0  &= ~(1 << 0);			/* GPIO0���� �Է¹������� ���� */
	set_GPIO_IRQ_edge(0, GPIO_RISING_EDGE);	/* gpio_mask, rising_edge */
	return_val = request_irq(IRQ_GPIO(0), key_handler, SA_INTERRUPT,
		"KEY PAD INT", NULL);		/* ���ͷ�Ʈ ó�� �Լ� ��� */
	// ù ��° �μ� : ���ͷ�Ʈ ��ȣ
	// �� ��° �μ� : ó���Լ��� ���� ������. ���ͷ�Ʈ �߻� �� �� �Լ� ����.
	// �� ��° �μ� : SA_INTERRUPT(���� ���ͷ�Ʈ)
	// �� ��° �μ� : /proc./interrupts���� irq �����ġ�� ��ϵǴ� ���ڿ�
	// �ټ� ��° �μ� : ���ͷ�Ʈ ������ �� ����̽��� ������ �� �ִ� id
	if(return_val < 0){
	      printk(KERN_ERR"pxa255_pro_key_init : Can't request irq %#010x\n",
		IRQ_GPIO(0));
		return -1;
	}
	return 0;
}

void key_handler(int irq, void *dev_id, struct pt_regs *file)
{
	unsigned char value;

	keyport = (unsigned short *)ioremap_nocache(KEY_BASE, 2);
	// Ű ��ư�� ���� ���� �ּҸ� Ŀ���� ����� �� �ֵ��� keyport�����ͷ� 
//mapping
	// nocache�� �������ġ�̱� ������ ���ش�.
	value = *keyport;		// �ּҷ� ���� Ű ���� �д´�.
	key_value = value & 0xf;	// ���� ���� 4��Ʈ�� ���޵Ǳ� ������ ���� Ű���� 
//�ɷ���.
#ifdef DEBUG
	printk("KEY INPUT : [%dh] \n", key_value);
#endif
	wake_up_interruptible(&WaitQ_keypad);	// ��� ���� ����̹�(read�Լ�)�//� 
//wakeup ��Ŵ.
	iounmap(keyport);			// mapping�� �ּҿ� ���� �ڿ� ��//��
}

// ���μ����� ����̽� ������ open�ϸ� ������ ������ ȣ���.
int key_open(struct inode *inode, struct file *file)
{
	if(key_usage != 0) return -EBUSY;	// ���� ����̽��� ������̸� ��� �Ұ�.
	MOD_INC_USE_COUNT;		// ����� ���� �ϳ� ������Ŵ.
	key_usage = 1;			// ���� ����̽��� ������̶�� �˸�.
	printk("open..ok \n");
	return 0;
}

// ���μ����� ����̽� ������ close�� �� ȣ���.
// Doesn't have a return value in version 2.0.x because it can't fail,
// but in version 2.2.x it is allowed to fail
int key_release(struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;		// ����� ���� �ϳ� ���ҽ�Ŵ.
	key_usage = 0;			// ���� ����̽��� ��������� ����.
	printk("device release\n");
	return 0;
}

ssize_t key_read(struct file *inode, char *gdata, size_t length, loff_t 
*off_what)
{
	printk("read..... start\n");
	interruptible_sleep_on(&WaitQ_keypad);	// wakeup(Ű �Է�)�̵����� ������ 
//sleep
	copy_to_user(gdata, &key_value, 1);	// wakeup�Ǿ����� ���� ���� 
//�������α׷��� ����
	printk("read..... ok\n");

	return length;
}

#ifdef MODULE
int init_module(void)
{
	return key_init();			// ����̽� �� ���ͷ�Ʈ ����Լ��� ȣ����.
}

void cleanup_module(void)
{
	int ret;
	free_irq(IRQ_GPIO(0), NULL);		// ���ͷ�Ʈ ó�� �Լ� ����.

	ret = unregister_chrdev(Major, DEVICE_NAME);	// Ŀ���� �� ����̽��� 
//������� �ʵ���
	if(ret < 0){
		printk("unregister_chrdev: error %d\n", ret);
	} else {
		printk("module clean up ok!!\n");
	}
}
#endif /* MODULE */

