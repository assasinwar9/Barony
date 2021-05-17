/*-------------------------------------------------------------------------------

	BARONY
	File: draw.hpp
	Desc: prototypes for draw.cpp, various drawing functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/
#pragma once
#include <SDL.h>
#include "entity.hpp"

#define FLIP_VERTICAL 1
#define FLIP_HORIZONTAL 2
SDL_Surface* flipSurface(SDL_Surface* surface, int flags);
void drawCircle(int x, int y, real_t radius, Uint32 color, Uint8 alpha);
void drawArc(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha);
void drawArcInvertedY(int x, int y, real_t radius, real_t angle1, real_t angle2, Uint32 color, Uint8 alpha);
void drawLine(int x1, int y1, int x2, int y2, Uint32 color, Uint8 alpha);
int drawRect(SDL_Rect* src, Uint32 color, Uint8 alpha);
int drawBox(SDL_Rect* src, Uint32 color, Uint8 alpha);
void drawGear(Sint16 x, Sint16 y, real_t size, Sint32 rotation);
void drawImage(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos);
void drawImageRing(SDL_Surface* image, SDL_Rect* src, int radius, int thickness, int segments, real_t angStart, real_t angEnd, Uint8 alpha);
void drawImageScaled(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos);
void drawImageScaledPartial(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, float percentY);
void drawImageAlpha(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint8 alpha);
void drawImageColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color);
void drawImageFancy(SDL_Surface* image, Uint32 color, real_t angle, SDL_Rect* src, SDL_Rect* pos);
void drawImageRotatedAlpha(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, real_t angle, Uint8 alpha);
void drawImageScaledColor(SDL_Surface* image, SDL_Rect* src, SDL_Rect* pos, Uint32 color);
SDL_Surface* scaleSurface(SDL_Surface* Surface, Uint16 Width, Uint16 Height);
void drawSky3D(view_t* camera, SDL_Surface* tex);
void drawLayer(long camx, long camy, int z, map_t* map);
void drawBackground(long camx, long camy);
void drawForeground(long camx, long camy);
void drawClearBuffers();
void raycast(view_t* camera, int mode, bool updateVismap = true);
void drawFloors(view_t* camera);
void drawSky(SDL_Surface* srfc);
void drawVoxel(view_t* camera, Entity* entity);
void drawEntities3D(view_t* camera, int mode);
void drawPalette(voxel_t* model);
void drawEntities2D(long camx, long camy);
void drawGrid(long camx, long camy);
void drawEditormap(long camx, long camy);
void drawWindow(int x1, int y1, int x2, int y2);
void drawDepressed(int x1, int y1, int x2, int y2);
void drawWindowFancy(int x1, int y1, int x2, int y2);
SDL_Rect ttfPrintTextColor( TTF_Font* font, int x, int y, Uint32 color, bool outline, const char* str );
SDL_Rect ttfPrintText( TTF_Font* font, int x, int y, const char* str );
SDL_Rect ttfPrintTextFormattedColor( TTF_Font* font, int x, int y, Uint32 color, char const * const fmt, ... );
SDL_Rect ttfPrintTextFormatted( TTF_Font* font, int x, int y, char const * const fmt, ... );
void printTextFormatted( SDL_Surface* font_bmp, int x, int y, char const * const fmt, ... );
void printTextFormattedAlpha(SDL_Surface* font_bmp, int x, int y, Uint8 alpha, char const * const fmt, ...);
void printTextFormattedColor(SDL_Surface* font_bmp, int x, int y, Uint32 color, char const * const fmt, ...);
void printTextFormattedFancy(SDL_Surface* font_bmp, int x, int y, Uint32 color, real_t angle, real_t scale, char* fmt, ...);
void printText( SDL_Surface* font_bmp, int x, int y, const char* str );
void drawSprite(view_t* camera, Entity* entity);
void drawTooltip(SDL_Rect* src, Uint32 optionalColor = 0);
Uint32 getPixel(SDL_Surface* surface, int x, int y);
void putPixel(SDL_Surface* surface, int x, int y, Uint32 pixel);
Uint32 makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

class TempTexture {
public:
	TempTexture() {
	}

	~TempTexture() {
		if( texid ) {
			glDeleteTextures(1,&texid);
			texid = 0;
		}
	}

	void load(SDL_Surface* surf, bool clamp, bool point) {
		SDL_LockSurface(surf);
		glGenTextures(1,&texid);
		glBindTexture(GL_TEXTURE_2D, texid);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
		if (clamp) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		if (point) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.f);
			//glGenerateMipmap(GL_TEXTURE_2D);
		}
		SDL_UnlockSurface(surf);
	}

	void bind() {
		glBindTexture(GL_TEXTURE_2D, texid);
	}

private:
	GLuint texid = 0;
};