CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I.

TARGET = gmb
SRCS = src/main.cpp \
       src/instance/instance.cpp \
       src/solution/solution.cpp \
       src/constraints/constraints.cpp \
       src/construction/construction.cpp \
       src/cultural/cultural.cpp \
       src/utils/random.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean