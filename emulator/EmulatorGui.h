#ifndef EMULATOR_GUI_H
#define EMULATOR_GUI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

struct ProgramInfo
{
    std::string name;
    std::string path;
    size_t size;
    time_t modifiedTime;
};

class EmulatorGUI
{
public:
    EmulatorGUI();
    ~EmulatorGUI();

    bool initialize();
    void run();
    void refreshProgramList();

private:
    // UI element structure - define BEFORE using it
    struct Rect
    {
        int x, y, w, h;
    };

    // SDL components
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *textTexture;
    TTF_Font *font;
    TTF_Font *smallFont;

    // UI state
    bool running;
    int selectedIndex;
    int scrollOffset;
    int hoveredIndex;
    std::vector<ProgramInfo> programs;
    std::string statusMessage;
    std::string emulatorOutput;
    std::string inputBuffer;
    bool emulatorRunning;

    // Process handling (Linux/WSL)
    pid_t emulatorPid;
    int emulatorInputPipe[2];
    int emulatorStdoutPipe[2];
    int emulatorStderrPipe[2];
    std::thread outputReaderThread;
    std::atomic<bool> emulatorActive;

    // UI dimensions
    int windowWidth;
    int windowHeight;
    const int MENU_WIDTH = 320;
    const int BUTTON_HEIGHT = 40;
    const int ITEM_HEIGHT = 35;

    // UI rectangles
    Rect programListRect;
    Rect runButtonRect;
    Rect rebuildButtonRect;
    Rect cleanButtonRect;
    Rect refreshButtonRect;
    Rect stopButtonRect;
    Rect outputRect;
    Rect inputRect;
    Rect inputFieldRect;
    Rect sendButtonRect;
    Rect scrollBarRect;
    Rect clearButtonRect; // Now Rect is defined above

    // Output scrolling
    int outputScrollOffset;
    int maxOutputLines;
    std::vector<std::string> outputLines;

    // Colors
    struct Color
    {
        Uint8 r, g, b, a;
    };

    Color bgColor = {30, 30, 35, 255};
    Color panelColor = {40, 40, 45, 255};
    Color borderColor = {80, 80, 90, 255};
    Color highlightColor = {0, 120, 215, 255};
    Color outputBgColor = {20, 20, 25, 255};

    // Methods
    void setupUI();
    void render();
    void renderText(const std::string &text, int x, int y, SDL_Color color);
    void renderButton(const std::string &text, const Rect &rect, bool hovered, SDL_Color bgColor);
    void handleEvents();
    void handleMouseClick(int x, int y);
    void handleMouseMotion(int x, int y);
    void handleKeyPress(SDL_Keycode key, Uint16 mod);
    void handleMouseWheel(int y);

    void runSelectedProgram();
    void runProgram(const std::string &programPath);
    void rebuildEmulator();
    void cleanBuild();
    void stopEmulator();
    void updateStatus(const std::string &message, bool isError = false);
    void appendOutput(const std::string &text);
    void clearOutput();
    void readEmulatorOutput();
    void updateOutputLines();

    std::string getProjectRoot();
    std::string getEmulatorPath();
    bool buildIfNeeded();
    void listPrograms();
    size_t getFileSize(const std::string &path);
};

#endif