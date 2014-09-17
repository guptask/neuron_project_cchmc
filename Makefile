CXX= g++
CXXFLAGS+= -c -std=c++11 -Wall -Werror `pkg-config --cflags opencv`
LDFLAGS+= -ltiff `pkg-config --libs opencv`
SRC= src
JSEG_SRC= third_party/jseg
SOURCES= $(wildcard $(SRC)/*.cpp)
INCLUDIR= $(wildcard $(SRC)/*.hpp)
OBJECTS= $(join $(addsuffix ../, $(dir $(SOURCES))), $(notdir $(SOURCES:.cpp=.o)))

EXECUTABLE = segment
JSEG_EXECUTABLE = segdist

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	@$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

%.o: $(SRC)/%.cpp $(INCLUDIR)
	@$(CXX) $(CXXFLAGS) $< -o $@
	@cd $(JSEG_SRC); $(MAKE) $(MFLAGS)

clean:
	@rm -f $(EXECUTABLE) *.o
	@rm -f $(JSEG_EXECUTABLE) $(JSEG_SRC)/*.o

.PHONY: all clean
