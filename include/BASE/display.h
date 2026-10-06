#ifndef HOMM1_BASE_DISPLAY_H
#define HOMM1_BASE_DISPLAY_H

enum LogicalScreenConstant {
    LOGICAL_SCREEN_WIDTH = 640,
    LOGICAL_SCREEN_HEIGHT = 480
};

enum PaletteFormatConstant {
    PALETTE_COLOR_COUNT = 256,
    COLOR_INDEX_MASK = 0xff
};

#define CLIENT_TO_GAME_X(x) (((x) * LOGICAL_SCREEN_WIDTH) / iMainWinScreenWidth)
#define CLIENT_TO_GAME_Y(y) (((y) * LOGICAL_SCREEN_HEIGHT) / gMainWinScreenHeight)

#endif
