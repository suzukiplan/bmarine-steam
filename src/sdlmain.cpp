/**
 * VGS-Zero SDK for Steam/SDL with GPU mode
 * License under GPLv3: https://github.com/suzukiplan/vgszero/blob/master/LICENSE-VGS0.txt
 * (C)2024, SUZUKI PLAN
 */
#include "SDL.h"
#include "gamepkg.h"
#include "../vgszero/src/core/vgs0.hpp"
#include "steam.hpp"
#include "sdlconf.hpp"
#include <chrono>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>

#ifdef DARWIN
#define WINDOW_TITLE "Battle Marine for macOS"
#else
#define WINDOW_TITLE "Battle Marine for Linux"
#endif

extern "C" {
    extern const unsigned int img_err_joypad[17664];
};

static pthread_mutex_t soundMutex = PTHREAD_MUTEX_INITIALIZER;
static bool halt = false;
static CSteam* steam = nullptr;
static VGS0 vgs0;

void log(const char* format, ...)
{
    FILE* fp = fopen("log.txt", "a");
    if (!fp) return;
    char buf[256];
    auto now = time(nullptr);
    auto t = localtime(&now);
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    fprintf(fp, "%04d.%02d.%02d %02d:%02d:%02d %s\n",
           t->tm_year + 1900,
           t->tm_mon + 1,
           t->tm_mday,
           t->tm_hour,
           t->tm_min,
           t->tm_sec,
           buf);
    fclose(fp);
}

static void audioCallback(void* userdata, Uint8* stream, int len)
{
    VGS0* vgs0 = (VGS0*)userdata;
    pthread_mutex_lock(&soundMutex);
    if (halt) {
        pthread_mutex_unlock(&soundMutex);
        return;
    }
    void* buf = vgs0->tickSound(len);
    memcpy(stream, buf, len);
    pthread_mutex_unlock(&soundMutex);
}

static inline unsigned char bit5To8(unsigned char bit5)
{
    bit5 <<= 3;
    bit5 |= (bit5 & 0b11100000) >> 5;
    return bit5;
}

// セーブデータの変更検出（ここでSteamの実績解除やハイスコア登録を行う）
static unsigned char _saveData[16384];
static void check_save_changed(const unsigned char* data, size_t size)
{
    static const char* rankIds[27] = {"E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "W1", "W2", "W3", "W4", "W5", "O1", "O2", "O2+", "O3", "O4", "O5", "O6", "O7", "O8", "O9", "OA", "OB", "XX"};
    if (size == 240 && 0 == memcmp(data, "BM#SCORE", 8)) {
        // 獲得階級をチェックしてSteamへ報告
        for (int i = 0; i < 27; i++) {
            if (data[176 + i * 2] != _saveData[176 + i * 2]) {
                unsigned short count;
                memcpy(&count, &data[176 + i * 2], 2);
                log("Rank %s = %d", rankIds[i], (int)count);
                steam->unlock(rankIds[i]);
            }
        }
        // ハイスコアをSteamへ報告
        int newScore = 0;
        for (int i = 0; i < 8; i++) {
            newScore *= 10;
            newScore += data[16 + (7 - i)];
        }
        newScore *= 10;
        if (0 < newScore) {
            steam->score(newScore);
        }
    }
}

int main(int argc, char* argv[])
{
    unlink("log.txt");
    bool cliError = false;
    int gpuType = SDL_WINDOW_OPENGL;

    for (int i = 1; !cliError && i < argc; i++) {
        switch (tolower(argv[i][1])) {
            case 'g':
                i++;
                if (argc <= i) {
                    cliError = true;
                    break;
                }
                if (0 == strcasecmp(argv[i], "OpenGL")) {
                    gpuType = SDL_WINDOW_OPENGL;
                } else if (0 == strcasecmp(argv[i], "Vulkan")) {
                    gpuType = SDL_WINDOW_VULKAN;
#ifdef DARWIN
                } else if (0 == strcasecmp(argv[i], "Metal")) {
                    gpuType = SDL_WINDOW_METAL;
#endif
                } else if (0 == strcasecmp(argv[i], "None")) {
                    gpuType = 0;
                }
                break;
            case 'h':
                cliError = true;
                break;
            default:
                cliError = true;
                break;
        }
    }
#ifdef DARWIN
    gpuType |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    if (cliError) {
        puts("usage: bmarine [-g { None .............. GPU: Do not use");
        puts("                   | OpenGL ............ GPU: OpenGL <default>");
        puts("                   | Vulkan ............ GPU: Vulkan");
#ifdef DARWIN
        puts("                   | Metal ............. GPU: Metal");
#endif
        puts("                   }]");
        return 1;
    }

    log("Booting %s", WINDOW_TITLE);
    SDL_version sdlVersion;
    SDL_GetVersion(&sdlVersion);
    log("SDL version: %d.%d.%d", sdlVersion.major, sdlVersion.minor, sdlVersion.patch);
    Config cfg;

    steam = new CSteam(log);
    steam->downloadLeaderboardScore = [](std::vector<CSteam::FriendScoreRecord*>* ranking) {
        int ptr = 0xD000 & 0x3FFF;
        int idx = 0;
        for (auto data : *ranking) {
            if (data->globalRank < 100000) {
                sprintf((char*)&vgs0.ctx.ram[ptr], "%5d ", data->globalRank);
            } else {
                sprintf((char*)&vgs0.ctx.ram[ptr], "100K+ ");
            }
            ptr += 6;
            sprintf((char*)&vgs0.ctx.ram[ptr], "%9d %s", data->score, data->name);
            ptr += 26;
            idx++;
            if (10 <= idx) {
                break;
            }
        }
        for (; idx < 10; idx++) {
            sprintf((char*)&vgs0.ctx.ram[ptr], "***** ********* ************");
            ptr += 32;
        }
    };
    steam->init();

    log("Initializing SDL");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        log("SDL_Init failed: %s", SDL_GetError());
        exit(-1);
    }

    // create window & renderer
    SDL_DisplayMode display;
    SDL_GetCurrentDisplayMode(0, &display);
    log("Screen Resolution: width=%d, height=%d", display.w, display.h);
    SDL_Window* window;
    SDL_Renderer* renderer;
    if (cfg.graphic.isFullScreen) {
        gpuType |= SDL_WINDOW_FULLSCREEN;
    }
    if (0 !=SDL_CreateWindowAndRenderer(cfg.graphic.isFullScreen ? display.w : cfg.graphic.windowWidth,
                                        cfg.graphic.isFullScreen ? display.h : cfg.graphic.windowHeight,
                                        gpuType,
                                        &window,
                                        &renderer)) {
        log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        exit(-1);
    }
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_RenderPresent(renderer);

    double sw = (cfg.graphic.isFullScreen ? display.w : cfg.graphic.windowWidth) / 480.0;
    double sh = (cfg.graphic.isFullScreen ? display.h : cfg.graphic.windowHeight) / 384.0;
    double scale = sw < sh ? sw : sh;
    int frameWidth = (int)((cfg.graphic.isFullScreen ? display.w : cfg.graphic.windowWidth) / scale);
    int frameHeight = (int)((cfg.graphic.isFullScreen ? display.h : cfg.graphic.windowHeight) / scale);
    int framePitch = frameWidth * 4;
    int offsetX = (frameWidth - 480) / 2;
    int offsetY = (frameHeight - 384) / 2;
    int maskTR = cfg.graphic.isScanline ? 0xF0F0F0F0 : 0xFFFFFFFF;
    int maskBL = cfg.graphic.isScanline ? 0x8F8F8F8F : 0xFFFFFFFF;
    int maskBR = cfg.graphic.isScanline ? 0x80808080 : 0xFFFFFFFF;
    log("Texture: width=%d, height=%d, offsetX=%d, offsetY=%d", frameWidth, frameHeight, offsetX, offsetY);
    SDL_RenderSetLogicalSize(renderer, frameWidth, frameHeight);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, frameWidth, frameHeight);
    if (!texture) {
        log("SDL_CreateTexture failed: %s", SDL_GetError());
        exit(-1);
    }
    auto frameBuffer = (unsigned int*)malloc(framePitch * frameHeight);
    if (!frameBuffer) {
        log("No memory");        
        exit(-1);
    }
    memset(frameBuffer, 0, framePitch * frameHeight);

    log("Initializing VGS-Zero");
    int romSize;
    const void* rom = nullptr;
    int bgmSize;
    const void* bgm = nullptr;
    int seSize;
    const void* se = nullptr;
    const unsigned char* ptr = gamepkg;
    if (0 != memcmp(ptr, "VGS0PKG", 8)) {
        puts("Invalid package!");
        return -1;
    }
    ptr += 8;
    memcpy(&romSize, ptr, 4);
    ptr += 4;
    rom = ptr;
    ptr += romSize;
    printf("- game.rom size: %d\n", romSize);
    if (romSize < 8 + 8192) {
        puts("Invalid game.rom size");
        exit(-1);
    }
    memcpy(&bgmSize, ptr, 4);
    ptr += 4;
    bgm = 0 < bgmSize ? ptr : nullptr;
    ptr += bgmSize;
    printf("- bgm.dat size: %d\n", bgmSize);
    memcpy(&seSize, ptr, 4);
    ptr += 4;
    se = 0 < seSize ? ptr : nullptr;
    printf("- se.dat size: %d\n", seSize);

    if (0 < bgmSize) {
        vgs0.loadBgm(bgm, bgmSize);
    }
    if (0 < seSize) {
        vgs0.loadSoundEffect(se, seSize);
    }
    vgs0.loadRom(rom, romSize);
    vgs0.setBgmVolume(cfg.sound.volumeBgm);
    vgs0.setSeVolume(cfg.sound.volumeSe);

    mkdir("save",  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH | S_IXOTH);
    vgs0.saveCallback = [](VGS0* vgs0, const void* data, size_t size) -> bool {
        log("Saving save.dat (%lubytes)", size);
        FILE* fp = fopen("save/save.dat", "wb");
        if (!fp) {
            log("File open error!");
            return false;
        }
        if (size != fwrite(data, 1, size, fp)) {
            log("File write error!");
            fclose(fp);
            return false;
        }
        fclose(fp);
        check_save_changed((const unsigned char*)data, size);
        memcpy(_saveData, data, size);
        return true;
    };

    vgs0.loadCallback = [](VGS0* vgs0, void* data, size_t size) -> bool {
        log("Loading save.dat (%lubytes)", size);
        FILE* fp = fopen("save/save.dat", "rb");
        if (!fp) {
            log("File open error!");
            return false;
        }
        size_t readSize = fread(data, 1, size, fp);
        if (readSize < size) {
            log("warning: file size is smaller than expected");
            memset(&((char*)data)[readSize], 0, size - readSize);
        }
        fclose(fp);
        memcpy(_saveData, data, readSize);
        return true;
    };

    log("Initializing AudioDriver");
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    desired.freq = 44100;
    desired.format = AUDIO_S16LSB;
    desired.channels = 1;
    desired.samples = 735; // desired.freq * 20 / 1000;
    desired.callback = audioCallback;
    desired.userdata = &vgs0;
    auto audioDeviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (0 == audioDeviceId) {
        log(" ... SDL_OpenAudioDevice failed: %s", SDL_GetError());
        exit(-1);
    }
    log("- obtained.freq = %d", obtained.freq);
    log("- obtained.format = %X", obtained.format);
    log("- obtained.channels = %d", obtained.channels);
    log("- obtained.samples = %d", obtained.samples);
    SDL_PauseAudioDevice(audioDeviceId, 0);

    log("Start main loop...");
    SDL_Event event;
    unsigned int loopCount = 0;
    const int waitFps60[3] = {17000, 17000, 16000};
    unsigned char msxKeyCodeMap[16];
    memset(msxKeyCodeMap, 0, sizeof(msxKeyCodeMap));
    bool joypadConnected = false;
    bool joypadConnectedPrev = false;
    bool detectJoypadDisconnected = false;
    unsigned char key1 = 0;

    while (!halt) {
        auto start = std::chrono::system_clock::now();
        loopCount++;
        if (loopCount % 6 == 0) {
            SteamAPI_RunCallbacks();
        }

        // Keyboard Input (SDL2)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                halt = true;
            } else if (event.type == SDL_KEYDOWN) {
                if (cfg.keyboard.quit == event.key.keysym.sym) {
                    halt = true;                    
                }
                if (cfg.keyboard.up == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_UP;
                }
                if (cfg.keyboard.down == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_DW;
                }
                if (cfg.keyboard.left == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_LE;
                }
                if (cfg.keyboard.right == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_RI;
                }
                if (cfg.keyboard.a == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_T1;
                }
                if (cfg.keyboard.b == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_T2;
                }
                if (cfg.keyboard.start == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_ST;
                }
                if (cfg.keyboard.start == event.key.keysym.sym) {
                    key1 |= VGS0_JOYPAD_SE;
                }
                if (cfg.keyboard.reset == event.key.keysym.sym) {
                    log("Reset");
                    vgs0.reset();
                }
            } else if (event.type == SDL_KEYUP) {
                if (cfg.keyboard.up == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_UP;
                }
                if (cfg.keyboard.down == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_DW;
                }
                if (cfg.keyboard.left == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_LE;
                }
                if (cfg.keyboard.right == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_RI;
                }
                if (cfg.keyboard.a == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_T1;
                }
                if (cfg.keyboard.b == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_T2;
                }
                if (cfg.keyboard.start == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_ST;
                }
                if (cfg.keyboard.start == event.key.keysym.sym) {
                    key1 ^= VGS0_JOYPAD_SE;
                }
            }
        }
        if (halt) {
            break;
        }

        // SteamInput
        auto pad1 = steam->getJoypad(&joypadConnected);
        if (joypadConnected) {
            if (!joypadConnectedPrev) {
                log("Joypad Connected!");
            }
            detectJoypadDisconnected = false;
        } else if (joypadConnectedPrev) {
            log("Joypad Disconnected! (waiting for resume...)");
            detectJoypadDisconnected = true;
            auto fb = frameBuffer;
            int ox = (frameWidth - 368) / 2;
            fb += (frameHeight - 48) / 2 * frameWidth;
            int ptr = 0;
            for (int y = 0; y < 48; y++, fb += frameWidth) {
                for (int x = 0; x < 368; x++) {
                    fb[ox + x] = img_err_joypad[ptr++];
                }
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
            SDL_UpdateTexture(texture, nullptr, frameBuffer, framePitch);
            SDL_SetRenderTarget(renderer, nullptr);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            usleep(20000);
            continue;
        } else if (detectJoypadDisconnected) {
            usleep(20000);
            continue;
        }
        joypadConnectedPrev = joypadConnected;

        // execute emulator 1 frame
        if (!steam->isOverlay()) {
            pthread_mutex_lock(&soundMutex);
            vgs0.tick(key1 | pad1);
            pthread_mutex_unlock(&soundMutex);
            if (vgs0.cpu->reg.IFF & 0x80) {
                if (0 == (vgs0.cpu->reg.IFF & 0x01)) {
                    log("Detected the HALT while DI");
                    break;
                }
            }
        }

        // render graphics
        auto vgsDisplay = vgs0.getDisplay();
        auto pcDisplay = (unsigned int*)frameBuffer;
        pcDisplay += offsetY * frameWidth;
        for (int y = 0; y < 192; y++) {
            for (int x = 0; x < 240; x++) {
                unsigned int rgb555 = vgsDisplay[x];
                unsigned int rgb888 = 0;
                rgb888 |= bit5To8((rgb555 & 0b0111110000000000) >> 10);
                rgb888 <<= 8;
                rgb888 |= bit5To8((rgb555 & 0b0000001111100000) >> 5);
                rgb888 <<= 8;
                rgb888 |= bit5To8(rgb555 & 0b0000000000011111);
                rgb888 <<= 8;
                auto offset = offsetX + x * 2;
                pcDisplay[offset] = rgb888;
                pcDisplay[offset + 1] = rgb888 & maskTR;
                pcDisplay[offset + frameWidth] = rgb888 & maskBL;
                pcDisplay[offset + frameWidth + 1] = rgb888 & maskBR;
            }
            vgsDisplay += 240;
            pcDisplay += frameWidth * 2;
        }

        // render display
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
        SDL_UpdateTexture(texture, nullptr, frameBuffer, framePitch);
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // sync 60fps
        std::chrono::duration<double> diff = std::chrono::system_clock::now() - start;
        int us = (int)(diff.count() * 1000000);
        int wait = waitFps60[loopCount % 3];
        if (us < wait) {
            usleep(wait - us);
        }
    }

    cfg.save();

    log("Terminating");
    delete steam;
    SDL_Quit();
    free(frameBuffer);
    return 0;
}
