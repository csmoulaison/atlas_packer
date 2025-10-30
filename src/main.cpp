#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#define FONT_CHARS_LEN 128

struct FontChar {
	u8* pixels;
	u32 size[2];
	i32 bearing[2];
	u32 advance;
};

int main(int argc, char** argv) {
	// Validate arguments
	if(argc < 2) {
		printf("Requires a font path as first argument.\n  (Usage: atlas font.tff 12)\n");
		return 1;
	}
	if(argc < 3) {
		printf("Requires a font size after font path argument.\n  (Usage: atlas font.tff 12)\n");
		return 1;
	}
	if(argc > 3) {
		printf("Too many arguments.\n  (Usage: atlas font.tff 12)\n");
		return 1;
	}

	const char* filename = argv[1];
	i32 font_size = atoi(argv[2]);
	if(font_size == 0) {
		printf("Invalid font size argument.\n  (Usage: atlas font.tff 12)\n");
		return 1;
	}

	// Load glyphs with freetype
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) { panic(); }

	FT_Face face;
	if(FT_New_Face(ft, filename, 0, &face)) { panic(); }

	FT_Set_Pixel_Sizes(face, 0, font_size);

	Arena arena;
	arena_init(&arena, GIGABYTE);

	FontChar char_list[FONT_CHARS_LEN];

	for(unsigned char c = 0; c < FONT_CHARS_LEN; c++) {
		if(FT_Load_Char(face, c, FT_LOAD_RENDER)) { panic(); }

		FontChar* ch = &char_list[c];
		ch->size[0] = face->glyph->bitmap.width;
		ch->size[1] = face->glyph->bitmap.rows;
		ch->bearing[0] = face->glyph->bitmap_left;
		ch->bearing[1] = face->glyph->bitmap_top;
		ch->advance = (u32)face->glyph->advance.x;

		u32 tex_size = sizeof(u8) * ch->size[0] * ch->size[1];
		ch->pixels = (u8*)arena_alloc(&arena, tex_size);
		memcpy(ch->pixels, face->glyph->bitmap.buffer, tex_size);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Debug print characters.
	for(u32 j = 0; j < FONT_CHARS_LEN; j++) {
		FontChar* c = &char_list[j];
		printf("CHARACTER\ns: %u %u, b: %u %u, a: %u\n", c->size[0], c->size[1], c->bearing[0], c->bearing[1], c->advance);
		for(u32 i = 0; i < c->size[0] * c->size[1]; i++) {
			if(i % c->size[0] == 0) { printf("\n"); }
			float val = c->pixels[i];
			char p = ' '; if(val > 0.5) { p = '#'; }
			printf("%c", p);
		}
		getchar();
	}
}
