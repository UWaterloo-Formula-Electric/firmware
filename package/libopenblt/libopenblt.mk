################################################################################
#
# libopenblt
#
################################################################################

LIBOPENBLT_VERSION = 1.3.4
LIBOPENBLT_SOURCE = openblt_v011000.tar.gz
LIBOPENBLT_SITE = https://github.com/feaser/openblt/archive
LIBOPENBLT_DEPENDENCIES = libusb libsocketcan

$(eval $(cmake-package))

