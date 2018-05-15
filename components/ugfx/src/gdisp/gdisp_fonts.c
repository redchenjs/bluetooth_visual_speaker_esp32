/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "../../gfx.h"

#if GFX_USE_GDISP && GDISP_NEED_TEXT

#include "mcufont/mcufont.h"

#define FONT_FLAG_DYNAMIC	0x80		// Custom flag to indicate dynamically allocated font
#define FONT_FLAG_UNLISTED	0x40		// Custom flag to indicate font is not currently listed

static const struct mf_font_list_s *fontList;

/**
 * Match a pattern against the font name.
 */
static bool_t matchfont(const char *pattern, const char *name) {
	while(1) {
		switch (pattern[0]) {
		case '*':
			if (name[0] == 0)
				return pattern[1] == 0;
			if (pattern[1] == name[0])
				pattern++;
			else
				name++;
			break;
		case 0:
			return name[0] == 0;
		default:
			if (name[0] != pattern[0])
				return FALSE;
			pattern++;
			name++;
			break;
		}
	}
}

font_t gdispOpenFont(const char *name) {
	const struct mf_font_list_s *fp;
	
	if (!fontList)
		fontList = mf_get_font_list();
		
	// Try the long names first
	for(fp = fontList; fp; fp = fp->next) {
		if (matchfont(name, fp->font->full_name))
			return fp->font;
	}

	// Try the short names if no long names match
	for(fp = fontList; fp; fp = fp->next) {
		if (matchfont(name, fp->font->short_name))
			return fp->font;
	}
	
	/* Return default builtin font.. better than nothing. */
	return mf_get_font_list()->font;
}

void gdispCloseFont(font_t font) {
	if ((font->flags & (FONT_FLAG_DYNAMIC|FONT_FLAG_UNLISTED)) == (FONT_FLAG_DYNAMIC|FONT_FLAG_UNLISTED)) {
		/* Make sure that no-one can successfully use font after closing */
		((struct mf_font_s *)font)->render_character = 0;
		
		/* Release the allocated memory */
		gfxFree((void *)font);
	}
}

font_t gdispScaleFont(font_t font, uint8_t scale_x, uint8_t scale_y)
{
	struct mf_scaledfont_s *newfont;
	
	if (!(newfont = gfxAlloc(sizeof(struct mf_scaledfont_s))))
		return 0;
	
	mf_scale_font(newfont, font, scale_x, scale_y);
	((struct mf_font_s *)newfont)->flags |= FONT_FLAG_DYNAMIC|FONT_FLAG_UNLISTED;
	return (font_t)newfont;
}

const char *gdispGetFontName(font_t font) {
	return font->short_name;
}

bool_t gdispAddFont(font_t font) {
	struct mf_font_list_s *hdr;

	if ((font->flags & (FONT_FLAG_DYNAMIC|FONT_FLAG_UNLISTED)) != (FONT_FLAG_DYNAMIC|FONT_FLAG_UNLISTED))
		return FALSE;
		
	if (!(hdr = gfxAlloc(sizeof(struct mf_font_list_s))))
		return FALSE;

	if (!fontList)
		fontList = mf_get_font_list();
	hdr->font = (const struct mf_font_s *)font;
	hdr->next = fontList;
	((struct mf_font_s *)font)->flags &= ~FONT_FLAG_UNLISTED;
	fontList = hdr;
	return TRUE;
}

#endif /* GFX_USE_GDISP && GDISP_NEED_TEXT */
