#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>

namespace {
constexpr int MAP_W = 60;
constexpr int MAP_H = 32;
constexpr int TILE = 6;
constexpr int MAP_X = 20;
constexpr int MAP_Y = 24;
constexpr int CONTINENT_COUNT = 4;
constexpr int COUNTRIES_PER_CONTINENT = 8;
constexpr int COUNTRY_COUNT = CONTINENT_COUNT * COUNTRIES_PER_CONTINENT;

struct Continent {
    const char* name;
    float cx, cy, rx, ry;
};

struct Country {
    char name[24];
    int continent;
    u32 color;
    int power;
};

const Continent continents[CONTINENT_COUNT] = {
    {"AURELIA", 12.0f, 9.0f, 10.0f, 7.0f},
    {"BOREALIS", 33.0f, 8.5f, 11.0f, 7.0f},
    {"CYRUS", 49.0f, 22.0f, 9.0f, 8.5f},
    {"DRAKON", 27.0f, 25.0f, 10.0f, 6.0f}
};

const char* syllableA[] = {
    "Vel", "Nor", "Az", "Bel", "Qal", "Mor", "Zan",
    "Kor", "Thar", "Val", "Eld", "Rav", "Sol", "Mer"
};
const char* syllableB[] = {
    "aria", "land", "ora", "ven", "heim", "uria",
    "oth", "ara", "esh", "oria", "on", "ia"
};

bool land[MAP_H][MAP_W];
int continentAt[MAP_H][MAP_W];
int countryAt[MAP_H][MAP_W];
Country countries[COUNTRY_COUNT];

int selectedContinent = -1;
int selectedCountry = -1;
int playerCountry = -1;
int menuCursor = 0;
bool gameStarted = false;

C3D_RenderTarget* topTarget = nullptr;

int randomInt(int maxValue) {
    return maxValue > 0 ? std::rand() % maxValue : 0;
}

u32 hsvColor(float h, float s, float v) {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    const float i = std::floor(h * 6.0f);
    const float f = h * 6.0f - i;
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - f * s);
    const float t = v * (1.0f - (1.0f - f) * s);

    switch (static_cast<int>(i) % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    return C2D_Color32(
        static_cast<u8>(r * 255.0f),
        static_cast<u8>(g * 255.0f),
        static_cast<u8>(b * 255.0f),
        255
    );
}

void makeCountryName(char* output, size_t size, int index) {
    const char* a = syllableA[randomInt(sizeof(syllableA) / sizeof(syllableA[0]))];
    const char* b = syllableB[randomInt(sizeof(syllableB) / sizeof(syllableB[0]))];
    std::snprintf(output, size, "%s%s", a, b);

    // Ensure names are distinct without requiring dynamic allocation.
    for (int i = 0; i < index; ++i) {
        if (std::strcmp(output, countries[i].name) == 0) {
            std::snprintf(output, size, "%s%s%d", a, b, index + 1);
            break;
        }
    }
}

bool isLandCell(int x, int y, int c) {
    const Continent& shape = continents[c];
    const float dx = (static_cast<float>(x) - shape.cx) / shape.rx;
    const float dy = (static_cast<float>(y) - shape.cy) / shape.ry;
    const float wobble =
        0.075f * std::sin(x * 0.71f + y * 0.29f + c * 1.7f) +
        0.045f * std::cos(x * 0.31f - y * 0.83f + c);
    return dx * dx + dy * dy < 1.0f + wobble;
}

void generateWorld() {
    std::memset(land, 0, sizeof(land));
    std::memset(continentAt, -1, sizeof(continentAt));
    std::memset(countryAt, -1, sizeof(countryAt));

    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            for (int c = 0; c < CONTINENT_COUNT; ++c) {
                if (isLandCell(x, y, c)) {
                    land[y][x] = true;
                    continentAt[y][x] = c;
                    break;
                }
            }
        }
    }

    int seedX[COUNTRY_COUNT];
    int seedY[COUNTRY_COUNT];

    for (int c = 0; c < CONTINENT_COUNT; ++c) {
        const int firstCountry = c * COUNTRIES_PER_CONTINENT;

        for (int n = 0; n < COUNTRIES_PER_CONTINENT; ++n) {
            const int id = firstCountry + n;
            bool placed = false;

            for (int attempt = 0; attempt < 500 && !placed; ++attempt) {
                const int x = randomInt(MAP_W);
                const int y = randomInt(MAP_H);

                if (!land[y][x] || continentAt[y][x] != c)
                    continue;

                bool spaced = true;
                for (int p = 0; p < n; ++p) {
                    const int dx = x - seedX[firstCountry + p];
                    const int dy = y - seedY[firstCountry + p];
                    if (dx * dx + dy * dy < 9) {
                        spaced = false;
                        break;
                    }
                }

                if (spaced) {
                    seedX[id] = x;
                    seedY[id] = y;
                    placed = true;
                }
            }

            if (!placed) {
                for (int y = 0; y < MAP_H && !placed; ++y) {
                    for (int x = 0; x < MAP_W && !placed; ++x) {
                        if (land[y][x] && continentAt[y][x] == c) {
                            seedX[id] = x;
                            seedY[id] = y;
                            placed = true;
                        }
                    }
                }
            }

            Country& country = countries[id];
            makeCountryName(country.name, sizeof(country.name), id);
            country.continent = c;
            country.power = 70 + randomInt(61);
            country.color = hsvColor(
                std::fmod((id * 0.61803398875f) + 0.07f, 1.0f),
                0.62f, 0.92f
            );
        }
    }

    // Each land cell belongs to its nearest seed on the same continent.
    // This produces one contiguous Voronoi-like territory per country.
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            if (!land[y][x])
                continue;

            const int c = continentAt[y][x];
            const int firstCountry = c * COUNTRIES_PER_CONTINENT;
            int bestId = firstCountry;
            int bestDistance = 0x7fffffff;

            for (int n = 0; n < COUNTRIES_PER_CONTINENT; ++n) {
                const int id = firstCountry + n;
                const int dx = x - seedX[id];
                const int dy = y - seedY[id];
                const int distance = dx * dx + dy * dy;

                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestId = id;
                }
            }

            countryAt[y][x] = bestId;
        }
    }
}

void drawRect(float x, float y, float w, float h, u32 color) {
    C2D_DrawRectangle(x, y, 0.0f, w, h,
                      color, color, color, color);
}

void drawMap() {
    const u32 ocean = C2D_Color32(17, 35, 57, 255);
    const u32 border = C2D_Color32(17, 23, 31, 255);
    drawRect(0, 0, 400, 240, ocean);

    // Subtle map frame.
    drawRect(14, 18, 372, 198, C2D_Color32(37, 57, 75, 255));

    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            const int id = countryAt[y][x];
            if (id < 0)
                continue;

            u32 color = countries[id].color;
            if (gameStarted && id == playerCountry)
                color = C2D_Color32(255, 220, 55, 255);

            const float px = static_cast<float>(MAP_X + x * TILE);
            const float py = static_cast<float>(MAP_Y + y * TILE);
            drawRect(px, py, TILE, TILE, color);

            // Thin political borders.
            if (x + 1 >= MAP_W || countryAt[y][x + 1] != id)
                drawRect(px + TILE - 1, py, 1, TILE, border);
            if (y + 1 >= MAP_H || countryAt[y + 1][x] != id)
                drawRect(px, py + TILE - 1, TILE, 1, border);
        }
    }

    // Highlight the chosen continent before the player starts.
    if (!gameStarted && selectedContinent >= 0) {
        const Continent& c = continents[selectedContinent];
        const float cx = MAP_X + c.cx * TILE;
        const float cy = MAP_Y + c.cy * TILE;
        const u32 marker = C2D_Color32(255, 255, 255, 255);
        drawRect(cx - 2, cy - 2, 5, 5, marker);
    }
}

void showContinentMenu() {
    consoleClear();
    std::printf("\x1b[1;1H WORLD CONQUEST");
    std::printf("\x1b[3;1H CHOOSE YOUR CONTINENT");
    std::printf("\x1b[5;1H%s 1. AURELIA  (WEST)", menuCursor == 0 ? ">" : " ");
    std::printf("\x1b[8;1H%s 2. BOREALIS (NORTH)", menuCursor == 1 ? ">" : " ");
    std::printf("\x1b[11;1H%s 3. CYRUS    (EAST)", menuCursor == 2 ? ">" : " ");
    std::printf("\x1b[14;1H%s 4. DRAKON   (SOUTH)", menuCursor == 3 ? ">" : " ");
    std::printf("\x1b[19;1H32 countries - 8 per continent");
    std::printf("\x1b[22;1HTap a continent or use UP/DOWN + A");
    std::printf("\x1b[24;1HSTART: exit");
}

void startGame(int continent) {
    selectedContinent = continent;
    const int first = continent * COUNTRIES_PER_CONTINENT;
    playerCountry = first + randomInt(COUNTRIES_PER_CONTINENT);
    selectedCountry = playerCountry;
    gameStarted = true;
    std::strncpy(countries[playerCountry].name, "YOUR NATION",
                 sizeof(countries[playerCountry].name) - 1);
    countries[playerCountry].name[sizeof(countries[playerCountry].name) - 1] = '\0';
}

void showGameInfo() {
    consoleClear();
    std::printf("\x1b[1;1H WORLD CONQUEST");
    std::printf("\x1b[3;1H YOUR CONTINENT: %s", continents[selectedContinent].name);
    std::printf("\x1b[5;1H YOUR NATION: %s", countries[playerCountry].name);
    std::printf("\x1b[7;1H TERRITORIES: %d COUNTRIES",
                COUNTRIES_PER_CONTINENT);
    std::printf("\x1b[10;1H Yellow = your country");
    std::printf("\x1b[13;1HWorld map: top screen");
    std::printf("\x1b[16;1HYour country is placed randomly");
    std::printf("\x1b[19;1Hwithin your chosen continent.");
    std::printf("\x1b[22;1HB: choose another continent");
    std::printf("\x1b[24;1HSTART: exit");
}

void renderFrame() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topTarget, C2D_Color32(17, 35, 57, 255));
    C2D_SceneBegin(topTarget);
    drawMap();
    C3D_FrameEnd(0);
}

void chooseContinent(int continent) {
    if (continent < 0 || continent >= CONTINENT_COUNT)
        return;
    startGame(continent);
    showGameInfo();
}

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    gfxInitDefault();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    if (!C2D_Init(4096)) {
        C3D_Fini();
        gfxExit();
        return 1;
    }
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!topTarget) {
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    consoleInit(GFX_BOTTOM, nullptr);
    generateWorld();
    showContinentMenu();

    while (aptMainLoop()) {
        hidScanInput();
        const u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        if (!gameStarted) {
            bool menuChanged = false;
            if (kDown & KEY_UP) {
                menuCursor = (menuCursor + CONTINENT_COUNT - 1) % CONTINENT_COUNT;
                menuChanged = true;
            }
            if (kDown & KEY_DOWN) {
                menuCursor = (menuCursor + 1) % CONTINENT_COUNT;
                menuChanged = true;
            }
            if (kDown & KEY_A)
                chooseContinent(menuCursor);

            touchPosition touch;
            if (kDown & KEY_TOUCH) {
                hidTouchRead(&touch);
                if (touch.py >= 24 && touch.py < 120) {
                    const int row = (touch.py - 24) / 24;
                    if (row >= 0 && row < CONTINENT_COUNT)
                        chooseContinent(row);
                }
            }

            if (!gameStarted && menuChanged)
                showContinentMenu();
        } else if (kDown & KEY_B) {
            gameStarted = false;
            selectedContinent = -1;
            playerCountry = -1;
            menuCursor = 0;
            showContinentMenu();
        }

        renderFrame();
        gspWaitForVBlank();
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
