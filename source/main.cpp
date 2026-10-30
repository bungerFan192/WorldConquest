#include <3ds.h>
#include <citro3d.h>

#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    gfxInitDefault();

    // Bring up the PICA200 GPU library. The first milestone uses the
    // console for diagnostics; the custom citro3D renderer comes next.
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    PrintConsole topConsole;
    PrintConsole bottomConsole;

    consoleInit(GFX_TOP, &topConsole);
    consoleInit(GFX_BOTTOM, &bottomConsole);

    consoleSelect(&topConsole);
    printf("\x1b[1;1H WORLD CONQUEST");
    printf("\x1b[3;1H Nintendo 3DS");
    printf("\x1b[5;1H C++ / libctru / citro3D");
    printf("\x1b[8;1H Initial engine boot");
    printf("\x1b[10;1H citro3D initialized!");

    consoleSelect(&bottomConsole);
    printf("\x1b[1;1H WORLD CONQUEST");
    printf("\x1b[3;1H BOTTOM SCREEN");
    printf("\x1b[5;1H Touch controls will live here.");
    printf("\x1b[8;1H Press START to exit.");

    while (aptMainLoop()) {
        hidScanInput();

        const u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            break;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    C3D_Fini();
    gfxExit();
    return 0;
}
