include $(TOPDIR)/rules.mk

PKG_NAME:=tuya-espd
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/tuya-espd
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Tuya-ESP Bridge Daemon
	DEPENDS:=+libubus +libubox +libblobmsg-json +libtuya +libpthread +cJSON +espd
endef

define Package/tuya-espd/description
	Bridge daemon that receives commands from the Tuya IoT cloud
	and controls ESP micro controllers via the espd UBUS daemon.
	Supports: listing devices, turning GPIO pins on/off, reading sensors.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

CMAKE_OPTIONS += -DENABLE_DEBUG=OFF

define Package/tuya-espd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tuya_espd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/tuya-espd $(1)/etc/init.d/tuya-espd
endef

$(eval $(call BuildPackage,tuya-espd))
