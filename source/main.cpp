#include <3ds.h>
#include <citro3d.h>

#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    gfxInitDefault();

    // Initialize citro3D now so the project is linked against the PICA200
    // rendering library from the first build. The custom map renderer will
    // be added in the next milestone.
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    consoleInit(GFX_TOP, nullptr);
    consoleInit(GFX_BOTTOM, nullptr);

    printf("\x1b[1;1H WORLD CONQUEST");
    printf("\x1b[3;1H Nintendo 3DS");
    printf("\x1b[5;1H C++ / libctru / citro3D");
    printf("\x1b[8;1H Initial engine boot");
    printf("\x1b[10;1H\x1b[32m citro3D initialized!\x1b[0m");

    printf("\x1b[1;1H WORLD CONQUEST");
    printf("\x1b[3;1H BOTTOM SCREEN");
    printf("\x1b[5;1H Touch controls will live here.");

    while (aptMainLoop()) {
        hidScanInput();

        const u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            break;
        }

        // Rendering, world simulation and touch input are introduced in
        // separate milestones to keep the first hardware build easy to debug.
    }

    C3D_Fini();
    gfxExit();
    return 0;
}
