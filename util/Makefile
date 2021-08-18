ODIR := ./utilobjs/

Cxx_SRCS := thread.cpp   lock.cpp  timer.cpp  timerThread.cpp 
Cxx_ODIR := ./utilobjs/cxxobjs/
Cxx_OBJS := $(addprefix $(Cxx_ODIR),$(Cxx_SRCS:.cpp=.o))  
Cxx_DEPS := $(addprefix $(Cxx_ODIR),$(Cxx_SRCS:.cpp=.d)) 


C_SRCS := queue.c freeLockQueue.c c_queue.c hashcode.c
C_ODIR := ./utilobjs/cobjs/
C_OBJS := $(addprefix $(C_ODIR),$(C_SRCS:.c=.o))  
C_DEPS := $(addprefix $(C_ODIR),$(C_SRCS:.c=.d)) 


INCLUDE_DIR := -I../export/logExport -I ./utilExport -I ../export/mempoolExport -I ../include 
CXXFLAGS := -g -fPIC -Wall -Werror -O2  $(INCLUDE_DIR)

LDFLAGS := -fvisibility=hidden

ifdef MAJ
DES=libutilself.so.$(MAJ).$(MIN)
SONAME=libutilself.so.$(MAJ)
else
DES=libutilself.so
SONAME=libutilself.so
endif
#$(ODIR)libutilself.a:$(OBJS)
#	ar crv ./utilobjs/libutilself.a $(OBJS)
#	readelf -d
$(ODIR)$(DES):$(Cxx_OBJS) $(C_OBJS)
	g++   -shared $(LDFLAGS)  -Wl,-soname,$(SONAME)  -o $@ $^
	cd ${ODIR};test -f libutilself.so || ln -s $(DES) libutilself.so;cd - 
	cd ${ODIR};test -f $(SONAME) || ln -s $(DES) $(SONAME);cd -

$(Cxx_ODIR)%o: %cpp
	test -d $(Cxx_ODIR) || mkdir -p $(Cxx_ODIR)
	g++ -c $(CXXFLAGS) $< -o $@



$(C_ODIR)%o: %c
	test -d $(C_ODIR) || mkdir -p $(C_ODIR)
	gcc -c $(CXXFLAGS) $< -o $@

#using '-', indicate that if make don't find the depentfile, don't care the errors,
#at begining, still don't produce the dependent file
include $(Cxx_DEPS)
include $(C_DEPS)

$(Cxx_ODIR)%d:%cpp
	test -d $(Cxx_ODIR) || mkdir -p $(Cxx_ODIR)
	@set -e; rm -f $@;
	g++ -MM $(CXXFLAGS) $< > $@.123456;
	sed 's,\($*\)\.o[ :]*, $(Cxx_ODIR)\1.o $@ : ,g' < $@.123456 > $@;
	rm -f $@.123456


$(C_ODIR)%.d:%.c
	test -d $(C_ODIR) || mkdir -p $(C_ODIR)
	@set -e; rm -f $@;
	g++ -MM $(CXXFLAGS) $< > $@.123456;
	sed 's,\($*\)\.o[ :]*, $(C_ODIR)\1.o $@ : ,g' < $@.123456 > $@;
	rm -f $@.123456


.PHONY: clean
clean:  
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f ./utilobjs/libutilself.a
	rm -f ./utilobjs/cobjs/*.o
	rm -f ./utilobjs/cobjs/*.d
	rm -f ./utilobjs/cxxobjs/*.o
	rm -f ./utilobjs/cxxobjs/*.d
	rm -f ./utilobjs/*.so.*
	rm -f ./utilobjs/*.so
	rm -f ../libs/libutilself.so*
	rm -rf ./utilobjs/cxxobjs
	rm -rf ./utilobjs/cobjs
	rm -rf ./utilobjs

