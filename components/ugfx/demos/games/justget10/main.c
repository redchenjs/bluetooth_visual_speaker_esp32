#include "gfx.h"
#include "jg10.h"

int main(void)
{
	GEventMouse ev;
#if !JG10_SHOW_SPLASH
	font_t font;
#endif

    gfxInit();

    ginputGetMouse(0);
    jg10Init();

#if JG10_SHOW_SPLASH
    jg10ShowSplash();
#else
    font = gdispOpenFont("DejaVuSans16_aa");
    gdispDrawString((gdispGetWidth()/2)-(gdispGetStringWidth("Touch to start!", font)/2), gdispGetHeight()/2, "Touch to start!", font, White);
    gdispCloseFont(font);
#endif

    while (TRUE) {
        ginputGetMouseStatus(0, &ev);
        if (ev.buttons & GINPUT_MOUSE_BTN_LEFT) {
            while (ev.buttons & GINPUT_MOUSE_BTN_LEFT) {            // Wait until release
                ginputGetMouseStatus(0, &ev);
            }

#if !JG10_SHOW_SPLASH
            font = gdispOpenFont("DejaVuSans16");
            gdispFillArea((gdispGetWidth()/2)-(gdispGetStringWidth("Touch to start!", font)/2), gdispGetHeight()/2, gdispGetWidth()/2, 17, Black);
            gdispCloseFont(font);
#endif

            jg10Start();
        }
    }
}
