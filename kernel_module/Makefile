obj-m += calc_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	insmod calc_module.ko

unload:
	rmmod calc_module

check:
	lsmod | grep calc