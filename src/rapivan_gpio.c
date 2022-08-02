#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/atomic.h>
#include <asm/uaccess.h>

#define DRIVER_AUTHOR "klesk"
#define DRV_NAME "RASPILED"

#define GPIO_17 0x7E200014
#define GPIO_17_SIZE 0x0000004
#define LED_ON 1
#define LED_OFF 0

static dev_t led_dev_number;
static struct cdev* driver_object;
static struct class *led_class;
static struct device* led_dev;

// static atomic_t access_count = ATOMIC_INIT(-1);

// static int open_gpio(struct inode* device_file, struct file* instance) {
//     printk("open_led(): drv open");
//     dev_info(led_dev,"%s","open_gpio() called\n");
//     if(instance->f_flags&O_RDWR || instance->f_flags&O_WRONLY){
//         if(atomic_inc_and_test(&access_count))
//             return 0;
    
//         atomic_dec(&access_count);
//         return -EBUSY;
//     }
//     return 0;
// }

// static char test_string[]="Hello World\n";

// static ssize_t read_gpio(struct file* instance, char __user* userbuffer, size_t count, loff_t* offset) {
//     printk("read_led(): drv open");
//     dev_info(led_dev,"%s","read_gpio() called\n");

//     unsigned long not_copied,to_copy;
//     to_copy=min(count,strlen(test_string)+1);
//     not_copied=copy_to_user(userbuffer,test_string,to_copy);
//     return to_copy-not_copied;
//     dev_info(led_dev,"driver open called");

//     TODO Read GPIO PIN STATUS
//     unsigned long to_copy, not_copied;
//     to_copy=min(count)
// }
// static int write_gpio(struct inode* device_file, struct file* instance) {
//     printk("write_led(): drv open");

//     //TODO Write to GPIOPIN

//     return 0;
// }
// static int close_gpio(struct inode* device_file, struct file* instance) {
//     printk("close_led(): drv close");
//     dev_info(led_dev,"%s","close_gpio() called\n");

//     if(instance->f_flags&O_RDWR || instance->f_flags&O_WRONLY){
//         return 0;
//     }
//     atomic_dec(&access_count);

//     return 0;
//}

static struct file_operations file_ops = {
    .owner = THIS_MODULE,
    // .open = open_gpio,
    // .read = read_gpio,
    // // .write= write_gpio,
    // .release = close_gpio,
};

static int __init init_led(void){
    printk("init_led(): Hello Korld");
    //register char device number(s)
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-alloc-chrdev-region.html
    if(alloc_chrdev_region(&led_dev_number,0,1,DRV_NAME)<0)
        return -EIO;
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
    if( IS_ERR(led_class) )
        goto free_cdev;
    led_dev = device_create(led_class,NULL,led_dev_number,NULL,"%s","raspiled");

    free_device_number:
        //unregister device number
        //https://www.kernel.org/doc/htmldocs/kernel-api/API-unregister-chrdev-region.html
        unregister_chrdev_region(led_dev_number,1);
        return -EIO;
    free_cdev:
        //decrement refcount ( if refcount == 0 then kobject_cleanup)
        //https://linuxtv.org/downloads/v4l-dvb-internals/device-drivers/API-kobject-put.html
        kobject_put(&driver_object->kobj);

    return 0;
}

static void __exit exit_led(void){

    //delete sysfs entry in /dev/
    //https://docs.huihoo.com/doxygen/linux/kernel/3.7/base_2class_8c.html#ab65ab0ad8a63fb884c83f4eaee8874bc
    class_destroy(led_class);
    cdev_del(driver_object);
    unregister_chrdev_region(led_dev_number,1);
    printk("close_led(): byebye");
    return;
}


// do_initcalls( init_led );
//cleanup_module( exit_led );
//https://www.kernel.org/doc/html/v5.0/driver-api/basics.html
module_init(init_led);
module_exit(exit_led);
MODULE_LICENSE("GPL");
MODULE_AUTHOR( DRIVER_AUTHOR);