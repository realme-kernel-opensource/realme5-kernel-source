ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
	dtbo-$(CONFIG_ARCH_TRINKET) += trinket-idp-overlay.dtbo
	trinket-idp-overlay.dtbo-base := trinket.dtb
else
	dtb-$(CONFIG_ARCH_TRINKET)	+= trinket-idp.dtb
endif
always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
