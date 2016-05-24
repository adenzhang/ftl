CXXFLAGS = -g -Wall -fmessage-length=0 -std=c++0x

#OBJS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
OBJS = BuildingOutline.o
LIBS =
INCLUDES = -I.

TARGET =	app

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(INCLUDES)

#.cpp.o:
#	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

