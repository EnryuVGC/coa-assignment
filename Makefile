CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS += -Iinclude

SRCS := $(wildcard src/*.cpp)
HDRS := $(wildcard include/*.h)

risc201: $(SRCS) $(HDRS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SRCS)

clean:
	rm -f risc201

.PHONY: clean
