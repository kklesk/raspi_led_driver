ifneq ($(KERNELRELEASE),)
obj-m := rapivan_gpio.o

else
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif

clean:
	 rm ./*.o *.ko *mod.c *symvers *.order *.mod


