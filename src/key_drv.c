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

/* operations(fops관련 함수들)*/
int key_open(struct inode *, struct file *);
int key_release(struct inode *, struct file *);
ssize_t key_read(struct file *, char *, size_t, loff_t *);


/* 인터럽트 처리 관련 함수들 */
int key_init(void);
void key_handler(int irq, void *dev_id, struct pt_regs *regs);

#define DEVICE_NAME	"keypad"
#define KEY_MAJOR	233		/* 키 버튼의 MAJOR# */
#define KEY_BASE	0x14000000	/* key button address */
#define KEY_RANGE	1

static struct file_operations key_fops =
{
	open:		key_open,
	read:		key_read,
	release:		key_release,
}; // fops를 드라이버에 등록하기 위한 구조체 정의.

static int key_usage = 0;		// 현재 드라이버가 사용중인가? open되었을 때...
static int Major;			// 이 드라이버의 MAJOR#
unsigned short *keyport;		// 커널에서 사용하기 위한 키 디바이스를 다루기 위한 주소

static unsigned char key_value;	// 디바이스 드라이버에서 읽은 값이 저장됨.

DECLARE_WAIT_QUEUE_HEAD(WaitQ_keypad);		// 대기 큐 정적 선언

// 키 버튼을 초기화한다. 		: initialize key button
// - 키 버튼을 위한 GPIO 포트 		: -GPIO port for key button interrupt
int key_init(void)
{
	int return_val;

	printk("keypad_init\n");
	Major = register_chrdev(KEY_MAJOR, DEVICE_NAME, &key_fops);
	// 문자 MAJOR#233, 디바이스 이름, key_fops를 파라미터로 디바이스 모듈로 
//등록한다.
	if(Major < 0){
		printk("KEY init_module: failed with %d\n", Major);
		return Major;
	}
	if(Major==0)
		Major = KEY_MAJOR;
	printk("KEY driver registered: major = %d\n", Major);

	GPDR0  &= ~(1 << 0);			/* GPIO0번을 입력받향으로 설정 */
	set_GPIO_IRQ_edge(0, GPIO_RISING_EDGE);	/* gpio_mask, rising_edge */
	return_val = request_irq(IRQ_GPIO(0), key_handler, SA_INTERRUPT,
		"KEY PAD INT", NULL);		/* 인터럽트 처리 함수 등록 */
	// 첫 번째 인수 : 인터럽트 번호
	// 두 번째 인수 : 처리함수에 대한 포인터. 인터럽트 발생 시 이 함수 수행.
	// 세 번째 인수 : SA_INTERRUPT(빠른 인터럽트)
	// 네 번째 인수 : /proc./interrupts에서 irq 사용장치로 등록되는 문자열
	// 다섯 번째 인수 : 인터럽트 공유시 각 디바이스를 구별할 수 있는 id
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
	// 키 버튼에 대한 실제 주소를 커널이 사용할 수 있도록 keyport포인터로 
//mapping
	// nocache는 입출력장치이기 때문에 써준다.
	value = *keyport;		// 주소로 부터 키 값을 읽는다.
	key_value = value & 0xf;	// 실제 값은 4비트로 전달되기 때문에 읽은 키값을 
//걸러냄.
#ifdef DEBUG
	printk("KEY INPUT : [%dh] \n", key_value);
#endif
	wake_up_interruptible(&WaitQ_keypad);	// 대기 중인 드라이버(read함수)�//� 
//wakeup 시킴.
	iounmap(keyport);			// mapping된 주소에 대한 자원 반//납
}

// 프로세스가 디바이스 파일을 open하며 접근할 때마다 호출됨.
int key_open(struct inode *inode, struct file *file)
{
	if(key_usage != 0) return -EBUSY;	// 현재 디바이스가 사용중이면 사용 불가.
	MOD_INC_USE_COUNT;		// 사용자 수를 하나 증가시킴.
	key_usage = 1;			// 현재 디바이스를 사용중이라고 알림.
	printk("open..ok \n");
	return 0;
}

// 프로세스가 디바이스 파일을 close할 때 호출됨.
// Doesn't have a return value in version 2.0.x because it can't fail,
// but in version 2.2.x it is allowed to fail
int key_release(struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;		// 사용자 수를 하나 감소시킴.
	key_usage = 0;			// 현재 디바이스를 사용중이지 않음.
	printk("device release\n");
	return 0;
}

ssize_t key_read(struct file *inode, char *gdata, size_t length, loff_t 
*off_what)
{
	printk("read..... start\n");
	interruptible_sleep_on(&WaitQ_keypad);	// wakeup(키 입력)이들어오기 전까지 
//sleep
	copy_to_user(gdata, &key_value, 1);	// wakeup되었으면 읽은 값을 
//응용프로그램에 보냄
	printk("read..... ok\n");

	return length;
}

#ifdef MODULE
int init_module(void)
{
	return key_init();			// 디바이스 및 인터럽트 등록함수를 호출함.
}

void cleanup_module(void)
{
	int ret;
	free_irq(IRQ_GPIO(0), NULL);		// 인터럽트 처리 함수 제거.

	ret = unregister_chrdev(Major, DEVICE_NAME);	// 커널이 이 디바이스를 
//사용하지 않도록
	if(ret < 0){
		printk("unregister_chrdev: error %d\n", ret);
	} else {
		printk("module clean up ok!!\n");
	}
}
#endif /* MODULE */

