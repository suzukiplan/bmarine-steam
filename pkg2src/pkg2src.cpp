#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "pkg2src /path/to/game.pkg\n");
        return -1;
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "file open error\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    FILE* fpC = fopen("gamepkg.c", "wt");
    FILE* fpH = fopen("gamepkg.h", "wt");

    fprintf(fpH, "#pragma once\n\n");
    fprintf(fpH, "extern \"C\" {\n    extern const unsigned char gamepkg[%d];\n}\n", size);
    fprintf(fpC, "const unsigned char gamepkg[%d] = {\n", size);
    bool firstLine = true;
    while (1) {
        unsigned char buf[16];
        int readSize = (int)fread(buf, 1, sizeof(buf), fp);
        if (readSize < 1) {
            fprintf(fpC, "\n");
            break;
        }
        if (firstLine) {
            firstLine = false;
        } else {
            fprintf(fpC, ",\n");
        }
        fprintf(fpC, "    ");
        for (int i = 0; i < readSize; i++) {
            if (i) {
                fprintf(fpC, ", 0x%02X", buf[i]);
            } else {
                fprintf(fpC, "0x%02X", buf[i]);
            }
        }
    }
    fprintf(fpC, "};\n");
    fclose(fp);
    return 0;
}
