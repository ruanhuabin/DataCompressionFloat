zlib=/opt/zlib1.2.3/
CC=gcc
CFLAGS=-Wall -I${zlib}/include/
#LDFLAGS=-L${zlib}/lib/ -lz
LIBS=${zlib}/lib/libz.a
SOURCES=lz4.c lz4hc.c nzips.c common.c workers.c hpos.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=hpos nhpos mrcviewer

all: $(SOURCES) $(EXECUTABLE)
	
#$(EXECUTABLE): $(OBJECTS) 
#	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

hpos: nzips.o lz4.o lz4hc.o common.o workers.o hpos.c
	$(CC) $(LDFLAGS) -g nzips.o lz4.o lz4hc.o common.o workers.o hpos.c ${LIBS} -o hpos 

nhpos: nzips.o lz4.o lz4hc.o common.o workers.o adapt.o main.c
	$(CC) $(LDFLAGS) -g nzips.o lz4.o lz4hc.o common.o workers.o adapt.o main.c ${LIBS} -lpthread -o nhpos 
mrcviewer:mrcviewer.c
	$(CC) -std=gnu99 -o mrcviewer mrcviewer.c
.o:
	$(CC) $(CFLAGS) $< -o $@


clean:
	rm -rf *.o hpos nhpos
