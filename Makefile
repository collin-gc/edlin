CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCE=edlin.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=edlin

all: $(SOURCE) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

run: $(EXECUTABLE)
	./$(EXECUTABLE)
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)