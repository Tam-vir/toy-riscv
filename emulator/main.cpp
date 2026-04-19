#include "EmulatorGui.h"

int main(int argc, char *argv[])
{
    EmulatorGUI gui;

    if (!gui.initialize())
    {
        printf("Failed to initialize GUI\n");
        return 1;
    }

    gui.run();

    return 0;
}