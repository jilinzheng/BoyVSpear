CXX = g++
CXXFLAGS = -Wall -std=c++17

SOURCES = main.cpp spear_blocker.cpp spear_runner.cpp assets.cpp menu.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = game_menu

LINUX_SDL_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf
MACOS_SDL_FLAGS = `pkg-config --cflags --libs sdl2 SDL2_ttf`

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PLATFORM_SDL_FLAGS = $(MACOS_SDL_FLAGS)
else
    PLATFORM_SDL_FLAGS = $(LINUX_SDL_FLAGS)
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(PLATFORM_SDL_FLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(PLATFORM_SDL_FLAGS)

clean:
	rm -f $(OBJECTS) $(TARGET)