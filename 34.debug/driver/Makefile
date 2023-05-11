obj-m := print.o proc.o oops.o
SRC := $(shell pwd)
#CFLAGS_print.o := -DDEBUG
ccflags-y := -DDEBUG -g

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) 

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c built-in.a *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
