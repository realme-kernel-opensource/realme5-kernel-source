# SPDX-License-Identifier: GPL-2.0
# ifndef VENDOR_EDIT Yunqing.Zeng@BSP.Kernel.Driver 2019/03/15 Modify for use oppo target dts for kernel
#dts-dirs += actions
#dts-dirs += al
#dts-dirs += allwinner
#dts-dirs += altera
#dts-dirs += amd
#dts-dirs += amlogic
#dts-dirs += apm
#dts-dirs += arm
#dts-dirs += broadcom
#dts-dirs += cavium
#dts-dirs += exynos
#dts-dirs += freescale
#dts-dirs += hisilicon
#dts-dirs += marvell
#dts-dirs += mediatek
#dts-dirs += nvidia
#dts-dirs += qcom
#dts-dirs += realtek
#dts-dirs += renesas
#dts-dirs += rockchip
#dts-dirs += socionext
#dts-dirs += sprd
#dts-dirs += xilinx
#dts-dirs += lg
#dts-dirs += zte
# else
# ifndef VENDOR_EDIT Hanqing.Wang@BSP.Kernel.Driver 2019/03/23 Modify for use 19631 target dts for kernel
dts-dirs += 19631
dts-dirs += 19632
dts-dirs += 19633
# endif Hanqing.Wang@BSP.Kernel.Driver 2019/03/23 Modify for use 19631 target dts for kernel
# endif

subdir-y	:= $(dts-dirs)

dtstree		:= $(srctree)/$(src)

dtb-$(CONFIG_OF_ALL_DTBS) := $(patsubst $(dtstree)/%.dts,%.dtb, $(foreach d,$(dts-dirs), $(wildcard $(dtstree)/$(d)/*.dts)))

always		:= $(dtb-y)

targets += dtbs

DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif
targets += $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := dts/*.dtb *.dtb
