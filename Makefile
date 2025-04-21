#EECS-498-007 Makefile. First release target is run when calling "make" for the autograder
# rest of the targets are there for development

# compiler
CXX = clang++

#compiler options
CXXFLAGS = -std=c++17 -pedantic -Wall

#program names
EXECUTABLE = game_engine_linux

#test sources
TESTSOURCES = $(wildcard test*.cpp)

SOURCES = $(wildcard src/*.cpp)

INCLUDES = -ISDL/include/ -Iglm/ -Irapidsjon/ -ISDL_Image -ISDL_mixer -ISDL_TTF -ILua -ILuaBridge -IBox2D/include/ -IBox2D/src

CXXFLAGS += $(INCLUDES)

LINKFLAGS = -L/usr/local/lib -LLua -LBox2D -Wl,-rpath=Box2D -lSDL2_mixer -lSDL2_ttf -lSDL2_image -lSDL2 -lSDL2main -llua -lbox2d

HEADERS = $(wildcard src/*.h) $(wildcard src/*hpp)

game_engine_linux: CXXFLAGS += -O3 -DNDEBUG
game_engine_linux: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE) $(LINKFLAGS)

game_engine_linux_debug: CXXFLAGS += -g3 -DDEBUG
game_engine_linux_debug: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_debug $(LINKFLAGS)

BOX2DSRC = $(wildcard Box2D/src/collision/*.cpp) $(wildcard Box2D/src/common/*.cpp) $(wildcard Box2D/src/dynamics/*.cpp) $(wildcard Box2D/src/rope/*.cpp)

box2d: CXXFLAGS += -O3 -DNDEBUG
box2d:
	$(CXX) $(CXXFLAGS) -shared  -o libbox2d.so $(BOX2DSRC) -fPIC
	mv libbox2d.so Box2D

valgrind: CXXFLAGS += -g2
valgrind:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(EXECUTABLE)_valgrind $(LINKFLAGS)

test: CXXFLAGS += -g3 -DDEBUG
test:
	$(CXX) $(CXXFLAGS) $(TESTSOURCES) src/luafuncs.cpp src/ParticleSystem.cpp src/RigidBody.cpp src/scene.cpp src/EventBus.cpp src/InputManager.cpp -o test $(LINKFLAGS)
.PHONY: test

.PHONY: clean
clean:
	rm eecs498-007 engine $(EXECUTABLE) $(EXECUTABLE)_debug $(EXECUTABLE)_valgrind test

.PHONY: style
style:
	clang-tidy $(SOURCES) -- $(INCLUDES)

all: game_engine_linux_debug game_engine_linux box2d
all: valgrind
.PHONY all: