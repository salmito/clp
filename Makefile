LUA_VER=5.1

PREFIX=/usr/local

INSTALL_LIBDIR=$(PREFIX)/lib/lua/$(LUA_VER)
INSTALL_SHAREDIR=$(PREFIX)/share/lua/$(LUA_VER)

SRC_DIR=src/
MODULE=clp
_SO=.so

BIN=$(MODULE)$(_SO)
all:
	cd $(SRC_DIR) && make LUA_VER=$(LUA_VER) all 

install: all
	mkdir -p $(INSTALL_LIBDIR)
	install $(SRC_DIR)/$(BIN) $(INSTALL_LIBDIR)

install-both: clean
	cd $(SRC_DIR) && make LUA_VER=5.1 all && cd - && make LUA_VER=5.1 install
	make clean
	cd $(SRC_DIR) && make LUA_VER=5.2 all && cd - && make LUA_VER=5.2 install
	make clean
	cd $(SRC_DIR) && make LUA_VER=5.3 all && cd - && make LUA_VER=5.3 install

uninstall:
	rm -f $(INSTALL_LIBDIR)/$(BIN)
	
uninstall-both:
	make LUA_VER=5.1 uninstall && make LUA_VER=5.2 uninstall && make LUA_VER=5.3 uninstall

%:
	cd $(SRC_DIR) && make $@

ultraclean:
	cd $(SRC_DIR) && make ultraclean
	rm -f `find -iname *~`

tar tgz: ultraclean
ifeq "$(VERSION)" ""
	echo "Usage: make tar VERSION=x"; false
else
	rm -rf $(MODULE)-$(VERSION)
	mkdir $(MODULE)-$(VERSION)
	tar c * --exclude="*.tar.gz" --exclude=".git" --exclude="$(MODULE)-$(VERSION)*" | (cd $(MODULE)-$(VERSION) && tar x)
	tar czvf $(MODULE)-$(VERSION).tar.gz $(MODULE)-$(VERSION)
	rm -rf $(MODULE)-$(VERSION)
	md5sum $(MODULE)-$(VERSION).tar.gz > $(MODULE)-$(VERSION).md5
endif

install-mod: 
	mkdir -p $(INSTALL_SHAREDIR)/clp/
	cp -r clp $(INSTALL_SHAREDIR)/clp/
	
