
CXX=g++
CXXFLAGS=-g
LIBS=
SOURCES=btree.cpp cd.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=btree

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LIBS) -o $(EXECUTABLE) $(OBJECTS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<