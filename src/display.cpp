#include "display.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Cursor cursors[] = {
    {1, 8, 29, 15, config.outputChannel[0]},
    {33, 8, 29, 15, config.outputChannel[1]},
    {65, 8, 29, 15, config.outputChannel[2]},
    {97, 8, 29, 15, config.outputChannel[3]},
    {112, 24, 15, 8, config.velocity}
};

SubMenu channelSubMenus[] = {
    {"PU1", RANGE_1_16, "1"},
    {"PU2", RANGE_1_16, "2"},
    {"WAV", RANGE_1_16, "3"},
    {"NOI", RANGE_1_16, "4"}
};

SubMenu midiSubMenus[] = {
    {"Velocity", RANGE_0_127, "100"},
    {"CC", ON_OFF, "OFF"}, 
    {"PC", ON_OFF, "OFF"}, 
    {"Realtime", ON_OFF, "OFF"}, 
    {"Clock", ON_OFF, "OFF"}
};

SubMenu readerSubMenus[] = {
    {"Byte Delay", RANGE_1000_5000_BY_100, "2000"},
    {"Exp. Correction", ON_OFF, "OFF"}
};

SubMenu configSubMenus[] = {
    {"Save", ACTION, ""},
    {"Default", ACTION, ""}
};

SubMenu aboutSubMenus[] = {
    {"Firmware", TEXT, _VERSION},
    {"Author", TEXT, _AUTHOR},
    {"GitHub", TEXT, _GITHUB}
};

MainMenu mainMenus[] = {
    {"Channels", sizeof(channelSubMenus), channelSubMenus},
    {"MIDI", sizeof(midiSubMenus),  midiSubMenus},
    {"Reader", sizeof(readerSubMenus), readerSubMenus},
    {"Config", sizeof(configSubMenus), configSubMenus},
    {"About", sizeof(aboutSubMenus), aboutSubMenus}
};

Display display;

void display_load()
{
    display.currentState = MAIN_DISPLAY;
    display.mute_pu1 = false;
    display.mute_pu2 = false;
    display.mute_wav = false;
    display.mute_noi = false;
    display.velocity = 100;
    display.mainCursorIndex = 0;
    display.mainCursors = cursors;
    display.cursorSize = 5;//sizeof(cursors);
    display.menuIndex = 0;
    display.submenuIndex = 0;
    display.mainMenus = mainMenus;
    display.mainMenuSize = sizeof(mainMenus);
}

static const unsigned char image_small_bw_bits[] = {0x00,0x00,0x00,0x00,0xe0,0xff,0xff,0x0f,0x20,0x00,0x40,0x08,0x38,0x00,0x40,0x08,0x08,0x00,0x40,0x08,0x0e,0x00,0x40,0x08,0x82,0xff,0xff,0x0f,0xc2,0xf9,0xff,0x0f,0xc2,0xf9,0xff,0x0f,0x42,0xe0,0xe7,0x0c,0x42,0xe0,0x4b,0x09,0xc2,0xf9,0x43,0x08,0xc2,0x49,0xe6,0x0c,0xfe,0xff,0xff,0x0f,0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0x0f,0x86,0x63,0x38,0x0c,0x56,0x5d,0xd7,0x0f,0x56,0x5d,0x17,0x0c,0x56,0x5d,0xf7,0x0d,0x56,0x63,0x18,0x0e,0xfe,0xff,0xff,0x0f,0x00,0x00,0x00,0x00};
void display_logo()
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.drawXBM(19, 4, 29, 23, image_small_bw_bits);
    u8g2.setFont(u8g2_font_haxrcorp4089_tr);
    u8g2.drawStr(57, 14, _USB_MANUFACTURER);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(58, 23, _YEAR);
    u8g2.setDrawColor(2);
    u8g2.drawBox(57, 16, 21, 8);
    u8g2.sendBuffer();
}

void display_firmware()
{
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.setFont(u8g2_font_profont29_tr);
    u8g2.drawStr(1, 21, _USB_PRODUCT);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(98, 31, _VERSION);
    u8g2.setDrawColor(2);
    u8g2.drawBox(80, 1, 47, 21);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_profont11_tr);
    u8g2.drawStr(1, 31, _PRODUCT_LINE);
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
    u8g2.drawBox(18, 1, 12, 6);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(1, 7, "PU1");
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(-2, 21, "[01]");
    u8g2.drawBox(50, 1, 12, 6);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(34, 7, "PU2");
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(30, 21, "[10]");
    u8g2.drawBox(82, 1, 12, 6);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(65, 7, "WAV");
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(62, 21, "[02]");
    u8g2.drawBox(114, 1, 12, 6);
    u8g2.setDrawColor(2);
    u8g2.drawBox(51, 4, 10, 2);
    u8g2.drawBox(19, 4, 10, 2);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(97, 7, "NOI");
    u8g2.setDrawColor(2);
    u8g2.drawBox(83, 4, 8, 2);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(94, 21, "[03]");
    u8g2.drawBox(1, 25, 110, 6);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(2, 30, "v");
    u8g2.setDrawColor(1);
    u8g2.drawStr(112, 31, "100");
    u8g2.setDrawColor(2);
    u8g2.drawBox(7, 26, 87, 4);
    u8g2.drawStr(115, 6, "m");
    //String(display.cursorSize).c_str()
    Cursor cu = display.mainCursors[display.mainCursorIndex];
    u8g2.setDrawColor(2);
    u8g2.drawBox(cu.x, cu.y, cu.w, cu.h);

    u8g2.sendBuffer();
}

void display_menu()
{
    //todo
}

void display_submenu()
{
   //todo
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