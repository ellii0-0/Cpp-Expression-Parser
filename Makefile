RAYLIB_PATH ?= C:/raylib-6.2

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O1 -Iinclude -I$(RAYLIB_PATH)/src
LDFLAGS  = -L$(RAYLIB_PATH)/src

RM_OBJ = rm -rf obj
RM_BIN = rm -f $(TARGET)
MKDIR  = mkdir -p obj
RUN    = ./$(TARGET)

ifeq ($(OS),Windows_NT)
	RAYLIB_PATH ?= C:/raylib-6.2
	CXXFLAGS = -std=c++17 -Wall -O1 -Iinclude -I$(RAYLIB_PATH)/src
	LDFLAGS  = -L$(RAYLIB_PATH)/src
	LDLIBS   = -lraylib -lopengl32 -lgdi32 -lwinmm
	TARGET   = build.exe
else
	CXXFLAGS = -std=c++17 -Wall -O1 -Iinclude
	LDFLAGS  =
	LDLIBS   = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	TARGET   = build
endif

SRC  = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp,obj/%.o,$(SRC))

.PHONY: all run clean

# ---------- Alvos ----------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

obj/%.o: src/%.cpp
	@$(MKDIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Compila (se preciso) e executa
run: $(TARGET)
	$(RUN)

# Remove objetos e executável
clean:
	-$(RM_OBJ)
	-$(RM_BIN)