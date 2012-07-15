all:
	@./configure --host=mipsel-linux
	@mv GNUmakefile tmpGNUmakefile
	@make
	@cp libexif/.libs/libexif.a ../../../lib/
	@mv tmpGNUmakefile GNUmakefile
clean:
	@mv GNUmakefile tmpGNUmakefile
	@if [ -r Makefile ];then make clean;fi
	@mv tmpGNUmakefile GNUmakefile
distclean:
	@mv GNUmakefile tmpGNUmakefile
	@if [ -r Makefile ];then make distclean;fi
	@mv tmpGNUmakefile GNUmakefile
