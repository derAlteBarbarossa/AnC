obj-m := cr3.o
cr3-objs := module.o
PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes
INCLUDE := ${KDIR}/include
CCFLAGS := -O3 -DMODULE -D__KERNEL__ ${WARN} ${INCLUDE}
KVERSION := $(shell uname -r)

all: clean
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	strip --strip-unneeded *.ko

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

test:
	# prints /proc/cr3 as hex
	echo "obase=16; `cat /proc/cr3`" | bc

install: all
	sudo rmmod cr3 | true
	make -C $(KDIR) M=$(PWD) modules_install
	sudo insmod cr3.ko
