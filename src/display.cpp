#include "display.h"
#include "midi_controller.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static const unsigned char logo_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0xe0,0xff,0xff,0x0f,0x20,0x00,0x40,0x08,0x38,0x00,0x40,0x08,0x08,0x00,0x40,0x08,0x0e,0x00,0x40,0x08,0x82,0xff,0xff,0x0f,0xc2,0xf9,0xff,0x0f,0xc2,0xf9,0xff,0x0f,0x42,0xe0,0xe7,0x0c,0x42,0xe0,0x4b,0x09,0xc2,0xf9,0x43,0x08,0xc2,0x49,0xe6,0x0c,0xfe,0xff,0xff,0x0f,0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0x0f,0x86,0x63,0x38,0x0c,0x56,0x5d,0xd7,0x0f,0x56,0x5d,0x17,0x0c,0x56,0x5d,0xf7,0x0d,0x56,0x63,0x18,0x0e,0xfe,0xff,0xff,0x0f,0x00,0x00,0x00,0x00};
static const unsigned char rusaakkmods_bits[] U8X8_PROGMEM = {0x00,0x00,0x80,0x10,0x00,0x00,0x02,0x00,0x00,0x80,0x10,0x00,0x00,0x02,0x97,0xcc,0xb9,0x94,0x6e,0x8c,0x33,0x91,0x22,0xa5,0x52,0x92,0x52,0x0a,0x91,0x24,0xa5,0x31,0x92,0x52,0x12,0x91,0x28,0xa5,0x52,0x92,0x52,0x22,0xe1,0xc6,0xb9,0x94,0x92,0x8c,0x1b};
static const unsigned char main_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4e,0x22,0xfc,0x3f,0x4e,0x32,0xfc,0x3f,0x92,0x49,0xfc,0x3f,0x92,0x71,0xfc,0x3f,0x52,0x32,0xfc,0x3f,0x52,0x4a,0xfc,0x3f,0x52,0x4a,0xfc,0x3f,0x56,0x22,0xfc,0x3f,0x52,0x22,0xfc,0x3f,0x52,0x42,0xfc,0x3f,0x52,0x4a,0xfc,0x3f,0x5a,0x22,0xfc,0x3f,0x4e,0x22,0xfc,0x3f,0x4e,0x22,0xfc,0x3f,0xde,0x4b,0xfc,0x3f,0x52,0x22,0xfc,0x3f,0x42,0x22,0xfc,0x3f,0x42,0x12,0xfc,0x3f,0x5e,0x2a,0xfc,0x3f,0x52,0x22,0xfc,0x3f,0x82,0x71,0xfc,0x3f,0x82,0x79,0xfc,0x3f,0x52,0x12,0xfc,0x3f,0x92,0x71,0xfc,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x06,0x00,0x00,0x30,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x1e,0x00,0x00,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x00,0x00,0xda,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x00,0x00,0xda,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x00,0x00,0xea,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x00,0x00,0xf6,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x00,0x00,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char microboy_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0xff,0xff,0xff,0xff,0x3f,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0xfc,0x0f,0xfc,0xf8,0x23,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0xf0,0x03,0xf0,0xf8,0x23,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0xe0,0x01,0xe0,0xf8,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xf8,0xe3,0xf1,0xe3,0xf8,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xf8,0xc7,0xf8,0xc7,0xf8,0x23,0xff,0x07,0xfc,0x00,0xf0,0x03,0x87,0x03,0xf0,0x83,0xf8,0xc7,0xf8,0xc7,0xf8,0x23,0xff,0x1f,0xfc,0x00,0xfc,0x0f,0xe7,0x0f,0xfc,0x8f,0xf8,0xc7,0xf8,0xc7,0xf8,0x23,0xff,0x3f,0xfc,0x00,0xfe,0x1f,0xf7,0x1f,0xfe,0x9f,0xf8,0xe1,0xf8,0xc7,0xf0,0x21,0xc7,0x39,0xe0,0x00,0x0e,0x1c,0x7f,0x1c,0x0e,0x9c,0x00,0xf0,0xf8,0xc7,0xe1,0x30,0xc7,0x71,0xe0,0x00,0x07,0x38,0x1f,0x38,0x07,0xb8,0x00,0xf8,0xf8,0xc7,0x43,0x38,0xc7,0x71,0xe0,0x00,0x07,0x38,0x0f,0x38,0x07,0xb8,0x00,0xf0,0xf8,0xc7,0x07,0x3c,0xc7,0x71,0xe0,0x00,0x07,0x00,0x07,0x00,0x07,0xb8,0xf8,0xe1,0xf8,0xc7,0x0f,0x3e,0xc7,0x71,0xe0,0x00,0x07,0x00,0x07,0x00,0x07,0xb8,0xf8,0xc7,0xf8,0xc7,0x1f,0x3f,0xc7,0x71,0xe0,0x00,0x07,0x00,0x07,0x00,0x07,0xb8,0xf8,0xc7,0xf8,0xc7,0x1f,0x3f,0xc7,0x71,0xe0,0x00,0x07,0x00,0x07,0x00,0x07,0xb8,0xf8,0xc7,0xf8,0xc7,0x1f,0x3f,0xc7,0x71,0xe0,0x00,0x0e,0x00,0x07,0x00,0x0e,0x9c,0xf8,0xe3,0xf1,0xe3,0x1f,0x3f,0xc7,0x71,0xfc,0x07,0xfe,0x3f,0x07,0x00,0xfe,0x9f,0x00,0xe0,0x01,0xe0,0x1f,0x3f,0xc7,0x71,0xfc,0x07,0xfc,0x3f,0x07,0x00,0xfc,0x8f,0x00,0xf0,0x03,0xf0,0x1f,0x3f,0xc7,0x71,0xfc,0x07,0xf0,0x3f,0x07,0x00,0xf0,0x83,0x00,0xfc,0x0f,0xfc,0x1f,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0xff,0xff,0xff,0xff,0x3f};
static const unsigned char right_bits[] U8X8_PROGMEM = {0x80,0x0d,0xc0,0x06,0x60,0x03,0xb0,0x01,0xd8,0x00,0x6c,0x00,0x36,0x00,0x1b,0x00,0x1b,0x00,0x36,0x00,0x6c,0x00,0xd8,0x00,0xb0,0x01,0x60,0x03,0xc0,0x06,0x80,0x0d};
static const unsigned char left_bits[] U8X8_PROGMEM = {0x1b,0x00,0x36,0x00,0x6c,0x00,0xd8,0x00,0xb0,0x01,0x60,0x03,0xc0,0x06,0x80,0x0d,0x80,0x0d,0xc0,0x06,0x60,0x03,0xb0,0x01,0xd8,0x00,0x6c,0x00,0x36,0x00,0x1b,0x00};

// only use these two fonts to conserve flash storage
#define FONT_SMALL u8g2_font_profont10_tr
#define FONT_LARGE u8g2_font_profont17_tr

#define NUM_CURSORS 5
Cursor cursors[NUM_CURSORS] = {
    {6, 9, 19, 13, RANGE_1_16, SOLO},
    {38, 9, 19, 13, RANGE_1_16, SOLO},
    {70, 9, 19, 13, RANGE_1_16, SOLO},
    {102, 9, 19, 13, RANGE_1_16, SOLO},
    {112, 24, 15, 8, RANGE_0_127, MUTE}
};

#define NUM_CHANNEL_SUBMENU 4
SubMenu channelSubMenus[NUM_CHANNEL_SUBMENU] = {
    {"PU1", RANGE_1_16, &config.outputChannel[0]}, 
    {"PU2", RANGE_1_16, &config.outputChannel[1]}, 
    {"WAV", RANGE_1_16, &config.outputChannel[2]},
    {"NOI", RANGE_1_16, &config.outputChannel[3]}
};

#define NUM_MIDI_SUBMENU 5
SubMenu midiSubMenus[NUM_MIDI_SUBMENU] = {
    {"VELOCITY", RANGE_0_127, &config.velocity},
    {"MIDI CC", ON_OFF, &config.ccEnabled}, 
    {"MIDI PC", ON_OFF, &config.pcEnabled}, 
    {"REALTIME", ON_OFF, &config.realTimeEnabled}, 
    {"CLOCK", ON_OFF, &config.clockEnabled}
};

#define NUM_READER_SUBMENU 2
SubMenu readerSubMenus[NUM_READER_SUBMENU] = {
    {"BYTE DELAY", RANGE_1000_5000_BY_100, &config.byteDelay},
    {"EXP.CORCT", ON_OFF, &config.experimentalCorrectionEnabled}
};

#define NUM_CONFIG_SUBMENU 2
SubMenu configSubMenus[NUM_CONFIG_SUBMENU] = {
    {"SAVE", SAVE_CONFIG, nullptr},
    {"DEFAULT", LOAD_DEFAULT, nullptr}
};

#define NUM_ABOUT_SUBMENU 3
SubMenu aboutSubMenus[NUM_ABOUT_SUBMENU] = {
    {"FIRMWARE", ABOUT, nullptr, _VERSION},
    {"AUTHOR", ABOUT, nullptr, _AUTHOR},
    {"GITHUB", ABOUT, nullptr, _GITHUB}
};

MainMenu mainMenus[NUM_MAIN_MENU] = {
    {"CHANNELS", NUM_CHANNEL_SUBMENU, channelSubMenus},
    {"MIDI", NUM_MIDI_SUBMENU,  midiSubMenus},
    {"READER", NUM_READER_SUBMENU, readerSubMenus},
    {"CONFIG", NUM_CONFIG_SUBMENU, configSubMenus},
    {"ABOUT", NUM_ABOUT_SUBMENU, aboutSubMenus}
};

Display display;

void display_load()
{
    display.currentState = MAIN_DISPLAY;
    display.mainCursorIndex = 0;
    display.mainCursors = cursors;
    display.cursorSize = NUM_CURSORS;
    display.menuIndex = 0;
    display.submenuIndex = 0;
    display.mainMenus = mainMenus;
    display.mainMenuSize = NUM_MAIN_MENU;
}

void display_logo()
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.drawXBMP(21, 4, 29, 23, logo_bits);
    u8g2.drawXBMP(52, 7, 54, 7, rusaakkmods_bits);
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(53, 23, _YEAR);
    u8g2.setDrawColor(2);
    u8g2.drawBox(52, 16, 21, 8);
    u8g2.sendBuffer();
}

void display_firmware()
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.drawXBMP(1, 1, 126, 21, microboy_bits);
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(98, 30, _VERSION);
    u8g2.setDrawColor(1);
    u8g2.drawStr(1, 30, _PRODUCT_LINE);
    u8g2.sendBuffer();
}

void display_splash()
{
    display_logo();
    delay(1000);
    display_firmware();
    delay(1000);
}

void display_disconnected()
{
    //todo
}

void display_main()
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);

    u8g2.setBitmapMode(1);
    u8g2.drawXBMP(0, 0, 128, 32, main_bits);

    //mute & signal
    u8g2.setDrawColor(2);
    u8g2.setFont(FONT_SMALL);
    
    //mute indicator
    if (midiController.isSolo) {
        u8g2.drawStr(19, 6, (midiController.soloTrack == 1 ? "s": "m"));
        u8g2.drawStr(51, 6,(midiController.soloTrack == 2 ? "s": "m"));
        u8g2.drawStr(83, 6, (midiController.soloTrack == 3 ? "s": "m"));
        u8g2.drawStr(115, 6, (midiController.soloTrack == 4 ? "s": "m"));
    }

    if (!midiController.isPU1Muted) u8g2.drawBox(25, 2, 4, 4);
    if (!midiController.isPU2Muted) u8g2.drawBox(57, 2, 4, 4);
    if (!midiController.isWAVMuted) u8g2.drawBox(89, 2, 4, 4);
    if (!midiController.isNOIMuted) u8g2.drawBox(121, 2, 4, 4);

    //velocity
    u8g2.setDrawColor(1);
    u8g2.setFont(FONT_SMALL);
    if (midiController.isMute) {
        u8g2.drawStr(112, 31, "---");
        u8g2.setDrawColor(2);
        u8g2.drawStr(48, 30, "mu-e");
    } else {
        static char velstr[4];
        sprintf(velstr, "%03d", config.velocity);
        u8g2.drawStr(112, 31, velstr);

        uint8_t vel = map(config.velocity, 0, 127, 0, 103);
        u8g2.setDrawColor(2);
        u8g2.drawBox(7, 26, vel, 4);
    }
    
    //channels
    u8g2.setDrawColor(1);
    u8g2.setFont(FONT_LARGE);
    static char chstr[3];
    sprintf(chstr, "%02d", config.outputChannel[0]);
    u8g2.drawStr(7, 21, chstr);
    sprintf(chstr, "%02d", config.outputChannel[1]);
    u8g2.drawStr(39, 21, chstr);
    sprintf(chstr, "%02d", config.outputChannel[2]);
    u8g2.drawStr(71, 21, chstr);
    sprintf(chstr, "%02d", config.outputChannel[3]);
    u8g2.drawStr(103, 21, chstr);

    Cursor cu = display.mainCursors[display.mainCursorIndex];
    u8g2.setDrawColor(2);
    u8g2.drawBox(cu.x, cu.y, cu.w, cu.h);

    u8g2.sendBuffer();
}

void display_menu()
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setDrawColor(1);
    if (display.menuIndex > 0) u8g2.drawXBMP(4, 8, 12, 16, right_bits);
    if (display.menuIndex < NUM_MAIN_MENU - 1)u8g2.drawXBMP(112, 8, 12, 16, left_bits);
    u8g2.drawBox(21, 8, 86, 16);
    u8g2.setDrawColor(2);
    u8g2.setFont(FONT_LARGE);
    u8g2.drawStr(25, 22, display.mainMenus[display.menuIndex].name);
    u8g2.sendBuffer();
}

void display_submenu()
{
    SubMenu sub = display.mainMenus[display.menuIndex].subMenus[display.submenuIndex];

    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setFont(FONT_LARGE);
    u8g2.drawStr(3, 13, sub.name);
    u8g2.drawStr(122, 29, "]");
    
    switch (sub.type)
    {
        case RANGE_1_16:
            char chnstr[3];
            sprintf(chnstr, "%02d", *sub.value);
            u8g2.drawStr(90, 29, chnstr);
            break;
        case RANGE_0_127:
            char valstr[4];
            sprintf(valstr, "%03d", *sub.value);
            u8g2.drawStr(85, 29, valstr);
            break;
        case RANGE_1000_5000_BY_100:
            char btstr[5];
            sprintf(btstr, "%04d", *sub.value);
            u8g2.drawStr(81, 29, btstr);
            break;
        case ON_OFF:
            if (*sub.value) u8g2.drawStr(90, 29, "ON");
            else u8g2.drawStr(86, 29, "OFF");
            break;
        case ABOUT:
            u8g2.setFont(FONT_SMALL);
            u8g2.drawStr(11, 27, sub.charValue);
            break;
        case SAVE_CONFIG:
        case LOAD_DEFAULT:
            u8g2.drawStr(90, 29, "OK");
            break;
        default:
            break;
    }

    u8g2.setFont(FONT_LARGE);
    u8g2.setDrawColor(2);
    u8g2.drawBox(2, 1, 93, 13);
    if (sub.type != ABOUT) {
        u8g2.drawStr(68, 29, "[");
        u8g2.drawBox(80, 17, 37, 13);
    } else {
        u8g2.drawStr(-1, 29, "[");
    }

    u8g2.sendBuffer();
}

void display_init()
{   
    display_load();

    u8g2.begin();

    display_splash();
}

void display_refresh()
{
    switch (display.currentState) {
        case MAIN_DISPLAY:
            display_main();
        break;

        case MAIN_MENU:
            display_menu();
        break;

        case SUBMENU:
            display_submenu();
        break;
    }
}