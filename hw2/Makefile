#
# This is for your executable
#
CFLAGS = -O3
CFLAGS = -g -Wall
#
# This is the setting for TA
#
# CFLAGS = -g -Wall -DTA_KB_SETTING
# CFLAGS = -O3 -DTA_KB_SETTING
# CFLAGS = -O3 -m32 -DTA_KB_SETTING

###############
# PHONY rules #
###############
.PHONY : hw2 test ref

hw2 : MAKEFLAGS = $(CFLAGS)
test: MAKEFLAGS = -DTEST_ASC $(CFLAGS)
ref : MAKEFLAGS = -DMAKE_REF $(CFLAGS)

hw2 : .cmdCharDef cmdReader
test: .cmdCharDef testAsc
ref : .cmdCharDef cmdReader-ref

##############
# Executable #
##############
cmdReader: main.o cmdReader.o cmdCharDef.o
	g++ -o $@ $(MAKEFLAGS) main.o cmdReader.o cmdCharDef.o
#	strip $@

testAsc: testAsc.o cmdCharDef.o
	g++ -o $@ $(MAKEFLAGS) testAsc.o cmdCharDef.o
	strip $@

cmdReader-ref: main.o cmdReader.o.ref cmdCharDef.o
	g++ -o $@ $(MAKEFLAGS) main.o cmdReader.o.ref cmdCharDef.o
	strip $@

################
# Object files #
################
main.o: main.cpp cmdParser.h
	g++ -c $(MAKEFLAGS) main.cpp

cmdReader.o: cmdReader.cpp cmdParser.h
	g++ -c $(MAKEFLAGS) cmdReader.cpp

cmdCharDef.o: cmdCharDef.cpp cmdParser.h
	g++ -c $(MAKEFLAGS) cmdCharDef.cpp

testAsc.o: testAsc.cpp cmdCharDef.h
	g++ -c $(MAKEFLAGS) testAsc.cpp

cmdParser.h: cmdCharDef.h
	touch cmdParser.h

#######################
# No dependency rules #
#######################
.cmdCharDef:
	@rm -f cmdCharDef.o

32:
	@ln -sf cmdReader.o.ref32 cmdReader.o.ref

64:
	@ln -sf cmdReader.o.ref64 cmdReader.o.ref

mac:
	@ln -sf cmdReader.o.mac cmdReader.o.ref

clean:
	rm -f *.o cmdReader testAsc

ctags:
	ctags *.cpp *.h
