PROJECT_NAME = build
CXX = g++

ifeq ($(BUILD_MODE), DEBUG)
	ifeq ($(ASAN), 1)
	 	CFLAGS += -fsanitize=address -g -fno-omit-frame-pointer -O0
	else
		CFLAGS += -g -O0
	endif
else
	CFLAGS += -s -O1
endif

INCLUDE_PATHS = -Iinclude
SRC_DIR = src
OBJ_DIR = obj

SRC  = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp,obj/%.o,$(SRC))

all: $(PROJECT_NAME)

$(OBJ_DIR):
	mkdir -p obj

$(PROJECT_NAME): $(OBJS)
	$(CXX) -o $(PROJECT_NAME) $(OBJS) $(CFLAGS) $(INCLUDE_PATHS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) -c $< -o $@ $(CFLAGS) $(INCLUDE_PATHS)

clean:
	rm -rf obj
	rm -f $(PROJECT_NAME)

.PHONY: all clean