#
# Copyright (C) 2010-2012 OpenWrt.org
#
# This Makefile and the code shipped in src/ is free software, licensed
# under the GNU Lesser General Public License, version 2.1 and later.
# See src/COPYING for more information.
#
# Refer to src/COPYRIGHT for copyright statements on the source files.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=SpeedTestC
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/SpeedTestC
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=idianjia network speed test util
  URL:=http://www.speedtest.net/
endef

define Package/SpeedTestC/description
	Client for SpeedTest.net infrastructure written in pure C99 standard using only POSIX libraries.
	Main purpose for this project was to deliver client for Speedtest.net infrastructure for embedded devices.
	All application is written in pure C99 standard (it should compile with C99 too), without using any external libraries - only POSIX is used.
	Code is not perfect, and it has a few bugs, but it is stable and it works.
	Big thanks for Luke Graham for his http function.
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/SpeedTestC/install
	$(INSTALL_DIR) $(1)/bin
	$(CP) $(PKG_BUILD_DIR)/SpeedTestC $(1)/bin
endef

$(eval $(call BuildPackage,SpeedTestC))
