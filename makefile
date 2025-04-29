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

# Target for cross-compilation to Beaglebone
# This target uses the specific ARM cross-compiler and doesn't rely on the host's SDL config
# Note: Assumes SDL headers/libs for the target are NOT provided via host sdl2-config/pkg-config
# If you need SDL on Beaglebone, you'll likely need a sysroot and target-specific flags/libraries.
beaglebone:
	arm-linux-gnueabihf-g++ $(CXXFLAGS) $(SOURCES) -o $(TARGET) --static $(LINUX_SDL_FLAGS) # Added CXXFLAGS and LINUX_SDL_FLAGS here, assuming they are relevant for the target env setup
	arm-linux-gnueabihf-strip -s $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)