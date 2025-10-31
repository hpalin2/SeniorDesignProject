CXX := clang++
ASIO_PREFIX := $(shell brew --prefix asio)
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -I$(ASIO_PREFIX)/include
LDFLAGS := -lpthread

TARGET := server
SRC := main.cpp

all: $(TARGET)

$(TARGET): $(SRC) crow_all.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)


