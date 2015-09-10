CC=g++
CFLAGS=-Wall -I./ -Wfatal-errors

MAIN=test/test

OBJECTS = $(subst cpp,o,$(wildcard *.cpp))
OBJECTS+= $(subst cpp,o,$(wildcard test/*.cpp))
OBJECTS+= $(subst cpp,o,$(wildcard clap/*.cpp))

# Add -DNDEBUG and -O2 in CFLAGS before testing
#CFLAGS+= -O2 -DNDEBUG
# User c++11
CFLAGS+= -std=c++11


all: $(MAIN) $(OBJECTS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $^

$(MAIN): $(OBJECTS)
	$(CC) -o $(MAIN) $(OBJECTS)

clean:
	rm -f $(MAIN) $(OBJECTS)

