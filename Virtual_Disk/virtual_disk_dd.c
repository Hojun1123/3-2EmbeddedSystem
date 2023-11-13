#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>      //kmelloc
#include <linux/uaccess.h>   //user access
#include <linux/kfifo.h>

#define DEV_NAME "VirtualDisk"
#define DEV_MAJOR 260

#define MY_IOCTL_NUMBER    100
#define IOCTL_READ_BUFFER    _IOW(MY_IOCTL_NUMBER, 0, int)
#define IOCTL_WRITE_BUFFER   _IOW(MY_IOCTL_NUMBER, 1, int)
#define IOCTL_BUF_INFORMATION   _IOW(MY_IOCTL_NUMBER, 2, int)

static int N = 32;    //kfifo size
static int M = 4;    //read size

/* define param and description */
module_param(N, int, 0000);
MODULE_PARM_DESC(N, "N 32");
module_param(M, int, 0000);
MODULE_PARM_DESC(M, "M 4");

struct kfifo fifo;

int VirtualDisk_open(struct inode *inode, struct file *filep);
int VirtualDisk_release(struct inode *inode, struct file *filep);
ssize_t VirtualDisk_write(struct file *filep, const char *buf, size_t count, loff_t *f_ops);
ssize_t VirtualDisk_read(struct file *filep, char *buf, size_t count, loff_t * f_ops);
long VirtualDisk_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);
int change_N(int new_buffer_size);
void change_M(int new_buffer_size);
void print_Buffer_Information(void);

int VirtualDisk_open(struct inode *inode, struct file *filep)
{
	printk("[Virtual Disk] Virtual Disk Open!!!\n");
	return 0;
}

int VirtualDisk_release(struct inode *inode, struct file *filep)
{
	printk("[Virtual Disk] Virtual Disk Release!!!\n");
	return 0;
}

ssize_t VirtualDisk_write(struct file *filep, const char * buf, size_t count, loff_t *f_ops)
{
	//write
	char temp, temp2;
	int i, err;
	char *write_buffer;

	write_buffer = kmalloc(count, GFP_KERNEL);
	//memory allocation exception
	if(write_buffer==NULL)
		return -1;

	//buf >> write_buffer
	err = copy_from_user(write_buffer, buf, count);
	if(err){
		printk(KERN_ERR "can't read write_buffer\n");
		kfree(write_buffer);
		return -1;
	}

	for(i=0; i<count; i++){
		temp = write_buffer[i];
		//delete enter
		if(temp == '\n')
			continue;
		if(kfifo_len(&fifo) >= N){
			kfifo_out(&fifo, &temp2, sizeof(char));
			printk("[Virtual Disk] delete %c on the front\n", temp2);
		}
		kfifo_in(&fifo, &temp, sizeof(char));
		printk("[Virtual Disk] add %c on the kfifo\n", temp);
	}
	printk("[Virtual Disk] kfifo length is %d\n", kfifo_len(&fifo));
	kfree(write_buffer);
	return count;
}


ssize_t VirtualDisk_read(struct file *filep, char *buf, size_t count, loff_t * f_ops)
{
	//read
	char temp;
	int i, err;
	char *read_buffer;

	//already read buffer
	if(*f_ops > 0)
		return 0;

	read_buffer = kmalloc(M+1, GFP_KERNEL);

	//memory allocation exception
	if(read_buffer==NULL)
		return -1;

	for(i=0; i<M; i++){
		if(kfifo_is_empty(&fifo)){
			printk("[Virtual Disk] empty queue!!!\n");
			break;
		}
		kfifo_out(&fifo, &temp, sizeof(char));
		read_buffer[i] = temp;
	}

	//empty buffer
	if(i==0){
		kfree(read_buffer);
		return 0;
	}


	//add enter
	read_buffer[i] = '\n';
	err = copy_to_user(buf, read_buffer, sizeof(char)*(i+1));
	if(err){
		printk(KERN_ERR "can't read read_buffer\n");
		kfree(read_buffer);
		return -1;
	}

	kfree(read_buffer);
	*f_ops += i+1;

	return i+1;
}

long VirtualDisk_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int new_buffer_size;
	printk(KERN_INFO "[Virtual Disk] size change\n");

	switch( cmd )   {
		case IOCTL_READ_BUFFER:
			printk(KERN_ALERT "[Virtual Disk] Read Buffer size : M\n");
			copy_from_user(&new_buffer_size, (const void*)arg, sizeof(int));
			change_M(new_buffer_size);
			break;

		case IOCTL_WRITE_BUFFER:
			printk(KERN_ALERT "[Virtual Disk] Write Buffer size : N\n");
			copy_from_user(&new_buffer_size, (const void*)arg, sizeof(int));
			change_N(new_buffer_size);
			break;

		case IOCTL_BUF_INFORMATION:
			print_Buffer_Information();
			break;

		default:
			printk(KERN_ERR "[Virtual Disk]IOCTL Message: Unknown command\n");
			break;
	}
	return 0;
}

void change_M(int new_buffer_size)
{
	M = new_buffer_size;
	printk("[Virtual Disk] success change read buffer size !\n");
}

int change_N(int new_buffer_size)
{
	int i, len;
	char *temp;
	char temp2;
	N = new_buffer_size;	
	len = kfifo_len(&fifo);

	temp = kmalloc(N, GFP_KERNEL);
	//memory allocation exception
	if(temp==NULL)
		return -1;

	//save data   
	kfifo_out(&fifo, temp, sizeof(char)*len);

	if(N >= len){
		printk("[Virtual Disk] when N is smaller than new_N\n");

		kfifo_free(&fifo);
		//new buffer
		if(kfifo_alloc(&fifo, N, GFP_KERNEL)){
			printk(KERN_ERR "error kfifo alloc\n");
			return -1;
		}
		kfifo_in(&fifo, temp, sizeof(char)*len); 
	}

	//new_buffer_size < fifo
	else{
		printk("[Virtual Disk] when N is bigger than new_N\n");
		//free & re_allocation
		kfifo_free(&fifo);
		if(kfifo_alloc(&fifo, N, GFP_KERNEL)){
        	        printk(KERN_ERR "error kfifo alloc\n");
			return -1;
	        }

		for(i=len-N; i<len; i++){
			temp2 = temp[i];
			kfifo_in(&fifo, &temp2, sizeof(char));
		}
	}
	
	kfree(temp);
	printk("[Virtual Disk] success change write buffer size !\n");
	return 0;
}

void print_Buffer_Information(void) 
{
	char *temp;
	int len = kfifo_len(&fifo);
	temp = kmalloc(sizeof(char)*len+1, GFP_KERNEL);
	//memory allocation exception
	if(temp == NULL)
		return;
	kfifo_out_peek(&fifo, temp, sizeof(char)*len);
	temp[len] = '\0';
	printk(KERN_INFO "[Virtual Disk] kfifo Information: \n");
	printk(KERN_INFO "kfifo size:\t %d\n", kfifo_size(&fifo));
	printk(KERN_INFO "kfifo len:\t %d\n", len);
	printk(KERN_INFO "kfifo avail:\t %d\n", kfifo_avail(&fifo));
	printk(KERN_INFO "M:\t\t %d\n", M);
	printk(KERN_INFO "N:\t\t %d\n", N);
	printk(KERN_INFO "kfifo:\t\t %s\n", temp);
	printk(KERN_INFO "[Virtual Disk] End infomation\n");
	kfree(temp);
}

struct file_operations VirtualDisk_fops = {
	.owner = THIS_MODULE,
	.open = VirtualDisk_open,
	.release = VirtualDisk_release,
	.write = VirtualDisk_write,
	.read = VirtualDisk_read,
	.unlocked_ioctl = VirtualDisk_ioctl,
};

int VirtualDisk_init(void)
{
	int ret = kfifo_alloc(&fifo, N, GFP_KERNEL);
	register_chrdev(DEV_MAJOR, DEV_NAME, &VirtualDisk_fops);
	if(ret){
		printk(KERN_ERR "error kfifo alloc\n");
	}
	printk(KERN_INFO "VirtualDisk(201810761) device driver registered\n");
	return 0;
}

void VirtualDisk_exit(void)
{
	kfifo_free(&fifo);
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
	printk(KERN_INFO "VirtualDisk(201810761) device driver unregistered\n");
}

module_init(VirtualDisk_init);
module_exit(VirtualDisk_exit);
MODULE_LICENSE("GPL");