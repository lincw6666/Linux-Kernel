obj-m	:=	mtest.o
KVER	:=	$(shell uname -r)
KDIR	:=	/lib/modules/$(KVER)/build
PWD		:=	$(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
