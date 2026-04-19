#include "EmulatorGui.h"
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <algorithm>
#include <fstream>
#include <poll.h>
#include <sstream>

EmulatorGUI::EmulatorGUI()
    : window(nullptr), renderer(nullptr), textTexture(nullptr), font(nullptr),
      running(true), selectedIndex(0), scrollOffset(0), hoveredIndex(-1),
      emulatorRunning(false), emulatorActive(false), windowWidth(1024), windowHeight(768),
      outputScrollOffset(0), maxOutputLines(0)
{
    emulatorInputPipe[0] = emulatorInputPipe[1] = -1;
    emulatorStdoutPipe[0] = emulatorStdoutPipe[1] = -1;
    emulatorStderrPipe[0] = emulatorStderrPipe[1] = -1;
    emulatorPid = -1;
}

EmulatorGUI::~EmulatorGUI()
{
    stopEmulator();

    if (font)
        TTF_CloseFont(font);
    if (textTexture)
        SDL_DestroyTexture(textTexture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}

bool EmulatorGUI::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0)
    {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        return false;
    }

    window = SDL_CreateWindow("RISC-V Emulator GUI",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              windowWidth, windowHeight,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return false;
    }

    // Load font
    font = TTF_OpenFont("./emulator/assets/DejaVuSans.ttf", 14);
    if (!font)
    {
        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
    }
    if (!font)
    {
        printf("Font loading failed: %s\n", TTF_GetError());
    }

    setupUI();
    refreshProgramList();

    return true;
}

void EmulatorGUI::setupUI()
{
    programListRect = {10, 50, MENU_WIDTH - 20, windowHeight - 200};
    runButtonRect = {10, windowHeight - 140, (MENU_WIDTH - 30) / 2, BUTTON_HEIGHT};
    rebuildButtonRect = {MENU_WIDTH / 2 + 5, windowHeight - 140, (MENU_WIDTH - 30) / 2, BUTTON_HEIGHT};
    cleanButtonRect = {10, windowHeight - 90, (MENU_WIDTH - 30) / 2, BUTTON_HEIGHT};
    refreshButtonRect = {MENU_WIDTH / 2 + 5, windowHeight - 90, (MENU_WIDTH - 30) / 2, BUTTON_HEIGHT};
    stopButtonRect = {10, windowHeight - 40, MENU_WIDTH - 20, BUTTON_HEIGHT};

    outputRect = {MENU_WIDTH + 10, 50, windowWidth - MENU_WIDTH - 20, windowHeight - 120};
    inputRect = {MENU_WIDTH + 10, windowHeight - 60, windowWidth - MENU_WIDTH - 20, 50};
    inputFieldRect = {MENU_WIDTH + 10, windowHeight - 55, windowWidth - MENU_WIDTH - 120, 40};
    sendButtonRect = {windowWidth - 100, windowHeight - 55, 90, 40};
    
    // Add clear button (positioned near the output area)
    clearButtonRect = {outputRect.x + outputRect.w - 60, outputRect.y + 5, 55, 25};
}

void EmulatorGUI::updateOutputLines()
{
    // Split emulatorOutput into lines
    outputLines.clear();
    std::istringstream stream(emulatorOutput);
    std::string line;
    while (std::getline(stream, line))
    {
        outputLines.push_back(line);
    }
    maxOutputLines = outputLines.size();
    
    // Adjust scroll offset if needed
    int visibleLines = (outputRect.h - 30) / 18;
    if (outputScrollOffset > maxOutputLines - visibleLines && maxOutputLines > visibleLines)
    {
        outputScrollOffset = std::max(0, maxOutputLines - visibleLines);
    }
}

void EmulatorGUI::render()
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // Left panel
    SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
    SDL_Rect leftPanel = {0, 0, MENU_WIDTH, windowHeight};
    SDL_RenderFillRect(renderer, &leftPanel);
    SDL_SetRenderDrawColor(renderer, 80, 80, 90, 255);
    SDL_RenderDrawLine(renderer, MENU_WIDTH, 0, MENU_WIDTH, windowHeight);

    renderText("RISC-V Emulator", 10, 10, {100, 200, 255, 255});
    renderText("User Programs:", 10, 35, {200, 200, 200, 255});

    // Program list
    SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
    SDL_Rect listBg = {programListRect.x, programListRect.y,
                       programListRect.w, programListRect.h};
    SDL_RenderFillRect(renderer, &listBg);
    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderDrawRect(renderer, &listBg);

    int visibleItems = programListRect.h / ITEM_HEIGHT;
    for (int i = 0; i < visibleItems && (scrollOffset + i) < (int)programs.size(); i++)
    {
        int idx = scrollOffset + i;
        int y = programListRect.y + i * ITEM_HEIGHT;

        if (idx == selectedIndex)
        {
            SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
            SDL_Rect itemRect = {programListRect.x + 1, y,
                                 programListRect.w - 2, ITEM_HEIGHT - 1};
            SDL_RenderFillRect(renderer, &itemRect);
        }

        std::string displayText = programs[idx].name + " (" +
                                  std::to_string(programs[idx].size / 1024) + " KB)";
        renderText(displayText, programListRect.x + 5, y + 5,
                   (idx == selectedIndex) ? SDL_Color{255, 255, 255, 255}
                                          : SDL_Color{200, 200, 200, 255});
    }

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    bool runHovered = (mouseX >= runButtonRect.x && mouseX <= runButtonRect.x + runButtonRect.w &&
                       mouseY >= runButtonRect.y && mouseY <= runButtonRect.y + runButtonRect.h);
    renderButton("Run Selected", runButtonRect, runHovered, {76, 175, 80, 255});

    bool rebuildHovered = (mouseX >= rebuildButtonRect.x && mouseX <= rebuildButtonRect.x + rebuildButtonRect.w &&
                           mouseY >= rebuildButtonRect.y && mouseY <= rebuildButtonRect.y + rebuildButtonRect.h);
    renderButton("Rebuild All", rebuildButtonRect, rebuildHovered, {255, 152, 0, 255});

    bool cleanHovered = (mouseX >= cleanButtonRect.x && mouseX <= cleanButtonRect.x + cleanButtonRect.w &&
                         mouseY >= cleanButtonRect.y && mouseY <= cleanButtonRect.y + cleanButtonRect.h);
    renderButton("Clean", cleanButtonRect, cleanHovered, {244, 67, 54, 255});

    bool refreshHovered = (mouseX >= refreshButtonRect.x && mouseX <= refreshButtonRect.x + refreshButtonRect.w &&
                           mouseY >= refreshButtonRect.y && mouseY <= refreshButtonRect.y + refreshButtonRect.h);
    renderButton("Refresh", refreshButtonRect, refreshHovered, {33, 150, 243, 255});

    bool stopHovered = (mouseX >= stopButtonRect.x && mouseX <= stopButtonRect.x + stopButtonRect.w &&
                        mouseY >= stopButtonRect.y && mouseY <= stopButtonRect.y + stopButtonRect.h);
    SDL_Color stopColor = emulatorRunning ? SDL_Color{244, 67, 54, 255} : SDL_Color{120, 120, 120, 255};
    renderButton("Stop Emulator", stopButtonRect, stopHovered, stopColor);

    // Output area
    SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
    SDL_Rect outputBg = {outputRect.x, outputRect.y, outputRect.w, outputRect.h};
    SDL_RenderFillRect(renderer, &outputBg);
    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderDrawRect(renderer, &outputBg);

    renderText("Output:", outputRect.x + 5, outputRect.y + 5, {150, 150, 150, 255});
    
    // Clear button
    bool clearHovered = (mouseX >= clearButtonRect.x && mouseX <= clearButtonRect.x + clearButtonRect.w &&
                         mouseY >= clearButtonRect.y && mouseY <= clearButtonRect.y + clearButtonRect.h);
    renderButton("Clear", clearButtonRect, clearHovered, {244, 67, 54, 255});

    // Update output lines for rendering
    updateOutputLines();
    int visibleLines = (outputRect.h - 30) / 18;
    
    // Draw scrollbar for output if needed
    if (maxOutputLines > visibleLines)
    {
        int scrollBarHeight = (visibleLines * (outputRect.h - 30)) / maxOutputLines;
        int scrollBarPos = (outputScrollOffset * (outputRect.h - 30)) / maxOutputLines;
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_Rect scrollBar = {outputRect.x + outputRect.w - 10, outputRect.y + 30 + scrollBarPos, 8, scrollBarHeight};
        SDL_RenderFillRect(renderer, &scrollBar);
        
        // Draw scrollbar track
        SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
        SDL_Rect trackRect = {outputRect.x + outputRect.w - 10, outputRect.y + 30, 8, outputRect.h - 30};
        SDL_RenderDrawRect(renderer, &trackRect);
    }

    // Render output lines with scrolling
    int lineY = outputRect.y + 30;
    int lineHeight = 18;
    int linesDisplayed = 0;
    
    for (int i = outputScrollOffset; i < (int)outputLines.size() && linesDisplayed < visibleLines; i++)
    {
        if (lineY + lineHeight < outputRect.y + outputRect.h - 5)
        {
            renderText(outputLines[i], outputRect.x + 5, lineY, {200, 200, 200, 255});
            lineY += lineHeight;
            linesDisplayed++;
        }
        else
        {
            break;
        }
    }
    
    if (outputLines.empty())
    {
        renderText("No output yet. Select a program and click 'Run Selected'.", 
                   outputRect.x + 5, outputRect.y + 30, {100, 100, 100, 255});
    }

    // Input area
    SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
    SDL_Rect inputBg = {inputRect.x, inputRect.y, inputRect.w, inputRect.h};
    SDL_RenderFillRect(renderer, &inputBg);
    SDL_SetRenderDrawColor(renderer, 60, 60, 70, 255);
    SDL_RenderDrawRect(renderer, &inputBg);

    renderText("Input:", inputRect.x + 5, inputRect.y + 5, {150, 150, 150, 255});

    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_Rect inputField = {inputFieldRect.x, inputFieldRect.y, inputFieldRect.w, inputFieldRect.h};
    SDL_RenderFillRect(renderer, &inputField);
    SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
    SDL_RenderDrawRect(renderer, &inputField);

    renderText(inputBuffer.empty() ? "Type input here..." : inputBuffer,
               inputFieldRect.x + 5, inputFieldRect.y + 12,
               inputBuffer.empty() ? SDL_Color{100, 100, 100, 255} : SDL_Color{255, 255, 255, 255});

    bool sendHovered = (mouseX >= sendButtonRect.x && mouseX <= sendButtonRect.x + sendButtonRect.w &&
                        mouseY >= sendButtonRect.y && mouseY <= sendButtonRect.y + sendButtonRect.h);
    renderButton("Send", sendButtonRect, sendHovered, {33, 150, 243, 255});

    // Status bar
    SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
    SDL_Rect statusBar = {0, windowHeight - 25, windowWidth, 25};
    SDL_RenderFillRect(renderer, &statusBar);
    renderText(statusMessage, 10, windowHeight - 20, {150, 150, 150, 255});

    SDL_RenderPresent(renderer);
}

void EmulatorGUI::renderText(const std::string &text, int x, int y, SDL_Color color)
{
    if (!font)
        return;

    SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, 800);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        SDL_Rect dest = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

void EmulatorGUI::renderButton(const std::string &text, const Rect &rect, bool hovered, SDL_Color bgColor)
{
    if (hovered)
    {
        SDL_SetRenderDrawColor(renderer,
                               std::min(255, bgColor.r + 30),
                               std::min(255, bgColor.g + 30),
                               std::min(255, bgColor.b + 30), 255);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, 255);
    }

    SDL_Rect buttonRect = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer, &buttonRect);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer, &buttonRect);

    renderText(text, rect.x + rect.w / 2 - text.length() * 4, rect.y + rect.h / 2 - 8, {255, 255, 255, 255});
}

void EmulatorGUI::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                handleMouseClick(event.button.x, event.button.y);
            }
            break;

        case SDL_MOUSEMOTION:
            handleMouseMotion(event.motion.x, event.motion.y);
            break;

        case SDL_MOUSEWHEEL:
            handleMouseWheel(event.wheel.y);
            break;

        case SDL_KEYDOWN:
            handleKeyPress(event.key.keysym.sym, event.key.keysym.mod);
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                windowWidth = event.window.data1;
                windowHeight = event.window.data2;
                setupUI();
            }
            break;
        }
    }
}

void EmulatorGUI::handleMouseClick(int x, int y)
{
    // Check program list click
    if (x >= programListRect.x && x <= programListRect.x + programListRect.w &&
        y >= programListRect.y && y <= programListRect.y + programListRect.h)
    {
        int itemIndex = (y - programListRect.y) / ITEM_HEIGHT + scrollOffset;
        if (itemIndex >= 0 && itemIndex < (int)programs.size())
        {
            selectedIndex = itemIndex;
            updateStatus("Selected: " + programs[selectedIndex].name);
        }
    }
    // Check clear button
    else if (x >= clearButtonRect.x && x <= clearButtonRect.x + clearButtonRect.w &&
             y >= clearButtonRect.y && y <= clearButtonRect.y + clearButtonRect.h)
    {
        clearOutput();
    }
    // Check output area scrollbar click
    else if (x >= outputRect.x && x <= outputRect.x + outputRect.w &&
             y >= outputRect.y && y <= outputRect.y + outputRect.h)
    {
        int visibleLines = (outputRect.h - 30) / 18;
        if (maxOutputLines > visibleLines && x >= outputRect.x + outputRect.w - 15)
        {
            int clickPos = y - (outputRect.y + 30);
            outputScrollOffset = (clickPos * maxOutputLines) / (outputRect.h - 30);
            outputScrollOffset = std::max(0, std::min(outputScrollOffset, maxOutputLines - visibleLines));
        }
    }
    // Check other buttons
    else if (x >= runButtonRect.x && x <= runButtonRect.x + runButtonRect.w &&
             y >= runButtonRect.y && y <= runButtonRect.y + runButtonRect.h)
    {
        runSelectedProgram();
    }
    else if (x >= rebuildButtonRect.x && x <= rebuildButtonRect.x + rebuildButtonRect.w &&
             y >= rebuildButtonRect.y && y <= rebuildButtonRect.y + rebuildButtonRect.h)
    {
        rebuildEmulator();
    }
    else if (x >= cleanButtonRect.x && x <= cleanButtonRect.x + cleanButtonRect.w &&
             y >= cleanButtonRect.y && y <= cleanButtonRect.y + cleanButtonRect.h)
    {
        cleanBuild();
    }
    else if (x >= refreshButtonRect.x && x <= refreshButtonRect.x + refreshButtonRect.w &&
             y >= refreshButtonRect.y && y <= refreshButtonRect.y + refreshButtonRect.h)
    {
        refreshProgramList();
    }
    else if (x >= stopButtonRect.x && x <= stopButtonRect.x + stopButtonRect.w &&
             y >= stopButtonRect.y && y <= stopButtonRect.y + stopButtonRect.h)
    {
        stopEmulator();
    }
    else if (x >= sendButtonRect.x && x <= sendButtonRect.x + sendButtonRect.w &&
             y >= sendButtonRect.y && y <= sendButtonRect.y + sendButtonRect.h)
    {
        if (emulatorRunning && !inputBuffer.empty() && emulatorInputPipe[1] != -1)
        {
            std::string input = inputBuffer + "\n";
            write(emulatorInputPipe[1], input.c_str(), input.length());
            appendOutput("> " + inputBuffer);
            inputBuffer.clear();
        }
    }
}

void EmulatorGUI::handleMouseMotion(int x, int y)
{
    // Update hover for program list
    if (x >= programListRect.x && x <= programListRect.x + programListRect.w &&
        y >= programListRect.y && y <= programListRect.y + programListRect.h)
    {
        int itemIndex = (y - programListRect.y) / ITEM_HEIGHT + scrollOffset;
        if (itemIndex >= 0 && itemIndex < (int)programs.size())
        {
            hoveredIndex = itemIndex;
        }
        else
        {
            hoveredIndex = -1;
        }
    }
    else
    {
        hoveredIndex = -1;
    }
}

void EmulatorGUI::handleKeyPress(SDL_Keycode key, Uint16 mod)
{
    if (key == SDLK_UP)
    {
        if (selectedIndex > 0)
        {
            selectedIndex--;
            if (selectedIndex < scrollOffset)
                scrollOffset = selectedIndex;
            updateStatus("Selected: " + programs[selectedIndex].name);
        }
    }
    else if (key == SDLK_DOWN)
    {
        if (selectedIndex < (int)programs.size() - 1)
        {
            selectedIndex++;
            int visibleItems = programListRect.h / ITEM_HEIGHT;
            if (selectedIndex >= scrollOffset + visibleItems)
            {
                scrollOffset = selectedIndex - visibleItems + 1;
            }
            updateStatus("Selected: " + programs[selectedIndex].name);
        }
    }
    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
    {
        if (emulatorRunning && !inputBuffer.empty() && emulatorInputPipe[1] != -1)
        {
            std::string input = inputBuffer + "\n";
            write(emulatorInputPipe[1], input.c_str(), input.length());
            appendOutput("> " + inputBuffer);
            inputBuffer.clear();
        }
        else if (!emulatorRunning)
        {
            runSelectedProgram();
        }
    }
    else if (key == SDLK_BACKSPACE)
    {
        if (!inputBuffer.empty())
        {
            inputBuffer.pop_back();
        }
    }
    else if (key >= 32 && key <= 126)
    {
        inputBuffer += (char)key;
    }
}

void EmulatorGUI::handleMouseWheel(int y)
{
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    
    // Check if mouse is over output area
    if (mouseX >= outputRect.x && mouseX <= outputRect.x + outputRect.w &&
        mouseY >= outputRect.y && mouseY <= outputRect.y + outputRect.h)
    {
        int visibleLines = (outputRect.h - 30) / 18;
        int maxScroll = std::max(0, maxOutputLines - visibleLines);
        
        if (y > 0) // Scroll up
        {
            outputScrollOffset = std::max(0, outputScrollOffset - 3);
        }
        else if (y < 0) // Scroll down
        {
            outputScrollOffset = std::min(maxScroll, outputScrollOffset + 3);
        }
    }
    else
    {
        // Scroll program list
        int visibleItems = programListRect.h / ITEM_HEIGHT;
        int maxScroll = std::max(0, (int)programs.size() - visibleItems);
        
        if (y > 0)
        {
            scrollOffset = std::max(0, scrollOffset - 3);
        }
        else if (y < 0)
        {
            scrollOffset = std::min(maxScroll, scrollOffset + 3);
        }
    }
}

void EmulatorGUI::runSelectedProgram()
{
    if (selectedIndex >= 0 && selectedIndex < (int)programs.size())
    {
        runProgram(programs[selectedIndex].path);
    }
    else
    {
        updateStatus("No program selected", true);
    }
}

void EmulatorGUI::runProgram(const std::string &programPath)
{
    if (emulatorRunning)
    {
        updateStatus("Emulator already running. Stop it first.", true);
        return;
    }

    // Complete cleanup before starting new program
    stopEmulator();

    // Wait a bit for cleanup to complete
    usleep(100000);

    if (!buildIfNeeded())
    {
        updateStatus("Build failed", true);
        return;
    }

    std::string emulatorPath = getEmulatorPath();
    if (!std::ifstream(emulatorPath).good())
    {
        updateStatus("Emulator not found. Run rebuild first.", true);
        return;
    }

    updateStatus("Running: " + programs[selectedIndex].name);
    appendOutput("\n=== Running " + programs[selectedIndex].name + " ===\n");

    // Create pipes for stdin, stdout, stderr
    if (pipe(emulatorInputPipe) == -1 || pipe(emulatorStdoutPipe) == -1 || pipe(emulatorStderrPipe) == -1)
    {
        updateStatus("Failed to create pipes", true);
        return;
    }

    emulatorPid = fork();

    if (emulatorPid == 0)
    {
        // Child process
        // Redirect stdin
        dup2(emulatorInputPipe[0], STDIN_FILENO);
        // Redirect stdout
        dup2(emulatorStdoutPipe[1], STDOUT_FILENO);
        // Redirect stderr
        dup2(emulatorStderrPipe[1], STDERR_FILENO);

        // Close all pipe ends in child
        close(emulatorInputPipe[0]);
        close(emulatorInputPipe[1]);
        close(emulatorStdoutPipe[0]);
        close(emulatorStdoutPipe[1]);
        close(emulatorStderrPipe[0]);
        close(emulatorStderrPipe[1]);

        // Execute emulator
        execl(emulatorPath.c_str(), emulatorPath.c_str(), programPath.c_str(), nullptr);
        exit(1);
    }
    else if (emulatorPid > 0)
    {
        // Parent process - close unused pipe ends
        close(emulatorInputPipe[0]);
        close(emulatorStdoutPipe[1]);
        close(emulatorStderrPipe[1]);

        emulatorActive = true;
        emulatorRunning = true;

        // Start output reader thread
        outputReaderThread = std::thread(&EmulatorGUI::readEmulatorOutput, this);
    }
    else
    {
        updateStatus("Failed to fork process", true);
        // Clean up pipes on failure
        close(emulatorInputPipe[0]);
        close(emulatorInputPipe[1]);
        close(emulatorStdoutPipe[0]);
        close(emulatorStdoutPipe[1]);
        close(emulatorStderrPipe[0]);
        close(emulatorStderrPipe[1]);
    }
}

void EmulatorGUI::readEmulatorOutput()
{
    char buffer[4096];
    ssize_t bytesRead;

    struct pollfd fds[2];
    fds[0].fd = emulatorStdoutPipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = emulatorStderrPipe[0];
    fds[1].events = POLLIN;

    while (emulatorActive)
    {
        int ret = poll(fds, 2, 100); // 100ms timeout

        if (ret > 0)
        {
            // Read stdout
            if (fds[0].revents & POLLIN)
            {
                bytesRead = read(emulatorStdoutPipe[0], buffer, sizeof(buffer) - 1);
                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    appendOutput(buffer);
                }
            }

            // Read stderr
            if (fds[1].revents & POLLIN)
            {
                bytesRead = read(emulatorStderrPipe[0], buffer, sizeof(buffer) - 1);
                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    appendOutput("[STDERR] " + std::string(buffer));
                }
            }
        }

        // Check if child process is still alive
        if (emulatorPid > 0)
        {
            int status;
            pid_t result = waitpid(emulatorPid, &status, WNOHANG);
            if (result == emulatorPid)
            {
                emulatorActive = false;
                emulatorRunning = false;
                if (WIFEXITED(status))
                {
                    appendOutput("\n=== Emulator exited with code " + std::to_string(WEXITSTATUS(status)) + " ===\n");
                }
                updateStatus("Emulator finished");
                break;
            }
        }
    }

    // Clean up pipe file descriptors
    if (emulatorStdoutPipe[0] != -1)
    {
        close(emulatorStdoutPipe[0]);
        emulatorStdoutPipe[0] = -1;
    }
    if (emulatorStderrPipe[0] != -1)
    {
        close(emulatorStderrPipe[0]);
        emulatorStderrPipe[0] = -1;
    }
}

void EmulatorGUI::stopEmulator()
{
    // Signal the thread to stop
    emulatorActive = false;

    // Kill the child process if it's still running
    if (emulatorPid > 0)
    {
        kill(emulatorPid, SIGTERM);
        usleep(100000);             // Wait 100ms for process to die
        kill(emulatorPid, SIGKILL); // Force kill if still running

        // Wait for process to be reaped
        int status;
        waitpid(emulatorPid, &status, WNOHANG);
        emulatorPid = -1;
    }

    // Wait for output reader thread to finish
    if (outputReaderThread.joinable())
    {
        outputReaderThread.join();
    }

    // Close all pipe ends
    if (emulatorInputPipe[1] != -1)
    {
        close(emulatorInputPipe[1]);
        emulatorInputPipe[1] = -1;
    }
    if (emulatorStdoutPipe[0] != -1)
    {
        close(emulatorStdoutPipe[0]);
        emulatorStdoutPipe[0] = -1;
    }
    if (emulatorStderrPipe[0] != -1)
    {
        close(emulatorStderrPipe[0]);
        emulatorStderrPipe[0] = -1;
    }

    // Reset pipe arrays
    emulatorInputPipe[0] = emulatorInputPipe[1] = -1;
    emulatorStdoutPipe[0] = emulatorStdoutPipe[1] = -1;
    emulatorStderrPipe[0] = emulatorStderrPipe[1] = -1;

    emulatorRunning = false;
}

void EmulatorGUI::rebuildEmulator()
{
    stopEmulator();
    updateStatus("Rebuilding emulator and tests...");
    appendOutput("\n=== Starting rebuild ===\n");

    std::string projectRoot = getProjectRoot();
    chdir(projectRoot.c_str());

    FILE *pipe = popen("make clean && make && make tests 2>&1", "r");
    if (pipe)
    {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            appendOutput(buffer);
        }
        pclose(pipe);
    }

    refreshProgramList();
    updateStatus("Rebuild complete");
    appendOutput("=== Rebuild complete ===\n");
}

void EmulatorGUI::cleanBuild()
{
    stopEmulator();
    updateStatus("Cleaning build files...");

    std::string projectRoot = getProjectRoot();
    chdir(projectRoot.c_str());

    FILE *pipe = popen("make clean 2>&1", "r");
    if (pipe)
    {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            appendOutput(buffer);
        }
        pclose(pipe);
    }

    refreshProgramList();
    updateStatus("Clean complete");
    appendOutput("=== Clean complete ===\n");
}

void EmulatorGUI::refreshProgramList()
{
    listPrograms();
    updateStatus("Program list refreshed (" + std::to_string(programs.size()) + " programs)");
}

void EmulatorGUI::listPrograms()
{
    programs.clear();

    std::string projectRoot = getProjectRoot();
    std::string binDir = projectRoot + "/bin";

    DIR *dir = opendir(binDir.c_str());
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string filename = entry->d_name;
            if (filename.find(".bin") != std::string::npos &&
                filename.find("firmware") == std::string::npos &&
                filename.find("bootloader") == std::string::npos)
            {

                std::string fullPath = binDir + "/" + filename;
                std::string name = filename.substr(0, filename.find(".bin"));
                size_t size = getFileSize(fullPath);

                programs.push_back({name, fullPath, size, 0});
            }
        }
        closedir(dir);
    }

    std::sort(programs.begin(), programs.end(),
              [](const ProgramInfo &a, const ProgramInfo &b)
              { return a.name < b.name; });
}

void EmulatorGUI::appendOutput(const std::string &text)
{
    emulatorOutput += text;
    
    // Update lines
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
    {
        outputLines.push_back(line);
    }
    maxOutputLines = outputLines.size();
    
    // Auto-scroll to bottom if we were already at the bottom
    int visibleLines = (outputRect.h - 30) / 18;
    bool wasAtBottom = (outputScrollOffset >= maxOutputLines - visibleLines - 2);
    if (wasAtBottom || outputScrollOffset >= maxOutputLines - visibleLines)
    {
        outputScrollOffset = std::max(0, maxOutputLines - visibleLines);
    }
    
    // Keep total output manageable (last 100000 chars)
    if (emulatorOutput.length() > 100000)
    {
        size_t newlinePos = emulatorOutput.find('\n', 50000);
        if (newlinePos != std::string::npos)
        {
            emulatorOutput = emulatorOutput.substr(newlinePos + 1);
            updateOutputLines();
        }
    }
}

void EmulatorGUI::clearOutput()
{
    emulatorOutput.clear();
    outputLines.clear();
    outputScrollOffset = 0;
    maxOutputLines = 0;
    appendOutput("=== Output cleared ===\n");
}

void EmulatorGUI::updateStatus(const std::string &message, bool isError)
{
    statusMessage = message;
    if (isError)
    {
        appendOutput("ERROR: " + message + "\n");
    }
}

std::string EmulatorGUI::getProjectRoot()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)))
    {
        std::string current = cwd;
        while (current != "/")
        {
            if (std::ifstream(current + "/Makefile").good() ||
                std::ifstream(current + "/bin/rvemu").good())
            {
                return current;
            }
            current = current.substr(0, current.find_last_of('/'));
        }
    }
    return ".";
}

std::string EmulatorGUI::getEmulatorPath()
{
    return getProjectRoot() + "/bin/rvemu";
}

bool EmulatorGUI::buildIfNeeded()
{
    std::string emulatorPath = getEmulatorPath();
    if (!std::ifstream(emulatorPath).good())
    {
        appendOutput("Emulator not found. Building...\n");
        rebuildEmulator();
        return std::ifstream(emulatorPath).good();
    }
    return true;
}

size_t EmulatorGUI::getFileSize(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        return st.st_size;
    }
    return 0;
}

void EmulatorGUI::run()
{
    while (running)
    {
        handleEvents();
        render();
        SDL_Delay(16);
    }
}