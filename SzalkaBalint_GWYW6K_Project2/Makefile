
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
TARGET   := bouncing_circle
SRC      := main.cpp

PLATFORM ?= linux

ifeq ($(PLATFORM),macos)
    LIBS := -framework OpenGL -lGLEW -lglfw
else
    LIBS := -lGL -lGLEW -lglfw
endif

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
