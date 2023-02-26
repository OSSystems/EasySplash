prefix ?= /usr/local
sysconfdir ?= /etc
sbindir ?= $(prefix)/sbin
base_libdir ?= /lib

SRC = Makefile Cargo.lock Cargo.toml $(shell find src -type f -wholename '*src/*.rs')

BIN = easysplash
DEFAULT_ANIMATION ?= ossystems-demo

SYSTEMD ?= 0
ifeq ($(SYSTEMD),1)
	PKG_CONFIG ?= pkg-config
	systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd);
	ARGS += --features systemd
ifeq ($(strip $(systemdsystemunitdir)),)
	$(error Could not find systemd.pc file. Aborting))
endif
endif

INITD ?= 0

DEBUG ?= 0
TARGET = debug
ifeq ($(DEBUG),0)
	ARGS += "--release"
	TARGET = release
endif

BINARY=target/$(TARGET)/$(BIN)

.PHONY: all clean distclean install uninstall update

all: $(BINARY) install

clean:
	cargo clean

distclean:
	rm -rf .cargo target

install: install-binary install-service install-glowing-animation install-ossystems-animation

install-binary:
	@install -Dm04755 "$(BINARY)" "$(DESTDIR)$(sbindir)/$(BIN)"

install-service:
	@install -Dm0644 "etc/easysplash.default" "$(DESTDIR)$(sysconfdir)/default/easysplash"

	@if [ "$(SYSTEMD)" = "1" ]; then \
		install -Dm0644 "etc/easysplash-start.service.in" "$(DESTDIR)$(systemdsystemunitdir)/easysplash-start.service"; \
		install -Dm0644 "etc/easysplash-quit.service.in" "$(DESTDIR)$(systemdsystemunitdir)/easysplash-quit.service"; \
	fi

	@if [ "$(INITD)" = "1" ]; then \
		install -Dm0755 "etc/easysplash-start.init.in" "$(DESTDIR)$(sysconfdir)/init.d/easysplash-start"; \
	fi

	@for script in "$(DESTDIR)$(systemdsystemunitdir)/easysplash-start.service" \
					"$(DESTDIR)$(systemdsystemunitdir)/easysplash-quit.service" \
					"$(DESTDIR)$(sysconfdir)/init.d/easysplash-start"; do \
		if [ -e $$script ]; then \
			sed -e "s,@SYSCONFDIR@,$(sysconfdir),g" \
			    -e "s,@SBINDIR@,$(sbindir),g" \
			    -i $$script; \
		fi; \
	done

install-glowing-animation:
	@mkdir -p "$(DESTDIR)$(base_libdir)/easysplash/"
	@cp -r data/glowing-logo "$(DESTDIR)$(base_libdir)/easysplash/"
	@ln -sf "$(base_libdir)/easysplash/$(DEFAULT_ANIMATION)" "$(DESTDIR)$(base_libdir)/easysplash/animation"

install-ossystems-animation:
	@mkdir -p "$(DESTDIR)$(base_libdir)/easysplash/"
	@cp -r data/ossystems-demo "$(DESTDIR)$(base_libdir)/easysplash/"
	@ln -sf "$(base_libdir)/easysplash/$(DEFAULT_ANIMATION)" "$(DESTDIR)$(base_libdir)/easysplash/animation"

$(BINARY):
	cargo build $(ARGS)
