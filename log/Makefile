SRCS =  resource.cpp  logclient.cpp log.cpp logserver.cpp
ODIR = ./logobjs/
OBJS = $(addprefix $(ODIR),$(SRCS:.cpp=.o))  
DEPS = $(addprefix $(ODIR),$(SRCS:.cpp=.d)) 

INCLUDE = -I./logExport -I ../export/utilExport -I ../export/mempoolExport
CXXFLAGS = -g -fPIC -fstack-protector -Wall -Werror=declaration-after-statement  $(INCLUDE) 
LDFLAGS = -pthread

$(ODIR)liblog.so:$(OBJS)
	gcc   -shared $(LDFLAGS)   -o $@ $^
#	ar crv ./logobjs/liblog.a $(OBJS)
#	g++ $(CXXFLAGS) $(OBJS) ../util/objs/Thread.o  -o test

$(ODIR)%o: %cpp
	test -d $(ODIR) || mkdir -p $(ODIR)
	g++ -c $(CXXFLAGS) $< -o $@

#using '-', indicate that if make don't find the depentfile, don't care the errors,
#at begining, still don't produce the dependent file
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

$(ODIR)%.d:%.cpp
	test -d $(ODIR) || mkdir -p $(ODIR)
	@set -e; rm -f $@;
	g++ -MM  $(CXXFLAGS)  $< > $@.123456;
	sed 's,\($*\)\.o[ :]*, $(ODIR)\1.o $@ : ,g' < $@.123456 > $@;
	rm -f $@.123456


.PHONY: clean
clean:  
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f ./logobjs/liblog.a
	rm -rf ./logobjs
	git clean -df

