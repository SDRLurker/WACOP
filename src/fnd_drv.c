/* fnd_mydrv.h */

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <linux/timer.h>

#define IOM_FND_MAJOR	241		// ioboard fnd device major #
#define IOM_FND_NAME	"FNDS"		// ioboard fnd device name
#define IOM_FND_ADDRESS	0x0C000004	// physical address of fnd device
#define port16(addr)	(*(volatile unsigned short *)addr)
// for 16 bit read and write

int iom_fnd_open(struct inode *, struct file *);
int iom_fnd_release(struct inode *, struct file *);
ssize_t iom_fnd_write(struct file *, const char *, size_t, loff_t *);
int iom_fnd_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

char Getsegcode(int x);
void iom_fnd_display(const char *digit);

static int fnd_usage = 0;
static int fnd_major = 0;
static unsigned short *iom_fnd_addr;
// The address being get by kernel of fnd device

static int leadingzero = 0;
static int blinking = 0;
static int dp = 0;

static char digit[6];
static const char zeros[6] = { 0, 0, 0, 0, 0, 0 };

static struct file_operations iom_fnd_fops = 
{
	open:		iom_fnd_open,
	write:		iom_fnd_write,
	release:	iom_fnd_release,
	ioctl:		iom_fnd_ioctl,
};

static unsigned short mandiv = 0; // man bun eui il cho =_=;
static void time_handler(int signum);

int iom_fnd_open(struct inode *minode, struct file *mfile)
{
	if(fnd_usage != 0) return -EBUSY; // accept for only 1 use
	MOD_INC_USE_COUNT; // use_count++;
	fnd_usage = 1; // I am using fnd device...
	
	return 0;
}

int iom_fnd_release(struct inode *minode, struct file *mfile)
{
	MOD_DEC_USE_COUNT; // use_count--;
	fnd_usage = 0; // I have finished using fnd device... =_=a
	//outw(0xff, iom_fnd_addr); 
	port16(iom_fnd_addr) = 0xff; // turn off fnd device...
	return 0;
}

static char segcode[16] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 
	0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71
}; // The array for one fnd display...

char Getsegcode(int x) // get the value of one fnd display
{
	if(x >= 0 && x < 16)
		return segcode[x];
	else
		return 0;
}

ssize_t iom_fnd_write(struct file *inode, const char *gdata, size_t length,
	loff_t *off_what)
{
	int i;
	unsigned long data;
	int dpcopy = dp;

	copy_from_user(&data, gdata, 4);
	//copy The gdata value from user to the data variable
	for(i=0;i<6;i++){
		digit[i] = Getsegcode(data % 10);
		if(dpcopy&0x01) digit[i] = digit[i]|0x80;
		else digit[i] = digit[i]&0x7f;
		dpcopy>>=1;
		data/=10;
		if(leadingzero==0 && data==0) break;
	} // if leadingzero==1 then you can see '0' fnd display
	for(i++;i<6;i++){
		if(dpcopy&0x01) digit[i] = digit[i]|0x80;
		else digit[i] = digit[i]&0x7f;
		dpcopy>>=1;
		digit[i]&=0x80; 
	}
	// execute this sentence when leadingzero==0
	
	//if(blinking==0){ for(i=0;i<100000;i++) iom_fnd_display(digit); }
	//if(blinking==1){
	//	for(i=0;i<50000;i++) iom_fnd_display(zeros);
	//	for(i=0;i<50000;i++) iom_fnd_display(digit);
	//}
	time_handler(0);
	
	return length;
}

static void time_handler(int signum)
{
	int i;
	if(fnd_usage){
		mandiv++;
		if(mandiv>10000) mandiv=0;
		if(blinking==0) for(i=0;i<10;i++)iom_fnd_display(digit);
		else{
			for(i=0;i<10;i++){
				if(mandiv<5000) iom_fnd_display(digit);
				if(mandiv>=5000) iom_fnd_display(zeros);
			}
		}
		//udelay(1000);
	}
}

static char digitselect[6] = {0xdf, 0xef, 0xf7, 0xfb, 0xfd, 0xfe};

void iom_fnd_display(const char *digit)
{
	int i;
	unsigned short tmp;

	for(i=0;i<6;i++){
		tmp = (digit[i]<<8) | digitselect[i]; 
		 outw(tmp, iom_fnd_addr);
		//port16(iom_fnd_addr) = tmp;
		// udelay(1000);
	}
}

int iom_fnd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, 
	unsigned long arg)
{
	if((cmd&0x01)==0x01) leadingzero = 1;
	else leadingzero = 0;
	if((cmd&0x02)==0x02) blinking = 1;
	else blinking = 0;
	//printk("blinking : %d",blinking);
	dp = 0;
	dp = (cmd>>2)&0x3F;
	return 0;
}

int init_module(void) // call this function when execute insmod program...
{
	int result;
	
	result = register_chrdev(IOM_FND_MAJOR,IOM_FND_NAME,&iom_fnd_fops);
	if(result < 0){
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}
	fnd_major = IOM_FND_MAJOR;
	iom_fnd_addr = ioremap(IOM_FND_ADDRESS,0x02);
	
	printk("init module, %s major number : %d\n", IOM_FND_NAME, fnd_major);
	return 0;
}

void cleanup_module(void)
{
	iounmap(iom_fnd_addr);
	if(unregister_chrdev(fnd_major,IOM_FND_NAME))
		printk(KERN_WARNING"%s DRIVER CLEANUP FALLED\n",IOM_FND_NAME);
}

