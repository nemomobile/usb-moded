require usb-moded.inc

SUMMARY = "Usb-moded: usb gadget driver handler and modesetting "
HOMEPAGE = "https://github.com/nemomobile/usb-moded"
SECTION = "libs/network"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://LICENSE;md5=5f30f0716dfdd0d91eb439ebec522ec2"

# 0.80.7 tag
SRCREV = "033c3b10197f3a6fc5ddf82ecf226b6d5c1bb208" 
SRC_URI = "git://github.com/nemomobile/usb-moded.git;protocol=https"

S = "${WORKDIR}/git"

inherit autotools-brokensep
