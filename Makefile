ODIR := ./objs/

Cxx_SRCS := main.cpp
Cxx_ODIR := ./objs/cxxobjs/
Cxx_OBJS := $(addprefix $(Cxx_ODIR),$(Cxx_SRCS:.cpp=.o))  
Cxx_DEPS := $(addprefix $(Cxx_ODIR),$(Cxx_SRCS:.cpp=.d)) 


C_SRCS := 
C_ODIR := ./objs/cobjs/
C_OBJS := $(addprefix $(C_ODIR),$(C_SRCS:.c=.o))  
C_DEPS := $(addprefix $(C_ODIR),$(C_SRCS:.c=.d)) 

TARGET=rtpproxy

MAJ ?= 2
MIN ?= 0
export MAJ MIN

INCLUDE_DIR := -I./export/logExport -I ./export/utilExport -I ./export/mempoolExport -I ./tinyxml/export -I ./export/timerEventExport -I ./export/rtpExport
CXXFLAGS := -g -fPIC -Wall -Werror -fno-omit-frame-pointer -fstack-protector  $(INCLUDE_DIR)
CFLAGS := -g -fPIC -Wall -Werror -fno-omit-frame-pointer -fstack-protector $(INCLUDE_DIR)

LDFLAGS :=  -llog -lmemory -lutilself -ltinyxml -pthread -lssl -lcrypto  -ltimerEvent  -lrtpp  -L./mempool/mempoolobjs -L./log/logobjs -L./tinyxml/libs -L./linklibs -L./timerEvent/timerEventObjs  -L ./rtp/rtp_objs -L./util/utilobjs  -Wl,-rpath=../libs -Wl,-rpath-link=../libs

$(TARGET): ALL
	cd ./mempool; make || exit "$$"; cd -
	cd ./log; make || exit "$$"; cd -
	cd ./util; make || exit "$$"; cd -
	cd ./timerEvent; make || exit "$$"; cd -
	cd ./rtp; make || exit "$$"; cd -
	test -d ./bin || mkdir ./bin
	g++ $(LDFLAGS) $(Cxx_OBJS) $(C_OBJS)     -o ./bin/$@

ALL: $(Cxx_OBJS) $(C_OBJS)


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



.PHONY: install
install:
	rm -rf ./libs
	install -d ./libs
	install ./mempool/mempoolobjs/libmemory.so* ./libs
	install ./util/utilobjs/libutilself.so* ./libs
	install ./log/logobjs/liblog.so* ./libs
	install ./tinyxml/libs/libtinyxml.so*  ./libs
	install ./timerEvent/timerEventObjs/lib*.so* ./libs
	install ./linklibs/lib*.so*  ./libs
	install ./rtp/rtp_objs/lib*.so*  ./libs
	mkdir -p rtpproxy
	cp -rf bin libs conf rtpproxy
	tar -zcvf rtpproxy.tar.gz rtpproxy
	rm -rf rtpproxy

.PHONY: clean
clean:  
	rm -rf $(ODIR)
	cd ./mempool;make clean;cd -
	cd ./util;make clean;cd -
	cd ./log;make clean; cd -
	cd ./timerEvent;make clean; cd -
	cd ./rtp;make clean; cd -
	rm -rf ./bin/$(TARGET)
	rm -rf ./bin/client
	rm -rf ./libs
	rm -f $(TARGET)
	rm -f *.tar.gz
