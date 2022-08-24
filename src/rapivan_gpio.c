#include <linux/module.h>
#include <linux/cdev.h>
// for file_operations struct
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <asm/uaccess.h>
//for ioremap
#include <asm/io.h>

#define DRIVER_AUTHOR "klesk"
#define DRV_NAME "RASPILED"

#define GPIO_17_ADDRESS 0x7E200014
// #define GPIO_17_SIZE 0x0000004
#define GPIO_17_SIZE 4
//#define GPIO_17 0
#define LED_ON 1
#define LED_OFF 0

u32 GPIO_17 = 0;

static dev_t led_dev_number;
static struct cdev* driver_object;
static struct class *led_class;
static struct device* led_dev;

static atomic_t access_count = ATOMIC_INIT(0);

static int open_gpio(struct inode* device_file, struct file* instance) {
    //printk("open_led(): drv open");
    dev_info(led_dev,"%s","open_gpio() called\n");
    // if(instance->f_flags&O_RDWR || instance->f_flags&O_WRONLY){
    //     if(atomic_inc_and_test(&access_count))
    //         return 0;
    
    //     atomic_dec(&access_count);
    //     return -EBUSY;
    // }
    return 0;
}

// static char hello_from_read[] = "Hello from read_gpio()";
// static ssize_t read_gpio ( struct file* instance, char __user* user_buffer, size_t count, loff_t* offest ){
//     unsigned long to_copy, not_copied;
//     // to_copy = min(count, strlen(hello_from_read)+1);
//     // not_copied=copy_to_user(userbuffer,hello_from_read,to_copy);
//     to_copy = min(count, GPIO_17_SIZE);
//     not_copied = copy_to_user(user_buffer,GPIO_17_ADDRESS,GPIO_17_SIZE);
//     return to_copy-not_copied;
// }

static char test_string[]="Hello World\n";

static ssize_t read_gpio(struct file* instance, char __user* userbuffer, size_t count, loff_t* offset) {
    dev_info(led_dev,"%s","read_gpio() called\n");
    unsigned long not_copied,to_copy;

    unsigned char current_led_value = 0;


    readb(&GPIO_17);
    to_copy=min(count,strlen(test_string)+1); 
    not_copied=copy_to_user(userbuffer,current_led_value,to_copy);
    pr_info("LED17 is %c",current_led_value);


    return to_copy-not_copied;
}

static void set_led(int status ){
    if(status == 1){
        pr_info("set_led(): ON");
        writeb(LED_ON,&GPIO_17);
       // return 0;
    } else if ( status == 0){
        pr_info("set_led(): OFF");
        writeb(LED_OFF,&GPIO_17);

       // return 0;
    }
    pr_info("set_led(): unkown");
       // return -1;
}

static ssize_t write_gpio(struct file* instance, const char __user* user_buffer, size_t max_bytes_to_write, loff_t* offest ) {
    //pr_info("write_led()");
    char kernel_mem[128];
    ssize_t to_copy, not_copied;

    to_copy = min(strlen(user_buffer),max_bytes_to_write);
    not_copied = copy_from_user(kernel_mem,user_buffer,to_copy);
    if(*kernel_mem == '1'){
        set_led(1);
    }
    else if(*kernel_mem == '0')
        set_led(0);

    //dev_info(led_dev,"driver open called");
    return not_copied;
    //TODO Read GPIO PIN STATUS
}



static int close_gpio(struct inode* device_file, struct file* instance) {
    pr_info("close_led(): drv close");
   // dev_info(led_dev,"%s","close_gpio() called\n");
    if(instance->f_flags&O_RDWR || instance->f_flags&O_WRONLY){
        return 0;
    }
    atomic_dec(&access_count);
    return 0;
}

//
// ssize_t write_drv( struct file* instance, const char* __user userbuffer, 
//                     size_t buffer_size, loff_t* offs )
//

static struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .open = open_gpio,
    .read = read_gpio,
    .write= write_gpio,
    .release = close_gpio,
};

static int __init init_led(void){

    //TODO add io mem
    if(request_mem_region(GPIO_17_ADDRESS,GPIO_17_SIZE,DRV_NAME)==NULL)
        return -EBUSY;
    GPIO_17 = *(u32*) ioremap( GPIO_17_ADDRESS, GPIO_17_SIZE );
    pr_info("init_led(): Hello Korld");
    //register char device number(s)
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-alloc-chrdev-region.html
    pr_info("alloc_chrdev_region(&led_dev_number,0,1,DRV_NAME)");
    if(alloc_chrdev_region(&led_dev_number,0,1,DRV_NAME)<0)
        return -EIO;
    pr_info("Major Nr: %d\nMinor Nr: %d",led_dev_number>>20, led_dev_number && 0xffffff);
    //allocate cdev struct
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-alloc.html
    driver_object = cdev_alloc();
    if(driver_object == NULL)
        goto free_device_number;
    //activate unload protection
    //TODO Maybe similar to the next two linese
    // --> cdev_init(driver_object,&file_ops);
    driver_object->owner = THIS_MODULE;
    driver_object->ops = &file_ops;
    //register cdev driver object
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-add.html
    if(cdev_add(driver_object,led_dev_number,1)<0)
        goto free_cdev;
    //write entry sysfs for /dev/ entry
    //udev deamon will automatically load file into /dev/
    led_class = class_create(THIS_MODULE,DRV_NAME);
    // if( IS_ERR(led_class) )
    //     goto free_cdev;
    led_dev = device_create(led_class,NULL,led_dev_number,NULL,"%s","raspiled");

    return 0;
    if(IS_ERR( led_class))
        goto free_cdev;
    led_dev = device_create(led_class,NULL,led_dev_number,NULL,"%s",DRV_NAME);

    free_device_number:
        pr_info("free_device_number");  
        //unregister device number
        //https://www.kernel.org/doc/htmldocs/kernel-api/API-unregister-chrdev-region.html
        unregister_chrdev_region(led_dev_number,1);
        return -EIO;
    free_cdev:
        pr_info("free_cdev");
        //decrement refcount ( if refcount == 0 then kobject_cleanup)
        //https://linuxtv.org/downloads/v4l-dvb-internals/device-drivers/API-kobject-put.html
        kobject_put(&driver_object->kobj);

    return 0;
}

static void __exit exit_led(void){

    //delete sysfs entry in /dev/
    //https://docs.huihoo.com/doxygen/linux/kernel/3.7/base_2class_8c.html#ab65ab0ad8a63fb884c83f4eaee8874bc
    device_destroy(led_class,led_dev_number);
    class_destroy(led_class);
    cdev_del(driver_object);
    unregister_chrdev_region(led_dev_number,1);
    pr_info("close_led(): byebye");
    return;
}


// do_initcalls( init_led );
//cleanup_module( exit_led );
//https://www.kernel.org/doc/html/v5.0/driver-api/basics.html
module_init(init_led);
module_exit(exit_led);
MODULE_LICENSE("GPL");
MODULE_AUTHOR( DRIVER_AUTHOR);