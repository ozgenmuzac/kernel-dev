# Set compiler optimization level
OPT = -O2
EXTRA_CFLAGS += $(OPT) -I$(LDDINC)

# Set final module name
TARGET = memcache

ifneq ($(KERNELRELEASE),)
# If kernel release flag is set,
# compiler will directly build and include object files
memcache-objs 	:= main.o
obj-m		:= memcache.o

else
# Otherwise, show compiler where to get builders
KERNELDIR 	?= /lib/modules/$(shell uname -r)/build
PWD       	:= $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD) modules

endif

install:
	install -d $(INSTALLDIR)
	install -c $(TARGET).o $(INSTALLDIR)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers modules.order

depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
