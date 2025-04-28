CXX = g++
CXXFLAGS = -Wall -std=c++17
SDL_FLAGS = `sdl2-config --cflags --libs` -lSDL2_ttf

SOURCES = main.cpp spear_blocker.cpp spear_runner.cpp assets.cpp menu.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = game_menu

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(SDL_FLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< $(SDL_FLAGS)

clean:
	rm -f $(OBJECTS) $(TARGET)
