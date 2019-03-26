obj-m	:=	1.o 2.o 3.o
KVER	:=	$(shell uname -r)
KDIR	:=	/lib/modules/$(KVER)/build
PWD		:=	$(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
