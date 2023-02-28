obj-m := virt_mfd_core.o virt_mfd_subdev1.o virt_mfd_subdev2.o
SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c built-in.a *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
