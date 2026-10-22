obj-m += irqrec.o
irqrec-objs := starter/src/irqrec.o
BUILD_FIXTURE ?= 1
ifeq ($(BUILD_FIXTURE),1)
obj-m += irqrec_fixture.o
irqrec_fixture-objs := fixture/src/irqrec_fixture.o
endif

TOOLCHAIN_ROOT ?= /opt/embeddedville
KDIR ?= $(TOOLCHAIN_ROOT)/linux-6.18.57
PWD := $(shell pwd)
ccflags-y += -I$(PWD)/starter/include

.PHONY: all modules userspace clean
all: modules userspace

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) W=1 BUILD_FIXTURE=$(BUILD_FIXTURE) modules

userspace:
	mkdir -p build
	$(CC) -std=c11 -D_GNU_SOURCE -O2 -Wall -Wextra -Werror -static -Istarter/include userspace/irqrec-read.c -o build/irqrec-read
	$(CC) -std=c11 -D_GNU_SOURCE -O2 -Wall -Wextra -Werror -static -Istarter/include userspace/irqrec-stage-test.c -o build/irqrec-stage-test
	$(CC) -std=c11 -D_GNU_SOURCE -O2 -Wall -Wextra -Werror -static -Istarter/include userspace/irqrec-test.c -o build/irqrec-test

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean || true
	rm -rf build

