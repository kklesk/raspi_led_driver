#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/device.h>

#define DRIVER_AUTHOR "kleskk"

#define MODULENAME "RASPILED"

static dev_t led_dev_number;
static struct cdev* driver_object;
static struct class *led_class;
static struct device* led_dev;


static int open_led(struct inode* device_file, struct file* instance) {
    return 0;
}
// static int read_led(struct file* instance, char __user* userbuffer, size_t count, loff_t* offset) {
//     return 0;
// }
// static int write_led(struct inode* device_file, struct file* instance) {
//     return 0;
// }
static int close_led(struct inode* device_file, struct file* instance) {
    return 0;
}

static struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .open = open_led,
    // .read = read_led,
    // .write= write_led,
    .release = close_led,
};

static int __init init_led(void){
    printk("init_led(): Hello Korld");
    //register char device number(s)
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-alloc-chrdev-region.html
    if(alloc_chrdev_region(&led_dev_number,0,1,MODULENAME))
        return -EIO;
    //allocate cdev struct
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-alloc.html
    driver_object = cdev_alloc();
    if(driver_object == NULL)
        goto free_device_number;
    //unload protection
    driver_object->owner = THIS_MODULE;
    driver_object->ops = &file_ops;
    //register cdev driver object
    //https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-add.html
    if(cdev_add(driver_object,led_dev_number,1))
        goto free_cdev;
    //write entry sysfs for /dev/ entry
    //udev deamon will automatically load file into /dev/
    led_class = class_create(THIS_MODULE,MODULENAME);
    device_create(led_class,NULL,led_dev_number,NULL,"%s",MODULENAME);

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