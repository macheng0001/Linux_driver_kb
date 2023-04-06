obj-m := virt-pinctrl.o pinctrl_consumer.o virt_gpiochip.o
SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c built-in.a *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
