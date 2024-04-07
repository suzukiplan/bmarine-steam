================================================================================
Battle Marine for Steam

Porgrammed by Yoji Suzuki
Copyright (C) 2024, SUZUKIPLAN
================================================================================

Thank you for purchasing the Battle Marine for Steam.
This document describes how to play this game and other information, so please read it before playing.

Description

Battle Marine is a simple naval barrage STG.
Enjoy a somewhat retro yet new and exhilarating game experience by firing a barrage of shots, inducing explosions, sweeping with powerful lasers, and automatically collecting medals (recovery and score) by jumping.
Destroying enemies consecutively within a certain time frame will result in a HIT combo. Aim for 999 HITs!
Your rank will be assessed according to the results of your play. Aim for the rank of FLEET ADMIRAL!

Controls

This game can be played with a gamepad or keyboard.
Button assignments can be freely customized in the SteamInput menu.

Keyboard Controls (Windows)

- Cursor key: D-Pad (Can be changed in the key configuration)
- X key: A button (Can be changed in the key configuration)
- Z key: B button (Can be changed in the key configuration)
- Space key: START button (Can be changed in the key configuration)
- ESC key: SELECT button (Can be changed in the key configuration)
- CTRL+R: Reset the game
- CTRL+X: Exit the game (CTRL+Q, CTRL+C also work)
- Alt+Enter: Toggle full screen (CTRL+F also work)
- CTRL+A: Toggle screen aspect mode
- CTRL+S: Screen Resolution Settings
- CTRL+K: Key configuration for the Keyboard

Keyboard Controls (Linux/macOS)

- Cursor key: D-Pad
- X key: A button
- Z key: B button
- Space key: START button
- ESC key: SELECT button
- R: Reset the game
- Q: Exit the game
- R: Reset

Notes on the Linux/macOS version

When you launch Battle Marine, a configuration file called config.json will be generated.
The config.json file is saved under the directory opened by opening the Battle Marine page in the Steam client and selecting "Settings Icon" > "Manage" > "Browse local files".
You can customize the following items:
- graphic.isFullScreen: true = full screen, false = window mode
- graphic.isScanline: true = with screen effects, false = without screen effects
- gaphic.windowHeight: height of the screen in window mode
- gaphic.windowWidth: width of the screen in window mode
- sound.volumeBgm: Volume of background music (0-100)
- sound.volumeSe: Volume of sound effect (0-100)
- keyboard.a: Key code for A button (jump)
- keyboard.b: key code for B button (fire)
- keyboard.start: key code for START button (pause)
- keyboard.select: key code for SELECT button (n/a)
- keyboard.up: key code for up button
- keyboard.down: key code for down button
- keyboard.left: Key code for left button
- keyboard.right: Key code for right button
- keyboard.reset: Reset button key code
- keyboard.quit: Key code for quit button

NOTE: The keycode specifies the SDL keycode.
https://wiki.libsdl.org/SDL2/SDLKeycodeLookup

How to Play

- When you destroy an enemy, a green or red medal will appear.
- Taking the green medal will restore your LIFE.
- Your score will increase if you take a red medal.
- If you keep defeating enemies within a certain time (120 frames), your combo will increase and medals will be collected automatically when you reach 100 HITs or more.
- Medals are also automatically collected by jumping.
- You are invincible during the laser (after the firing motion is complete).
- Unlike normal STGs, explosions also have hit points.
- If you keep collecting green medals while your LIFE is full, the red medal's base points will continue to increase.
- The number of crowns at the bottom of the screen will increase for every 16 green medals you take while your LIFE is full.
- The maximum number of crowns displayed is 30 (480 green medals), but the maximum number of crowns is 4,095 (65,535 green medals).
- When you collide with enemy bullets, your LIFE will be reduced and the number of crowns will be strongly reduced.
- With the exception of some enemies (crabs), red medals will appear when enemies are killed by lasers.
- Obtaining a medal also resets the combo duration to 120 frames

Try it for FREE

The Steam version of Battle Marine is only available for a fee,
but if you have a Raspberry Pi Zero 2W you can try the Raspberry Pi version for FREE.
If you like the Raspberry Pi version, please consider purchasing the Steam version.
https://github.com/suzukiplan/bmarine-zero

> We have a policy of NOT ISSUING ANY FREE KEYS for Steam since we are giving away the RasPi version for FREE.

Video distribution, etc.

There are no restrictions on the distribution of this game on TV, YouTube, or other video distribution sites.
Of course, there are no restrictions on monetization.
There is no need to contact the author prior to or after the event.
Similarly, there are no restrictions on publishing screenshots in magazines, web media, social media, etc.

Contact us

1. Twitter: https://twitter.com/suzukiplan
2. GitHub: https://github.com/suzukiplan/bmarine-steam/issues

Licenses

This product uses the following Open Source Software:

1. PicoJSON - a C++ JSON parser / serializer (LICENSE-PICOJSON.txt)
2. Simple DirectMedia Layer (LICENSE-SDL.txt)
3. SUZUKIPLAN - Z80 Emulator (LICENSE-Z80.txt)
4. VGS-Zero Library for Z80 (LICENSE-VGS0LIB.txt)
5. VGS-Zero (LICENSE-VGS0.txt)
