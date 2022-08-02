default: all
##############################################################
PASSH=$(shell command -v passh)
GIT=$(shell command -v git)
SED=$(shell command -v gsed||command -v sed)
NODEMON=$(shell command -v nodemon)
FZF=$(shell command -v fzf)
BLINE=$(shell command -v bline)
UNCRUSTIFY=$(shell command -v uncrustify)
PWD=$(shell command -v pwd)
FIND=$(shell command -v find)
EMBED_BINARY=$(shell command -v embed)
JQ_BIN=$(shell command -v jq)
##############################################################
DIR=$(shell $(PWD))
PROJECT_DIR=$(DIR)
ETC_DIR=$(DIR)/etc
##############################################################
TIDIED_FILES = \
			   fsmon*/*.c fsmon*/*.h

##############################################################
all: do-reset build test
do-reset:
	@reset
clean:
	@rm -rf build
test: do-clear do-test
do-clear:
	@clear
do-meson: 
	@eval cd . && {  meson build || { meson build --reconfigure || { meson build --wipe; } && meson build; }; }
do-build:
	@eval cd . && { ninja -C build; }
do-test:
	@eval cd . && { meson test -C build -v; }
build: do-meson do-build
uncrustify:
	@$(UNCRUSTIFY) -c etc/uncrustify.cfg --replace $(TIDIED_FILES) 
uncrustify-clean:
	@find  . -type f -name "*unc-back*"|xargs -I % unlink %
fix-dbg:
	@$(SED) 's|, % s);|, %s);|g' -i $(TIDIED_FILES)
	@$(SED) 's|, % lu);|, %lu);|g' -i $(TIDIED_FILES)
	@$(SED) 's|, % d);|, %d);|g' -i $(TIDIED_FILES)
	@$(SED) 's|, % zu);|, %zu);|g' -i $(TIDIED_FILES)
tidy: uncrustify uncrustify-clean fix-dbg 
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
meson-introspect-all:
	@meson introspect --all -i meson.build
meson-introspect-targets:
	@meson introspect --targets -i meson.build
meson-binaries:
	@meson introspect --targets  meson.build -i | jq 'map(select(.type == "executable").filename)|flatten|join("\n")' -Mrc|xargs -I % echo build/%
meson-binaries-loc:
	@make meson-binaries|xargs -I % echo %.c|sort -u|xargs Loc --files|bline -a bold:green -r yellow -R 1-6


run-binary:
	@make meson-binaries | fzf --reverse | xargs -I % sh -xc "./%"
meson-tests-list:
	@meson test -C build --list
meson-tests:
	@make meson-tests-list|fzf --reverse -m | xargs -I % env cmd="\
		meson test --num-processes 1 -C build -v --no-stdsplit --print-errorlogs \"%\"" \
			env bash -c '\
	passh "$$cmd" && \
	ansi -n --green --bold "OK" && \
	echo -n "> " && \
	ansi -n --yellow --italic "$$cmd" && \
	echo \
'
