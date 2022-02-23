ODIR := ./timerEventObjs/

Cxx_SRCS := timerMeter.cpp timerProcess.cpp task.cpp
Cxx_ODIR := $(ODIR)cxxobjs/
Cxx_OBJS := $(addprefix $(Cxx_ODIR),$(Cxx_SRCS:.cpp=.o))  
Cxx_DEPS := $(addprefix $(Cxx_ODIR),$(Cxx_SRCS:.cpp=.d)) 


C_SRCS := 
C_ODIR := ${ODIR}cobjs/
C_OBJS := $(addprefix $(C_ODIR),$(C_SRCS:.c=.o))  
C_DEPS := $(addprefix $(C_ODIR),$(C_SRCS:.c=.d)) 


INCLUDE_DIR := -I../export/logExport -I ../export/utilExport -I ../export/mempoolExport -I ./include -I ./timerEventExport
CXXFLAGS := -g -fPIC -Wall -Werror -fno-omit-frame-pointer -fstack-protector  $(INCLUDE_DIR)
CFLAGS := -g -fPIC -Wall -Werror -fno-omit-frame-pointer -fstack-protector $(INCLUDE_DIR)

LDFLAGS := -fvisibility=hidden
# -llog -L../log/logobjs

ifdef MAJ
DES = libtimerEvent.so.$(MAJ).$(MIN)
SONAME=libtimerEvent.so.$(MAJ)
else
DES = libtimerEvent.so
SONAME=libtimerEvent.so
endif
LINKNAME=libtimerEvent.so

$(ODIR)$(DES):$(Cxx_OBJS) $(C_OBJS)
	gcc   -shared $(LDFLAGS)  -Wl,-soname,$(SONAME)  -o $@ $^
	cd ${ODIR};test -f $(LINKNAME) || ln -s $(DES) $(LINKNAME);cd -
	cd ${ODIR};test -f $(SONAME) || ln -s $(DES) $(SONAME);cd -

$(Cxx_ODIR)%o: %cpp
	test -d $(Cxx_ODIR) || mkdir -p $(Cxx_ODIR)
	g++ -c $(CXXFLAGS) $< -o $@



$(C_ODIR)%o: %c
	test -d $(C_ODIR) || mkdir -p $(C_ODIR)
	gcc -c $(CFLAGS) $< -o $@

#using '-', indicate that if make don't find the depentfile, don't care the errors,
#at begining, still don't produce the dependent file
ifneq ($(MAKECMDGOALS),clean)
include $(Cxx_DEPS)
include $(C_DEPS)
endif

$(Cxx_ODIR)%d:%cpp
	test -d $(Cxx_ODIR) || mkdir -p $(Cxx_ODIR)
	@set -e; rm -f $@;
	g++ -MM $(CXXFLAGS) $< > $@.123456;
	sed 's,\($*\)o[ :], $(Cxx_ODIR)\1o $@ : ,g' < $@.123456 > $@;
	rm -f $@.123456


$(C_ODIR)%.d:%.c
	test -d $(C_ODIR) || mkdir -p $(C_ODIR)
	@set -e; rm -f $@;
	gcc -MM $(CFLAGS) $< > $@.123456;
	sed 's,\($*\)o[ :], $(C_ODIR)\1o $@ : ,g' < $@.123456 > $@;
	rm -f $@.123456


.PHONY: clean
clean:  
	rm -rf $(ODIR)
