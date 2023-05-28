$(eval $(kernel-module))
AESDCHAR_VERSION = '5df4487e992e954a9828ba8477d97737e0dd70cf'
AESDCHAR_SITE = 'git@github.com:cu-ecen-aeld/assignments-3-and-later-ECleverito.git'
AESDCHAR_SITE_METHOD = git
AESDCHAR_GIT_SUBMODULES = YES

AESDCHAR_MODULE_SUBDIRS = aesd-char-driver

define AESDCHAR_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar_load $(TARGET_DIR)/usr/bin
	$(INSTALL) -m 0755 $(@D)/aesd-char-driver/aesdchar_unload $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
