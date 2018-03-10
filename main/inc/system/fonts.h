/*
 * fonts.h
 *
 *  Created on: 2018-02-10 15:54
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef FONTS_H
#define FONTS_H

#define RGB(R,G,B)  (((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3))

enum fonts_color {
    LightPink               = RGB(255,182,193),
    Pink                    = RGB(255,192,203),
    Crimson                 = RGB(220, 20, 60),
    LavenderBlush           = RGB(255,240,245),
    PaleVioletRed           = RGB(219,112,147),
    HotPink                 = RGB(255,105,180),
    DeepPink                = RGB(255, 20,147),
    MediumVioletRed         = RGB(199, 21,133),
    Orchid                  = RGB(218,112,214),
    Thistle                 = RGB(216,191,216),
    Plum                    = RGB(221,160,221),
    Violet                  = RGB(238,130,238),
    Magenta                 = RGB(255,  0,255),
    Fuchsia                 = RGB(255,  0,255),
    DarkMagenta             = RGB(139,  0,139),
    Purple                  = RGB(128,  0,128),
    MediumOrchid            = RGB(186, 85,211),
    DarkVoilet              = RGB(148,  0,211),
    DarkOrchid              = RGB(153, 50,204),
    Indigo                  = RGB( 75,  0,130),
    BlueViolet              = RGB(138, 43,226),
    MediumPurple            = RGB(147,112,219),
    MediumSlateBlue         = RGB(123,104,238),
    SlateBlue               = RGB(106, 90,205),
    DarkSlateBlue           = RGB( 72, 61,139),
    Lavender                = RGB(230,230,250),
    GhostWhite              = RGB(248,248,255),
    Blue                    = RGB(  0,  0,255),
    MediumBlue              = RGB(  0,  0,205),
    MidnightBlue            = RGB( 25, 25,112),
    DarkBlue                = RGB(  0,  0,139),
    Navy                    = RGB(  0,  0,128),
    RoyalBlue               = RGB( 65,105,225),
    CornflowerBlue          = RGB(100,149,237),
    LightSteelBlue          = RGB(176,196,222),
    LightSlateGray          = RGB(119,136,153),
    SlateGray               = RGB(112,128,144),
    DoderBlue               = RGB( 30,144,255),
    AliceBlue               = RGB(240,248,255),
    SteelBlue               = RGB( 70,130,180),
    LightSkyBlue            = RGB(135,206,250),
    SkyBlue                 = RGB(135,206,235),
    DeepSkyBlue             = RGB(  0,191,255),
    LightBLue               = RGB(173,216,230),
    PowDerBlue              = RGB(176,224,230),
    CadetBlue               = RGB( 95,158,160),
    Azure                   = RGB(240,255,255),
    LightCyan               = RGB(225,255,255),
    PaleTurquoise           = RGB(175,238,238),
    Cyan                    = RGB(  0,255,255),
    Aqua                    = RGB(  0,255,255),
    DarkTurquoise           = RGB(  0,206,209),
    DarkSlateGray           = RGB( 47, 79, 79),
    DarkCyan                = RGB(  0,139,139),
    Teal                    = RGB(  0,128,128),
    MediumTurquoise         = RGB( 72,209,204),
    LightSeaGreen           = RGB( 32,178,170),
    Turquoise               = RGB( 64,224,208),
    Auqamarin               = RGB(127,255,170),
    MediumAquamarine        = RGB(  0,250,154),
    MediumSpringGreen       = RGB(  0,255,127),
    MintCream               = RGB(245,255,250),
    SpringGreen             = RGB( 60,179,113),
    SeaGreen                = RGB( 46,139, 87),
    Honeydew                = RGB(240,255,240),
    LightGreen              = RGB(144,238,144),
    PaleGreen               = RGB(152,251,152),
    DarkSeaGreen            = RGB(143,188,143),
    LimeGreen               = RGB( 50,205, 50),
    Lime                    = RGB(  0,255,  0),
    ForestGreen             = RGB( 34,139, 34),
    Green                   = RGB(  0,128,  0),
    DarkGreen               = RGB(  0,100,  0),
    Chartreuse              = RGB(127,255,  0),
    LawnGreen               = RGB(124,252,  0),
    GreenYellow             = RGB(173,255, 47),
    OliveDrab               = RGB( 85,107, 47),
    Beige                   = RGB(245,245,220),
    LightGoldenrodYellow    = RGB(250,250,210),
    Ivory                   = RGB(255,255,240),
    LightYellow             = RGB(255,255,224),
    Yellow                  = RGB(255,255,  0),
    Olive                   = RGB(128,128,  0),
    DarkKhaki               = RGB(189,183,107),
    LemonChiffon            = RGB(255,250,205),
    PaleGodenrod            = RGB(238,232,170),
    Khaki                   = RGB(240,230,140),
    Gold                    = RGB(255,215,  0),
    Cornislk                = RGB(255,248,220),
    GoldEnrod               = RGB(218,165, 32),
    FloralWhite             = RGB(255,250,240),
    OldLace                 = RGB(253,245,230),
    Wheat                   = RGB(245,222,179),
    Moccasin                = RGB(255,228,181),
    Orange                  = RGB(255,165,  0),
    PapayaWhip              = RGB(255,239,213),
    BlanchedAlmond          = RGB(255,235,205),
    NavajoWhite             = RGB(255,222,173),
    AntiqueWhite            = RGB(250,235,215),
    Tan                     = RGB(210,180,140),
    BrulyWood               = RGB(222,184,135),
    Bisque                  = RGB(255,228,196),
    DarkOrange              = RGB(255,140,  0),
    Linen                   = RGB(250,240,230),
    Peru                    = RGB(205,133, 63),
    PeachPuff               = RGB(255,218,185),
    SandyBrown              = RGB(244,164, 96),
    Chocolate               = RGB(210,105, 30),
    SaddleBrown             = RGB(139,69, 19),
    SeaShell                = RGB(255,245,238),
    Sienna                  = RGB(160, 82, 45),
    LightSalmon             = RGB(255,160,122),
    Coral                   = RGB(255,127, 80),
    OrangeRed               = RGB(255, 69,  0),
    DarkSalmon              = RGB(233,150,122),
    Tomato                  = RGB(255, 99, 71),
    MistyRose               = RGB(255,228,225),
    Salmon                  = RGB(250,128,114),
    Snow                    = RGB(255,250,250),
    LightCoral              = RGB(240,128,128),
    RosyBrown               = RGB(188,143,143),
    IndianRed               = RGB(205, 92, 92),
    Red                     = RGB(255,  0,  0),
    Brown                   = RGB(165, 42, 42),
    FireBrick               = RGB(178, 34, 34),
    DarkRed                 = RGB(139,  0,  0),
    Maroon                  = RGB(128,  0,  0),
    White                   = RGB(255,255,255),
    WhiteSmoke              = RGB(245,245,245),
    Gainsboro               = RGB(220,220,220),
    LightGrey               = RGB(211,211,211),
    Silver                  = RGB(192,192,192),
    DarkGray                = RGB(169,169,169),
    Gray                    = RGB(128,128,128),
    DimGray                 = RGB(105,105,105),
    Black                   = RGB(0,0,0)
};

enum fonts_size {
    FONT_1206   = 0,
    FONT_1608   = 1,
    FONT_1616   = 2,
    FONT_3216   = 3
};

extern const unsigned char fonts_height[];
extern const unsigned char fonts_width[];

extern const unsigned char c_chFont1206[95][12];
extern const unsigned char c_chFont1608[95][16];
extern const unsigned char c_chFont1616[95][32];
extern const unsigned char c_chFont3216[95][64];

extern const unsigned char c_chBmp4016[96];
extern const unsigned char c_chSingal816[16];
extern const unsigned char c_chMsg816[16];
extern const unsigned char c_chBluetooth88[8];
extern const unsigned char c_chBat816[16];
extern const unsigned char c_chGPRS88[8];
extern const unsigned char c_chAlarm88[8];

#endif
