#include <stdio.h>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#define CSM_BASE_IMPLEMENTATION
#include "base/base.h"

#define FONT_CHARS_LEN 128

struct FontChar {
	u8* pixels;
	u32 size[2];
	i32 bearing[2];
	u32 advance;
};

struct PackRect {
	u32 x, y, w, h;
	bool packed = false;
};

// Represents an empty space in the atlas being packed.
struct PackNode {
	i32 x, y, w, h;
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

	/* Debug print characters.
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
	} */


	// NOW: CODE COPIED FROM:
	// https://www.david-colson.com/2020/03/10/exploring-rect-packing.html
	// 
	// 1. Adapt to our style to understand. No vectors, snake case, etc.
	// 2. Render to the packed rect locations.
	// 3. Pack texture and character data into binary file and save to disk.
	std::vector<PackRect> rects;
	// NOW: Populate rects from FontChar data.
	// - sort rects by area?
	 
	std::vector<PackNode> leaves;

	// NOW: 700x700 is implicit here, I guess? We want to choose something relative
	// to the chosen font size presumably, and we might want it to be a power of 2.
	// 
	// Our initial node is the entire image
	leaves.push_back({0, 0, 700, 700});

	for (PackRect& rect : rects)
	{
		bool done = false;
		// Backward iterate over nodes
		for (int i = (int)leaves.size() - 1; i >= 0 && !done; --i) {
			PackNode& node = leaves[i];

			// If the node is big enough, we've found a suitable spot for our rectangle
			if (node.w > rect.w && node.h > rect.h) {
				rect.x = node.x;
				rect.y = node.y;

				// Split the rectangle, calculating the unused space
				int remainingWidth = node.w - rect.w;
				int remainingHeight = node.h - rect.h;

				PackNode newSmallerNode;
				PackNode newLargerNode;
				// We can work out which way we need to split by checking which
				//  remaining dimension is larger
				if (remainingHeight > remainingWidth) {
					// The lesser split here will be the top right
					newSmallerNode.x = node.x + rect.w;
					newSmallerNode.y = node.y;
					newSmallerNode.w = remainingWidth;
					newSmallerNode.h = rect.h;

					newLargerNode.x = node.x;
					newLargerNode.y = node.y + rect.h;
					newLargerNode.w = node.w;
					newLargerNode.h = remainingHeight;
				} else {
					// The lesser split here will be the bottom left
					newSmallerNode.x = node.x;
					newSmallerNode.y = node.y + rect.h;
					newSmallerNode.w = rect.w;
					newSmallerNode.h = remainingHeight;

					newLargerNode.x = node.x + rect.w;
					newLargerNode.y = node.y;
					newLargerNode.w = remainingWidth;
					newLargerNode.h = node.h;
				}

				// Removing the node we're using up
				leaves[i] = leaves.back();
				leaves.pop_back();

				// Push back the two new nodes, note smaller one last.
				leaves.push_back(newLargerNode);
				leaves.push_back(newSmallerNode);

				done = true;
			}
		}

		if (!done) {
			continue;
		}

		rect.packed = true;
	}
}
