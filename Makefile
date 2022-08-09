default: all
##############################################################
include submodules/c_deps/etc/tools.mk

##############################################################
DIR=$(shell $(PWD))
PROJECT_DIR=$(DIR)
ETC_DIR=$(DIR)/etc
##############################################################
TIDIED_FILES = \
			   fsmon*/*.c fsmon*/*.h

##############################################################
include submodules/c_deps/etc/includes.mk
all: do-reset build test
do-reset:
	@reset
clean:
	@rm -rf build
test: do-clear do-test
do-clear:
	@clear
pull:
	@git pull
nodemon:
	$(PASSH) $(NODEMON) -I -i build \
		-w "submodules/c_deps/meson/deps/*/meson.build" \
		-w "*/*.c" \
		-w "*/*.h" \
		-w "fsmon*/*.c" \
		-w "fsmon-chan/*.c" \
		-w Makefile \
		-w meson.build \
		-w "*/meson.build" \
		-w meson_options.txt \
		-e build,c,h,Makefile,txt \
		-x env -- sh -c 'make||true'
dev: nodemon
