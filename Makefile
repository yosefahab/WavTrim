CXX = clang++
LLDFLAGS ?=
CXXFLAGS = -g --std=c++17 -Wall -Werror -Wextra
# -g enables debug information,
# -Wall turns on all warnings,
# -Werror turns warnings to errors,
# -WExtra turns on extra debug information not provided by -Wall

SOURCES = wavTrim.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = wavTrim

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(LLDFLAGS) $^ -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.exe *.out