KERNELDIR := /home/hjy/linux/imx6ull/kernel/nxp/linux-imx-rel_imx_4.1.15_2.1.0_ga
CURRENT_PATH := $(shell pwd)
CROSS_COMPILE := arm-linux-gnueabihf-
ARCH := arm

obj-m := platform_led_no_dts.o platform_led_driver_no_dts.o
 
all: build

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
