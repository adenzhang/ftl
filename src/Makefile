CXXFLAGS = -g -O3 -Wall -fmessage-length=0 -std=c++14

#OBJS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
OBJS = MergeSortedLists.o
LIBS =
INCLUDES = -I.

TARGET =	app

$(TARGET):	$(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS) $(INCLUDES)

#.cpp.o:
#	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

