prefix ?= /usr/local
sysconfdir ?= /etc
sbindir = $(prefix)/sbin
base_libdir = /lib

SRC = Makefile Cargo.lock Cargo.toml $(shell find src -type f -wholename '*src/*.rs')

BIN = easysplash
DEFAULT_ANIMATION ?= ossystems-demo

SYSTEMD ?= 0
ifeq ($(SYSTEMD),1)
	ARGS += --features systemd
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

install: install-binary install-service install-animations

install-binary:
	@install -Dm04755 "$(BINARY)" "$(DESTDIR)$(sbindir)/$(BIN)"

install-service:
	@install -Dm0644 "etc/easysplash.default" "$(DESTDIR)$(sysconfdir)/default/easysplash"

	@if [ "$(SYSTEMD)" = "1" ]; then \
		install -Dm0644 "etc/easysplash-start.service.in" "$(DESTDIR)$(base_libdir)/systemd/system/easysplash-start.service"; \
		install -Dm0644 "etc/easysplash-quit.service.in" "$(DESTDIR)$(base_libdir)/systemd/system/easysplash-quit.service"; \
	fi

	@if [ "$(INITD)" = "1" ]; then \
		install -Dm0755 "etc/easysplash-start.init.in" "$(DESTDIR)$(sysconfdir)/init.d/easysplash-start"; \
	fi

	@for script in "$(DESTDIR)$(base_libdir)/systemd/system/easysplash-start.service" \
					"$(DESTDIR)$(base_libdir)/systemd/system/easysplash-quit.service" \
					"$(DESTDIR)$(sysconfdir)/init.d/easysplash-start"; do \
		if [ -e $$script ]; then \
			sed -e "s,@SYSCONFDIR@,$(sysconfdir),g" \
			    -e "s,@SBINDIR@,$(sbindir),g" \
			    -i $$script; \
		fi; \
	done

install-animations:
	@mkdir -p "$(DESTDIR)$(base_libdir)/easysplash/"
	@cp -r data/glowing-logo "$(DESTDIR)$(base_libdir)/easysplash/"
	@cp -r data/ossystems-demo "$(DESTDIR)$(base_libdir)/easysplash/"
	@ln -s "$(base_libdir)/easysplash/$(DEFAULT_ANIMATION)" "$(DESTDIR)$(base_libdir)/easysplash/animation"

$(BINARY):
	cargo build $(ARGS)
