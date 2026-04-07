
# Debug definitions
#DEBUG = -g

#DEBUG_FLAGS = -DSERIALPORT_DEBUG 

CXXFLAGS += -Wall -Wextra -std=c++17  -O3 $(DEBUG)#-+Werror -fopenmp -mavx 

LDFLAGS= -lpthread
EXECUTABLE=hyperbird

SRC_FILES = src/*.cpp

# xlnt library variables:
XLNT_INC_PATH = -I$(CURDIR)/libs/xlnt/install/include
XLNT_LIB_PATH = -L$(CURDIR)/libs/xlnt/install/lib
XLNT_LIBS = -lxlnt

ANDOR_SDK_LIB_PATH = -L/usr/local/lib
ANDOR_SDK_LIBS = -latcore

# Merge all libs information into a single variable
LIB_PATHS = $(XLNT_LIB_PATH) $(ANDOR_SDK_LIB_PATH)
INCLUDE_PATHS = -Iinclude $(XLNT_INC_PATH)
LIBS = $(XLNT_LIBS) $(ANDOR_SDK_LIBS)


all: $(EXECUTABLE)

$(EXECUTABLE): $(SRC_FILES) include/**
	$(CXX) $(SRC_FILES) $(INCLUDE_PATHS) $(CXXFLAGS) $(DEBUG_FLAGS) -o $(EXECUTABLE) $(LIB_PATHS) $(LDFLAGS) $(LIBS) 


clean:
	rm -f $(EXECUTABLE)
