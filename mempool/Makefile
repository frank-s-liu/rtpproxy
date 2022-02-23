SRCS = mempoolWrap.c  mempool.c 
ODIR = ./mempoolobjs/
OBJS = $(addprefix $(ODIR),$(SRCS:.c=.o))  
DEPS = $(addprefix $(ODIR),$(SRCS:.c=.d)) 

MAJ ?= 1
MIN ?= 0

INCLUDE := -I./export
#DEFINE := -DMEMPOOL_DEBUG

# note -pthread in compling process can trigger GCC to define -D_REENTRANT
CFLAGS := -g -Wall -Werror=declaration-after-statement -fPIC -O2 -fno-omit-frame-pointer -fstack-protector -pthread $(INCLUDE) $(DEFINE)
#-Wl,--wrap,malloc

LDFLAGS := -Wl,--wrap,malloc -Wl,--wrap,free

ifdef MAJ
DES = libmemory.so.$(MAJ).$(MIN)
SONAME=libmemory.so.$(MAJ)
else
DES = libmemory.so 
SONAME=libmemory.so
endif
LINKNAME=libmemory.so


$(ODIR)$(DES):$(OBJS)
	gcc   -shared $(LDFLAGS)  -Wl,-soname,$(SONAME)  -o $@ $^
	cd ${ODIR};test -f $(LINKNAME) || ln -s $(DES) $(LINKNAME);cd -
	cd ${ODIR};test -f $(SONAME) || ln -s $(DES) $(SONAME);cd -
#	ar crv  ./mempoolobjs/libmem.a  $(OBJS) 
#	g++ $(CFLAGS) $(OBJS) ../util/objs/Thread.o  -o test

$(ODIR)%.o: %.c
	test -d $(ODIR) || mkdir -p $(ODIR)
	gcc -c $(CFLAGS) $< -o $@

#using '-', indicate that if make don't find the depentfile, don't care the errors,
#at begining, still don't produce the dependent file
ifneq ($(MAKECMDGOALS),clean) 
-include $(DEPS)
endif

$(ODIR)%.d:%.c
	test -d $(ODIR) || mkdir -p $(ODIR)
	@set -e; rm -f $@;
	gcc -MM  $(CFLAGS)  $< > $@.123456;
	sed 's,\($*\)\.o[ :], $(ODIR)\1.o $@ : ,g' < $@.123456 > $@;
	rm -f $@.123456


.PHONY: clean
clean:  
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f ./mempoolobjs/*.so.*
	rm -f ./mempoolobjs/*.o
	rm -rf ./mempoolobjs

