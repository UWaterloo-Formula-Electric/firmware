################################################################################
#
# libopenblt
#
################################################################################

LIBOPENBLT_VERSION = 1.3.4
LIBOPENBLT_SOURCE = openblt_v011000.tar.gz
LIBOPENBLT_SITE = https://github.com/feaser/openblt/archive
LIBOPENBLT_INSTALL_TARGET = YES
LIBOPENBLT_DEPENDENCIES = libusb libsocketcan

define LIBOPENBLT_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/build/libopenblt.so $(TARGET_DIR)/usr/lib
endef

$(eval $(cmake-package))

