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
constexpr int COUNTRIES_PER_CONTINENT = 4;
constexpr int COUNTRY_COUNT = CONTINENT_COUNT * COUNTRIES_PER_CONTINENT;

struct Point { float x, y; };
struct Continent {
    const char* name;
    Point outline[14];
    int points;
};

struct Country {
    char name[24];
    int continent;
    u32 color;
    int power;
};

const Continent continents[CONTINENT_COUNT] = {
    {"AURELIA", {{2,5},{5,2},{11,1},{16,4},{18,8},{16,12},{14,16},{9,17},{5,14},{2,11},{1,8}}, 11},
    {"BOREALIS", {{21,2},{26,1},{31,3},{35,2},{39,5},{40,9},{37,12},{34,13},{30,11},{27,13},{23,10},{20,6}}, 12},
    {"CYRUS", {{43,7},{47,4},{52,5},{57,9},{58,14},{55,18},{53,23},{49,24},{46,21},{44,17},{41,14},{42,10}}, 12},
    {"DRAKON", {{17,20},{21,17},{26,18},{30,20},{34,19},{39,22},{41,26},{38,29},{33,30},{28,28},{24,30},{20,27},{16,24}}, 13}
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

bool insidePolygon(float x, float y, const Continent& c) {
    bool inside = false;
    for (int i = 0, j = c.points - 1; i < c.points; j = i++) {
        const Point& a = c.outline[i];
        const Point& b = c.outline[j];
        if (((a.y > y) != (b.y > y)) &&
            (x < (b.x - a.x) * (y - a.y) / (b.y - a.y + 0.0001f) + a.x))
            inside = !inside;
    }
    return inside;
}

bool isLandCell(int x, int y, int c) {
    const float coast = 0.55f * std::sin(x * 1.7f + y * 0.83f + c * 2.1f) +
                        0.30f * std::cos(x * 0.61f - y * 1.31f + c);
    return insidePolygon(x + 0.5f, y + 0.5f, continents[c]) && coast > -0.60f;
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
            if (countries[id].controlled)
                color = C2D_Color32(255, 255, 255, 255);

            const float px = static_cast<float>(MAP_X + x * TILE);
            const float py = static_cast<float>(MAP_Y + y * TILE);
            drawRect(px, py, TILE, TILE, color);

            // Political borders; selected country gets a bright outline.
            const u32 edge = (id == selectedCountry)
                ? C2D_Color32(255, 230, 80, 255) : border;
            if (x + 1 >= MAP_W || countryAt[y][x + 1] != id)
                drawRect(px + TILE - 1, py, 1, TILE, edge);
            if (y + 1 >= MAP_H || countryAt[y + 1][x] != id)
                drawRect(px, py + TILE - 1, TILE, 1, edge);
            if (id == selectedCountry) {
                if (x > 0 && countryAt[y][x - 1] != id)
                    drawRect(px, py, 1, TILE, edge);
                if (y > 0 && countryAt[y - 1][x] != id)
                    drawRect(px, py, TILE, 1, edge);
            }
        }
    }

    // Highlight the chosen continent before the player starts.
    if (!gameStarted) {
        // Continent selection is shown by its menu entry.
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
    std::printf("\x1b[19;1H16 countries - 4 per continent");
    std::printf("\x1b[22;1HTap a continent or use UP/DOWN + A");
    std::printf("\x1b[24;1HSTART: exit");
}

void startGame(int continent) {
    selectedContinent = continent;
    const int first = continent * COUNTRIES_PER_CONTINENT;
    playerCountry = first + randomInt(COUNTRIES_PER_CONTINENT);
    selectedCountry = playerCountry;
    for (int i = 0; i < COUNTRY_COUNT; ++i) countries[i].controlled = false;
    countries[playerCountry].controlled = true;
    gameStarted = true;
}

void showGameInfo(const char* message = nullptr) {
    consoleClear();
    int owned = 0;
    for (int i = 0; i < COUNTRY_COUNT; ++i) if (countries[i].controlled) ++owned;
    std::printf("\x1b[1;1H WORLD CONQUEST");
    std::printf("\x1b[3;1H YOUR NATION: %s", countries[playerCountry].name);
    std::printf("\x1b[5;1H CONTROLLED: %d / %d", owned, COUNTRY_COUNT);
    std::printf("\x1b[8;1H SELECTED: %s", countries[selectedCountry].name);
    std::printf("\x1b[10;1H POWER: %d", countries[selectedCountry].power);
    std::printf("\x1b[13;1HUP/DOWN: choose country");
    std::printf("\x1b[15;1HA: show selected country");
    std::printf("\x1b[17;1HX: declare war / attack");
    std::printf("\x1b[19;1HOnly border neighbours can be");
    std::printf("\x1b[20;1Hattacked. White = your territory.");
    std::printf("\x1b[22;1H%s", message ? message : "Conquer the world!");
    std::printf("\x1b[24;1HB: back | START: exit");
}

bool adjacentToPlayer(int target) {
    for (int y = 0; y < MAP_H; ++y) for (int x = 0; x < MAP_W; ++x) {
        int id = countryAt[y][x];
        if (id < 0 || !countries[id].controlled) continue;
        if ((x > 0 && countryAt[y][x-1] == target) ||
            (x+1 < MAP_W && countryAt[y][x+1] == target) ||
            (y > 0 && countryAt[y-1][x] == target) ||
            (y+1 < MAP_H && countryAt[y+1][x] == target)) return true;
    }
    return false;
}

void attackSelected() {
    if (countries[selectedCountry].controlled) { showGameInfo("Already yours."); return; }
    if (!adjacentToPlayer(selectedCountry)) { showGameInfo("Not a neighbour."); return; }
    int strength = 0;
    for (int i = 0; i < COUNTRY_COUNT; ++i)
        if (countries[i].controlled) strength += countries[i].power;
    int attack = strength / 2 + randomInt(45);
    int defense = countries[selectedCountry].power + randomInt(45);
    if (attack >= defense) {
        countries[selectedCountry].controlled = true;
        countries[selectedCountry].power = 55 + randomInt(40);
        showGameInfo("Victory! Country conquered.");
    } else {
        countries[selectedCountry].power += 8;
        showGameInfo("Defence held. Try again.");
    }
}

void selectCountry(int id) {
    selectedCountry = (id + COUNTRY_COUNT) % COUNTRY_COUNT;
    showGameInfo();
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
        } else {
            if (kDown & KEY_B) {
                gameStarted = false;
                for (int i = 0; i < COUNTRY_COUNT; ++i) countries[i].controlled = false;
                playerCountry = -1;
                menuCursor = 0;
                showContinentMenu();
            } else {
                if (kDown & KEY_UP) selectCountry(selectedCountry - 1);
                if (kDown & KEY_DOWN) selectCountry(selectedCountry + 1);
                if (kDown & KEY_A) showGameInfo();
                if (kDown & KEY_X) attackSelected();
            }
        }

        renderFrame();
        gspWaitForVBlank();
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
