EXTRA_CFLAGS += -O1 -Wno-unused-variable -Wno-unused-value -Wno-unused-label -Wno-unused-parameter -Wno-uninitialized
EXTRA_CFLAGS += -I$(src)/include  -Wno-unused -Wno-unused-function

CONFIG_BUILT_IN = $(CONFIG_REALTEK_USB_RTL8191SU)

export TOPDIR := $(src)

ifeq ($(CONFIG_BUILT_IN), y)
include $(src)/config
else
include $(TOPDIR)/config
endif

ifeq ($(CONFIG_RTL8711), y)
RTL871X = rtl8711
MODULE_NAME = 8711
endif

ifeq ($(CONFIG_RTL8712), y)

RTL871X = rtl8712

ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 8712s
endif
ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME = 8712u
endif

endif

ifeq ($(CONFIG_SDIO_HCI), y)

 
_OS_INTFS_FILES := os_intf/osdep_service.o \
                    os_intf/linux/os_intfs.o \
                    os_intf/osdep_sdio_intf.o \
		    		os_intf/linux/sdio_intf.o \

_HAL_INTFS_FILES := hal/$(RTL871X)/hal_init.o \
		    hal/$(RTL871X)/sdio_halinit.o \
		    hal/$(RTL871X)/sdio_ops.o \
		    hal/$(RTL871X)/sdio_ops_linux.o    	

endif


ifeq ($(CONFIG_USB_HCI), y)
 
ifeq ($(CONFIG_BUILT_IN), y)
$(shell cp $(src)/autoconf_$(RTL871X)_usb_linux.h $(src)/include/autoconf.h)
else
$(shell cp $(TOPDIR)/autoconf_$(RTL871X)_usb_linux.h $(TOPDIR)/include/autoconf.h)
endif
 
_OS_INTFS_FILES := os_intf/osdep_service.o \
				   os_intf/linux/os_intfs.o \
				   os_intf/linux/usb_intf.o \


_HAL_INTFS_FILES := hal/$(RTL871X)/hal_init.o \
					hal/$(RTL871X)/usb_ops.o \
					hal/$(RTL871X)/usb_ops_linux.o \
					hal/$(RTL871X)/usb_halinit.o \
		
endif


PWD := $(shell pwd)


ifeq ($(CONFIG_PLATFORM_I386_PC), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
SUBARCH := $(shell uname -m | sed -e s/i.86/i386/)
ARCH ?= $(SUBARCH)
CROSS_COMPILE ?=
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
INSTALL_PREFIX :=
endif

ifeq ($(CONFIG_PLATFORM_ARM_S3C), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
ARCH := arm
CROSS_COMPILE := arm-linux-
KVER  := 2.6.24.7_$(ARCH)
KSRC := /usr/src/kernels/linux-$(KVER)
endif

ifeq ($(CONFIG_PLATFORM_RTD2880B), y)
EXTRA_CFLAGS += -DCONFIG_BIG_ENDIAN -DCONFIG_PLATFORM_RTD2880B
ARCH:=
CROSS_COMPILE:=
KVER:=
KSRC:=
endif

ifeq ($(CONFIG_PLATFORM_MIPS_RMI), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
ARCH:=mips
CROSS_COMPILE:=mipsisa32r2-uclibc-
KVER:= 
KSRC:= /root/work/kernel_realtek
endif

ifeq ($(CONFIG_PLATFORM_RTK_DMP), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -DRTK_DMP_PLATFORM
ARCH:=mips
CROSS_COMPILE:=mipsel-linux-
KVER:= 
KSRC:= /root/Desktop/SVN/linux-2.6.12
endif

ifeq ($(CONFIG_PLATFORM_MIPS_PLM), y)
EXTRA_CFLAGS += -DCONFIG_BIG_ENDIAN
ARCH:=mips
CROSS_COMPILE:=mipsisa32r2-uclibc-
KVER:= 
KSRC:= /root/work/kernel_realtek
endif

ifeq ($(CONFIG_PLATFORM_MSTAR389), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -DCONFIG_PLATFORM_MSTAR389
ARCH:=mips
CROSS_COMPILE:= mips-linux-gnu-
KVER:= 2.6.28.10
KSRC:= /home/mstar/mstar_linux/2.6.28.9/
endif

ifneq ($(KERNELRELEASE),)

ifeq ($(CONFIG_BUILT_IN), y)
obj-y := $(MODULE_NAME).o
else
obj-m := $(MODULE_NAME).o
endif

$(MODULE_NAME)-y += cmd/rtl871x_cmd.o cmd/$(RTL871X)_cmd.o

$(MODULE_NAME)-y += crypto/rtl871x_security.o 
$(MODULE_NAME)-y += debug/rtl871x_debug.o 

$(MODULE_NAME)-y += eeprom/rtl871x_eeprom.o efuse/rtl8712_efuse.o

$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)

$(MODULE_NAME)-y += io/rtl871x_io.o \
			io/$(RTL871X)_io.o

$(MODULE_NAME)-y += ioctl/rtl871x_ioctl_query.o \
          			ioctl/rtl871x_ioctl_set.o \
	   				ioctl/rtl871x_ioctl_linux.o \
					ioctl/rtl871x_ioctl_rtl.o

$(MODULE_NAME)-y += led/rtl8712_led.o

$(MODULE_NAME)-y += mlme/ieee80211.o mlme/rtl871x_mlme.o				
$(MODULE_NAME)-$(CONFIG_MLME_EXT) += mlme/rtl871x_mlme_ext.o mlme/rtl871x_wlan_mlme.o mlme/rtl871x_wlan_sme.o

$(MODULE_NAME)-$(CONFIG_MP_INCLUDED) += mp/rtl871x_mp.o \
					mp/rtl871x_mp_ioctl.o

$(MODULE_NAME)-y += os_dep/linux/io_linux.o \
					os_dep/linux/xmit_linux.o \
					os_dep/linux/cmd_linux.o \
					os_dep/linux/mlme_linux.o \
					os_dep/linux/recv_linux.o

$(MODULE_NAME)-y += $(_OS_INTFS_FILES)

$(MODULE_NAME)-y += pwrctrl/rtl871x_pwrctrl.o

$(MODULE_NAME)-y += recv/rtl871x_recv.o recv/$(RTL871X)_recv.o

$(MODULE_NAME)-y += rf/rtl871x_rf.o rf/$(RTL871X)_rf.o
$(MODULE_NAME)-y += sta_mgt/rtl871x_sta_mgt.o

$(MODULE_NAME)-y += xmit/rtl871x_xmit.o xmit/$(RTL871X)_xmit.o

else

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD)  modules

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	/sbin/depmod -a ${KVER}

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KVER}
	
	
config_r:
	@echo "make config"
	/bin/bash script/Configure script/config.in
	
.PHONY: modules clean

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm .tmp_versions -fr ; rm Module.symvers -fr
	cd cmd ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd crypto ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd debug ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd eeprom ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd hal/$(RTL871X) ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd io ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd ioctl ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd led ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd mlme ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd mp ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd os_dep/linux ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd os_intf ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd os_intf/linux ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd pwrctrl ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd recv ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd rf ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd sta_mgt ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd xmit; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 
	cd efuse; rm -fr *.mod.c *.mod *.o .*.cmd *.ko 

endif

