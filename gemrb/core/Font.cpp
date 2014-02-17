/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Font.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"

#include <sstream>

namespace GemRB {

Font::Font(Palette* pal)
: resRefs(NULL), numResRefs(0), palette(NULL), maxHeight(0)
{
	name[0] = '\0';
	SetPalette(pal);
}

Font::~Font(void)
{
	blank->release();
	SetPalette(NULL);
	free(resRefs);
}

bool Font::AddResRef(const ieResRef resref)
{
	if (resref) {
		resRefs = (ieResRef*)realloc(resRefs, sizeof(ieResRef) * ++numResRefs);
		strnlwrcpy( resRefs[numResRefs - 1], resref, sizeof(ieResRef)-1);
		return true;
	}
	return false;
}

bool Font::MatchesResRef(const ieResRef resref)
{
	for (int i=0; i < numResRefs; i++)
	{
		if (strnicmp( resref, resRefs[i], sizeof(ieResRef)-1) == 0){
			return true;
		}
	}
	return false;
}

// Canvas can't be a sprite, because we may need to realloc
void Font::BlitGlyphToCanvas(const Sprite2D* glyph, int x, int y,
							 ieByte* canvas, const Size& size) const
{
	assert(canvas);
	assert(!glyph->BAM);
	// TODO: should handle partial glyphs
	if (!Region(0, 0, size.w, size.h).PointInside(x, y)) {
		return; // off the canvas
	}

	// copy the glyph to the canvas
	ieByte* src = (ieByte*)glyph->pixels;
	ieByte* dest = canvas + (size.w * y) + x;
	for(int row = 0; row < glyph->Height; row++ ) {
		//assert(dest <= canvas + (size.w * size.h));
		if (dest + glyph->Width > canvas + (size.w * size.h)) {
			break;
		}
		memcpy(dest, src, glyph->Width);
		dest += size.w;
		src += glyph->Width;
	}
}

size_t Font::RenderText(const String& string, Region& rgn,
						Palette* color, ieByte alignment, ieByte** canvas, bool grow) const
{
	assert(color);
	ieWord lineHeight = maxHeight;
	if (alignment&IE_FONT_SINGLE_LINE) {
		int stringh = StringSize(string).h;
		lineHeight = (stringh < maxHeight) ? stringh : maxHeight;
	}
	assert(lineHeight <= maxHeight);
	int x = 0, y = lineHeight;

	if (!canvas) {
		// canvas vertical alignment is handled by centering the completed
		// canvas within a given frame. also the canvas can possibly grow vertically, rendering this useless.
		if (alignment & (IE_FONT_ALIGN_MIDDLE|IE_FONT_ALIGN_BOTTOM)) {
			int lc = 1;
			for (size_t i = 0; i <= string.length(); i++) {
				if (string[i] == L'\n')
					lc++;
			}
			int h = lc * lineHeight;
			if (alignment & IE_FONT_ALIGN_MIDDLE) {
				y += ( rgn.h - h ) / 2;
			} else {
				y += ( rgn.h - h );
			}
		}
	}

	// is this horribly inefficient?
	std::wistringstream stream(string);
	String line, word;
	const Sprite2D* currGlyph = NULL;
	bool done = false, lineBreak = false;
	size_t charCount = 0;

	while (!done && (lineBreak || getline(stream, line))) {
		lineBreak = false;

		// check if we need to extend the canvas
		if (canvas && grow && rgn.h < y) {
			size_t pos = stream.tellg();
			pos -= line.length();
			Size textSize = StringSize(string.substr(pos));
			ieWord numNewPixels = textSize.Area();
			ieWord lineArea = rgn.w * maxHeight;
			// round up
			ieWord numLines = 1 + ((numNewPixels - 1) / lineArea);
			// extend the region and canvas both
			size_t curpos = rgn.h * rgn.w;
			size_t vGrow = (numLines * maxHeight) + descent;
			rgn.h += vGrow;
			*canvas = (ieByte*)realloc(*canvas, rgn.w * rgn.h);
			assert(canvas);
			// "zero out" (using color key) the new area or we will get garbage in the areas we dont blit to
			ieDword ck = GetCharSprite(string[0])->GetColorKey();
			memset(*canvas + curpos, ck, vGrow * rgn.w);
		}

		ieWord lineW = StringSize(line).w;
		if (alignment & IE_FONT_ALIGN_CENTER) {
			x = ( rgn.w - lineW ) / 2;
		} else if (alignment & IE_FONT_ALIGN_RIGHT) {
			x = ( rgn.w - lineW );
		} else {
			x = 0;
		}

		size_t lineLen = line.length();
		if (lineLen) {
			size_t linePos = 0, wordBreak = 0;
			while (line[linePos] == L' ') {
				// skip spaces at the beginning of a line
				linePos++;
			}
			// FIXME: I'm not sure how to handle Asian text
			// should a "word" be a single Asian glyph? that way we wouldnt clip off text (we were doing this before the rewrite too).
			// we could check the core encoding for the 'zerospace' attribute and treat single characters as words
			// that would looks funny with partial translations, however. we would need to handle both simultaniously.
			while (!lineBreak && (wordBreak = line.find_first_of(L' ', linePos))) {
				word = line.substr(linePos, wordBreak - linePos);

				int wordW = StringSize(word).w;
				if (!(alignment&IE_FONT_SINGLE_LINE)) {
					if (x + wordW > rgn.w && wordW <= rgn.w) {
						// wrap to new line, only if the word isnt >= the entire line
						lineBreak = true;
						line = line.substr(linePos);
					}
				}

				if (!lineBreak) {
					// print the word
					wchar_t currChar = '\0';
					size_t i;
					for (i = 0; i < word.length(); i++) {
						// process glyphs in word
						currChar = word[i];
						if (currChar == '\r') {
							continue;
						}
						if (i > 0) { // kerning
							x -= GetKerningOffset(word[i-1], currChar);
						}
						currGlyph = GetCharSprite(currChar);
						// should probably use rect intersection, but new lines shouldnt be to the left ever.
						if (!rgn.PointInside(x + rgn.x - currGlyph->XPos,
											 y + rgn.y - currGlyph->YPos)) {
							if (wordW < (int)lineW) {
								// this probably doest cover every situation 100%
								// we consider printing done if the blitter is outside the region
								// *and* the word isnt wider then the line
								done = true;
							} else {
#if DEBUG_FONT
								Log(WARNING, "Font", "The word '%ls' (width=%d) overruns %d",
									word.c_str(), wordW, rgn.w);
#endif
							}
							break; // either done, or skipping
						}
						if (canvas) {
							BlitGlyphToCanvas(currGlyph, x, y - currGlyph->YPos,
											  *canvas, rgn.Dimensions());
						} else {
							core->GetVideoDriver()->BlitSprite(currGlyph, x + rgn.x, y + rgn.y,
															   true, &rgn, color);
						}
						x += currGlyph->Width;
					}
					if (done) break;
					linePos += i + 1;
				}
				if (wordBreak == String::npos) {
					linePos--; // we previously counted a non-existant space
					break;
				}
				x += GetCharSprite(' ')->Width;
			}
			charCount += linePos;
		}

		if (!lineBreak && !stream.eof())
			charCount++; // for the newline
		y += maxHeight;
	}
	assert(charCount <= string.length());
	return charCount;
}

Sprite2D* Font::RenderTextAsSprite(const String& string, const Size& size,
								   ieByte alignment, Palette* color, size_t* numPrinted) const
{
	Size canvasSize = StringSize(string); // same as size(0, 0)
	// if the string is larger than the region shrink the canvas
	// except 0 means we should size to fit in that dimension
	if (size.w) {
		// potentially resize
		if (size.w < canvasSize.w) {
			if (!(alignment&IE_FONT_SINGLE_LINE)) {
				// we need to resize horizontally which creates new lines
				ieWord trimmedArea = ((canvasSize.w - size.w) * canvasSize.h);
				// this automatically becomes multiline, therefore use maxHeight
				ieWord lineArea = size.w * maxHeight;
				// round up
				ieWord numLines = 1 + ((trimmedArea - 1) / lineArea);
				if (!size.h) {
					// grow as much as needed vertically.
					canvasSize.h += (numLines * maxHeight) + descent;
					// there is a chance we didn't grow enough vertically...
					// we can't possibly know how lines will break ahead of time,
					// over a long enough paragraph we can overflow the canvas
					// this is handled in RenderText() by reallocing the canvas based on
					// the same estimation algorithim (total area of text) used here
				} else if (size.h > canvasSize.h) {
					// grow by line increments until we hit the limit
					// round up, because even a partial line should be blitted (and clipped)
					ieWord maxLines = 1 + (((size.h - canvasSize.h) - 1) / maxHeight);
					if (maxLines < numLines) {
						numLines = maxLines;
					}
					canvasSize.h += (numLines * maxHeight) + descent;
					// if the new canvas size is taller than size.h it will be dealt with later
				}
			}
			canvasSize.w = size.w;
		}
		// else: we already fit in the designated area (horizontally). No need to resize.
	}
	if (!(alignment&IE_FONT_SINGLE_LINE)) {
		if (canvasSize.h < maxHeight) {
			// should be at least maxHeight (+ decender added later then trimmed if too large for size)
			canvasSize.h = maxHeight;
		}
		canvasSize.h += descent; // compensate for last line descenders
		// FIXME: this decender "calculation" is just a guess
		//canvasSize.h += maxHeight / 2; // compensation for decenders
	}
	if (size.h && size.h < canvasSize.h) {
		// we can't unbreak lines ("\n") so at best we can clip the text.
		canvasSize.h = size.h;
	}

	ieDword ck = GetCharSprite(string[0])->GetColorKey();
	// we must calloc/memset because not all glyphs are equal height. set remainder to the color key
	ieByte* canvasPx = (ieByte*)calloc(canvasSize.w, canvasSize.h);
	if (ck != 0) {
		// start with transparent canvas
		memset(canvasPx, ck, canvasSize.w * canvasSize.h);
	}

	Region rgn = Region(Point(0,0), canvasSize);
	size_t ret = RenderText(string, rgn, palette, alignment, &canvasPx, (size.h) ? false : true);
	// FIXME: what do we do about overestimates making a canvas to tall and therefore potentially causing misalignment?
	if (numPrinted) {
		*numPrinted = ret;
	}
	Palette* pal = color;
	if (!pal)
		pal = palette;
	// must ue rgn! the canvas height might be changed in RenderText()
	Sprite2D* canvas = core->GetVideoDriver()->CreateSprite8(rgn.w, rgn.h,
															 canvasPx, pal, true, ck);
	if (alignment&IE_FONT_ALIGN_CENTER) {
		canvas->XPos = (size.w - canvasSize.w) / 2;
	} else if (alignment&IE_FONT_ALIGN_RIGHT) {
		canvas->XPos = size.w - canvasSize.w;
	}
	// FIXME: this is broken for size.h == 0
	// we can re-examine rgn after calling RenderText, to get the actual size
	// however, the canvas could have been overestimated
	// we should probably address this in RenderText to realloc the canvas to the final size used
	if (alignment&IE_FONT_ALIGN_MIDDLE) {
		canvas->YPos = (size.h - canvasSize.h) / 2;
	} else if (alignment&IE_FONT_ALIGN_BOTTOM) {
		canvas->YPos = size.h - canvasSize.h;
	}
	return canvas;
}

size_t Font::Print(Region rgn, const char* string,
				   Palette* hicolor, ieByte Alignment) const
{
	String* tmp = StringFromCString(string);
	size_t ret = Print(rgn, *tmp, hicolor, Alignment);
	delete tmp;
	return ret;
}

size_t Font::Print(Region rgn, const String& string, Palette* color,
				   ieByte alignment) const
{
	Palette* pal = color;
	if (!pal) {
		pal = palette;
	}
	return RenderText(string, rgn, pal, alignment);
}

Size Font::StringSize(const String& string, const Size* stop) const
{
	if (!string.length()) return Size();
	ieWord w = 0, h = 0, lines = 1;
	ieWord curh = 0, curw = 0;
	int decent = 0, maxd = 0;
	bool multiline = false;
	for (size_t i = 0; i < string.length(); i++) {
		if (string[i] == L'\n') {
			if (curw > w)
				w = curw;
			curw = 0;
			multiline = true;
			lines++;
		} else {
			const Sprite2D* curGlyph = GetCharSprite(string[i]);
			curh = curGlyph->Height;
			decent = curGlyph->Height - curGlyph->YPos;
			maxd = (decent > maxd) ? decent : maxd;
			curh += 0;
			if (curh > h)
				h = curh;
			curw += curGlyph->Width;
			if (i > 0) { // kerning
				curw -= GetKerningOffset(string[i-1], string[i]);
			}
		}
		if (stop && (curw > stop->w || curh > stop->h))
			break;
	}
	if (!multiline) {
		h += maxd;
	} else {
		h = (maxHeight * lines) + this->descent;
	}
	w = (curw > w) ? curw : w;
	assert(w);
	return Size(w, h);
}

void Font::SetName(const char* newName)
{
	strnlwrcpy( name, newName, sizeof(name)-1);
}

Palette* Font::GetPalette() const
{
	assert(palette);
	palette->acquire();
	return palette;
}

void Font::SetPalette(Palette* pal)
{
	if (pal) pal->acquire();
	if (palette) palette->release();
	palette = pal;
}

}
