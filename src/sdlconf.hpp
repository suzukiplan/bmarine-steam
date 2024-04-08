/**
 * VGS-Zero SDK for Steam - Configuration for Linux and macOS
 * License under GPLv3: https://github.com/suzukiplan/vgszero/blob/master/LICENSE-VGS0.txt
 * (C)2024, SUZUKI PLAN
 */
#pragma once
#include "SDL.h"
#include "picojson.h"
#include <fstream>
#include <iostream>
#include <ctype.h>
#include <string.h>

void log(const char* format, ...);

class Config {
public:
    struct Graphic {
        int windowWidth;
        int windowHeight;
        bool isFullScreen;
        bool isScanline;
    } graphic;

    struct Sound {
        int volumeBgm;
        int volumeSe;
    } sound;

    struct Keyboard {
        int up;
        int down;
        int left;
        int right;
        int a;
        int b;
        int autoB;
        int select;
        int start;
        int reset;
        int quit;
    } keyboard;

    Config()
    {
        graphic.windowWidth = 480;
        graphic.windowHeight = 384;
        graphic.isFullScreen = true;
        graphic.isScanline = true;
        sound.volumeBgm = 100;
        sound.volumeSe = 100;
        keyboard.up = SDLK_UP;
        keyboard.down = SDLK_DOWN;
        keyboard.left = SDLK_LEFT;
        keyboard.right = SDLK_RIGHT;
        keyboard.select = SDLK_ESCAPE;
        keyboard.start = SDLK_SPACE;
        keyboard.b = SDLK_z;
        keyboard.a = SDLK_x;
        keyboard.autoB = SDLK_c;
        keyboard.reset = SDLK_r;
        keyboard.quit = SDLK_q;
        load();
        dump();
    }

    void dump()
    {
        log("- graphic.windowWidth: %d", graphic.windowWidth);
        log("- graphic.windowHeight: %d", graphic.windowHeight);
        log("- graphic.isFullScreen: %s", graphic.isFullScreen ? "true" : "false");
        log("- graphic.isScanline: %s", graphic.isScanline ? "true" : "false");
        log("- sound.volumeBgm: %d", sound.volumeBgm);
        log("- sound.volumeSe: %d", sound.volumeSe);
        log("- keyboard.up: 0x%X", keyboard.up);
        log("- keyboard.down: 0x%X", keyboard.down);
        log("- keyboard.left: 0x%X", keyboard.left);
        log("- keyboard.right: 0x%X", keyboard.right);
        log("- keyboard.a: 0x%X", keyboard.a);
        log("- keyboard.b: 0x%X", keyboard.b);
        log("- keyboard.autoB: 0x%X", keyboard.autoB);
        log("- keyboard.start: 0x%X", keyboard.start);
        log("- keyboard.select: 0x%X", keyboard.select);
        log("- keyboard.reset: 0x%X", keyboard.reset);
        log("- keyboard.quit: 0x%X", keyboard.quit);
    }

    void save()
    {
        log("Saving config.json");
        picojson::value j;
        picojson::object o;
        picojson::object graphicJson;
        picojson::object soundJson;
        picojson::object keyboardJson;

        graphicJson.insert(std::make_pair("windowWidth", picojson::value((double)graphic.windowWidth)));
        graphicJson.insert(std::make_pair("windowHeight", picojson::value((double)graphic.windowHeight)));
        graphicJson.insert(std::make_pair("isFullScreen", picojson::value(graphic.isFullScreen)));
        graphicJson.insert(std::make_pair("isScanline", picojson::value(graphic.isScanline)));
        o.insert(std::make_pair("graphic", graphicJson));

        soundJson.insert(std::make_pair("volumeBgm", picojson::value((double)sound.volumeBgm)));
        soundJson.insert(std::make_pair("volumeSe", picojson::value((double)sound.volumeSe)));
        o.insert(std::make_pair("sound", soundJson));

        keyboardJson.insert(std::make_pair("#hint", picojson::value("SDLKeycodeLookup")));        
        keyboardJson.insert(std::make_pair("up", picojson::value(toString(keyboard.up))));
        keyboardJson.insert(std::make_pair("down", picojson::value(toString(keyboard.down))));
        keyboardJson.insert(std::make_pair("left", picojson::value(toString(keyboard.left))));
        keyboardJson.insert(std::make_pair("right", picojson::value(toString(keyboard.right))));
        keyboardJson.insert(std::make_pair("a", picojson::value(toString(keyboard.a))));
        keyboardJson.insert(std::make_pair("b", picojson::value(toString(keyboard.b))));
        keyboardJson.insert(std::make_pair("autoB", picojson::value(toString(keyboard.autoB))));
        keyboardJson.insert(std::make_pair("start", picojson::value(toString(keyboard.start))));
        keyboardJson.insert(std::make_pair("select", picojson::value(toString(keyboard.select))));
        keyboardJson.insert(std::make_pair("reset", picojson::value(toString(keyboard.reset))));
        keyboardJson.insert(std::make_pair("quit", picojson::value(toString(keyboard.quit))));
        o.insert(std::make_pair("keyboard", keyboardJson));

        try {
            std::ofstream ofs("config.json");
            ofs << picojson::value(o).serialize(true) << std::endl;
        } catch (...) {
            log("Save failed! (continue without save config.json)");
        }
    }

    int hex2dec(char c)
    {
        switch (tolower(c)) {
            case '0': return 0;
            case '1': return 1;
            case '2': return 2;
            case '3': return 3;
            case '4': return 4;
            case '5': return 5;
            case '6': return 6;
            case '7': return 7;
            case '8': return 8;
            case '9': return 9;
            case 'a': return 10;
            case 'b': return 11;
            case 'c': return 12;
            case 'd': return 13;
            case 'e': return 14;
            case 'f': return 15;
            default: return -1;
        }
    }

    std::string toString(int i)
    {
        char buf[80];
        snprintf(buf, sizeof(buf), "0x%X", i);
        std::string result = buf;
        return result;
    }

    int toKeyCode(const char* str) {
        int result = 0;
        if (0 == strncmp(str, "0x", 2)) {
            str += 2;
            while (*str) {
                int d = hex2dec(*str);
                if (d < 0) {
                    break;
                }
                result <<= 4;
                result |= d;
                str++;
            }
        }
        return result;
    }

    void load()
    {
        log("Loading config.json");
        std::ifstream ifs("config.json", std::ios::in);
        if (ifs.fail()) {
            log("File not found (use default settings)");
            save();
            return;
        }
        const std::string json((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        picojson::value v;
        const std::string err = picojson::parse(v, json);
        if (err.empty() == false) {
            log(("Detected error: " + err).c_str());
            return;
        }
        auto obj = v.get<picojson::object>();

        auto graphicJson = obj["graphic"].get<picojson::object>();
        if (graphicJson.find("windowWidth")->second.is<double>()) {
            graphic.windowWidth = (int)graphicJson["windowWidth"].get<double>();
            if (graphic.windowWidth < 240) {
                graphic.windowWidth = 240;
            }
        }

        if (graphicJson.find("windowHeight")->second.is<double>()) {
            graphic.windowHeight = (int)graphicJson["windowHeight"].get<double>();
            if (graphic.windowHeight < 192) {
                graphic.windowHeight = 192;
            }
        }

        if (graphicJson.find("isFullScreen")->second.is<bool>()) {
            graphic.isFullScreen = graphicJson["isFullScreen"].get<bool>();
        }

        if (graphicJson.find("isScanline")->second.is<bool>()) {
            graphic.isScanline = graphicJson["isScanline"].get<bool>();
        }

        auto soundJson = obj["sound"].get<picojson::object>();
        if (soundJson.find("volumeBgm")->second.is<double>()) {
            sound.volumeBgm = (int)soundJson["volumeBgm"].get<double>();
            if (sound.volumeBgm < 0) {
                sound.volumeBgm = 0;
            } else if (100 < sound.volumeBgm) {
                sound.volumeBgm = 100;
            }
        }

        if (soundJson.find("volumeSe")->second.is<double>()) {
            sound.volumeSe = (int)soundJson["volumeSe"].get<double>();
            if (sound.volumeSe < 0) {
                sound.volumeSe = 0;
            } else if (100 < sound.volumeSe) {
                sound.volumeSe = 100;
            }
        }

        auto keyboardJson = obj["keyboard"].get<picojson::object>();
        if (keyboardJson.find("up")->second.is<double>()) {
            keyboard.up = (int)keyboardJson["up"].get<double>();
        } else if (keyboardJson.find("up")->second.is<std::string>()) {
            keyboard.up = toKeyCode(keyboardJson["up"].get<std::string>().c_str());
        }

        if (keyboardJson.find("down")->second.is<double>()) {
            keyboard.down = (int)keyboardJson["down"].get<double>();
        } else if (keyboardJson.find("down")->second.is<std::string>()) {
            keyboard.down = toKeyCode(keyboardJson["down"].get<std::string>().c_str());
        }

        if (keyboardJson.find("left")->second.is<double>()) {
            keyboard.left = (int)keyboardJson["left"].get<double>();
        } else if (keyboardJson.find("left")->second.is<std::string>()) {
            keyboard.left = toKeyCode(keyboardJson["left"].get<std::string>().c_str());
        }

        if (keyboardJson.find("right")->second.is<double>()) {
            keyboard.right = (int)keyboardJson["right"].get<double>();
        } else if (keyboardJson.find("right")->second.is<std::string>()) {
            keyboard.right = toKeyCode(keyboardJson["right"].get<std::string>().c_str());
        }

        if (keyboardJson.find("a")->second.is<double>()) {
            keyboard.a = (int)keyboardJson["a"].get<double>();
        } else if (keyboardJson.find("a")->second.is<std::string>()) {
            keyboard.a = toKeyCode(keyboardJson["a"].get<std::string>().c_str());
        }

        if (keyboardJson.find("b")->second.is<double>()) {
            keyboard.b = (int)keyboardJson["b"].get<double>();
        } else if (keyboardJson.find("b")->second.is<std::string>()) {
            keyboard.b = toKeyCode(keyboardJson["b"].get<std::string>().c_str());
        }

        if (keyboardJson.find("autoB")->second.is<double>()) {
            keyboard.autoB = (int)keyboardJson["autoB"].get<double>();
        } else if (keyboardJson.find("autoB")->second.is<std::string>()) {
            keyboard.autoB = toKeyCode(keyboardJson["autoB"].get<std::string>().c_str());
        }

        if (keyboardJson.find("start")->second.is<double>()) {
            keyboard.start = (int)keyboardJson["start"].get<double>();
        } else if (keyboardJson.find("start")->second.is<std::string>()) {
            keyboard.start = toKeyCode(keyboardJson["start"].get<std::string>().c_str());
        }

        if (keyboardJson.find("select")->second.is<double>()) {
            keyboard.select = (int)keyboardJson["select"].get<double>();
        } else if (keyboardJson.find("select")->second.is<std::string>()) {
            keyboard.select = toKeyCode(keyboardJson["select"].get<std::string>().c_str());
        }

        if (keyboardJson.find("reset")->second.is<double>()) {
            keyboard.reset = (int)keyboardJson["reset"].get<double>();
        } else if (keyboardJson.find("reset")->second.is<std::string>()) {
            keyboard.reset = toKeyCode(keyboardJson["reset"].get<std::string>().c_str());
        }

        if (keyboardJson.find("quit")->second.is<double>()) {
            keyboard.quit = (int)keyboardJson["quit"].get<double>();
        } else if (keyboardJson.find("quit")->second.is<std::string>()) {
            keyboard.quit = toKeyCode(keyboardJson["quit"].get<std::string>().c_str());
        }
    }
};
