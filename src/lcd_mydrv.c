#include<linux/module.h>
#include<asm/hardware.h>
#include<asm/uaccess.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/errno.h>
#include<linux/types.h>
#include<asm/ioctl.h>
#include<linux/ioport.h>
#include<linux/delay.h>
#include<asm/io.h>
#include<linux/init.h>
#include "textlcd.h"

#define IOM_LCD_MAJOR		242
#define IOM_LCD_NAME		"text LCD driver"
#define IOM_LCD_ADDRESS_BASE	0x0C00001A
#define IOM_LCD_ADDRESS_RANGE	4

static void setcommand(unsigned short cmd);
static void setdata(unsigned short data);

int iom_lcd_open(struct inode *, struct file *);
int iom_lcd_release(struct inode *, struct file *);
ssize_t iom_lcd_write(struct file *, const char *, size_t, loff_t *);
int iom_lcd_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

static int txtlcd_usage = 0;
static int txtlcd_major = 0;
static char *txtlcd_base;
static volatile unsigned short *instr_addr;
static volatile unsigned short *data_addr;

static struct file_operations iom_lcd_fops = 
{
	open:		iom_lcd_open,
	write:		iom_lcd_write,
	ioctl:		iom_lcd_ioctl,
	release:	iom_lcd_release,
};

int iom_lcd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	unsigned long gdata)
{
	unsigned long command;

	copy_from_user(&command, (char*)gdata, 4);
	printk("ioctl %d %x\n", cmd, command);

	switch(cmd){
		case IOM_LCD_COMMAND:
			setcommand((unsigned short)command);
			break;
		default:
			printk("driver : no such command!\n");
			return -ENOTTY;
	}
	return 0;
}

static void initialize_textlcd()
{
	setcommand(CMD_FUNCTION_SET | F_DATA8 | F_LINE2 | F_FONT10);
	udelay(4000);
	setcommand(CMD_FUNCTION_SET | F_DATA8 | F_LINE2 | F_FONT10);
	udelay(100);
	setcommand(CMD_DISPLAY | D_ON | D_CURSOR);
	setcommand(CMD_ENTRY_MODE | E_CURSOR);
	setcommand(CMD_CLEAR_DISPLAY);
	setcommand(CMD_RETURN_HOME);
	udelay(100);
}

static void clear_textlcd()
{
	setcommand(CMD_CLEAR_DISPLAY);
	setcommand(CMD_RETURN_HOME);
	udelay(100);
}

static void setcommand(unsigned short cmd)
{
	*instr_addr = cmd;
	udelay(50);
}

static void setdata(unsigned short data)
{
	*data_addr = data;
	udelay(50);
}

int iom_lcd_open(struct inode *minode, struct file *mfile)
{
	if(txtlcd_usage != 0) return -EBUSY;
	
	MOD_INC_USE_COUNT;
	txtlcd_usage = 1;
	clear_textlcd();
	return 0;
}

int iom_lcd_release(struct inode *minode, struct file *mfile)
{
	MOD_DEC_USE_COUNT;
	txtlcd_usage = 0;
	return 0;
}

#define BUFSZ 32
unsigned char buf[BUFSZ];

ssize_t iom_lcd_write(struct file *inode, const char *gdata, size_t length, 
	loff_t *off_what)
{
	int i;
	
	if(length > BUFSZ) length = BUFSZ;
	copy_from_user(buf,gdata,length);
	printk("%s %d\n",buf,length);
	for(i=0; i<length; i++) setdata(buf[i]);
	return length;
}

int init_module(void)
{
	int result;
	result = register_chrdev(IOM_LCD_MAJOR,IOM_LCD_NAME,&iom_lcd_fops);
	
	if(result<0){
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}

	txtlcd_base = ioremap(IOM_LCD_ADDRESS_BASE,0x04);
	data_addr = (unsigned short *)txtlcd_base;
	instr_addr = (unsigned short *)(txtlcd_base+2);
	txtlcd_major = IOM_LCD_MAJOR;

	initialize_textlcd();

	printk("init module, %s major number %d\n",IOM_LCD_NAME,txtlcd_major);
	return 0;
}

void cleanup_module(void)
{
	iounmap(txtlcd_base);
	
	if(unregister_chrdev(txtlcd_major, IOM_LCD_NAME))
		printk(KERN_WARNING"%s DRIVER CLEANUP FALLED\n",IOM_LCD_NAME);
}
	
