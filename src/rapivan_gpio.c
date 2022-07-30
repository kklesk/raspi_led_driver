#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>


#define MODULENAME "RASPYLED"

static dev_t led_dev_number;
static struct cdev* driver_object;
static struct class led_class;
static struct device* led_dev;


static struct file_op{
    .owner = THIS_MODULE;
    .open = open_led;
    .read = read_led;
    .release = close_led;
}

static int __init init_led(void){
//TODO

}

static int read_led

static void __exit close_led(void){

}



module_init( init_led );
module_exit( close_led );
MODULE_LICENCE("GPL");
MODULE_AUTOR("klesk");