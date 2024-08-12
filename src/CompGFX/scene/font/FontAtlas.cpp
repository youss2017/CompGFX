#include "FontAtlas.hpp"
#include <algorithm>
#include <execution>
#include <stb/stb_image_write.h>
using namespace std;
using namespace egx;

FontAtlas& egx::FontAtlas::LoadTTFont(const std::string& file)
{
	m_TTFile = cpp::ReadAllBytes(file);
	if (!m_TTFile) {
		throw runtime_error(cpp::Format("Could not read file {{{}}}", file));
	}
	m_StbFontInfoStructure = {};
	stbtt_InitFont(&m_StbFontInfoStructure, m_TTFile->data(), stbtt_GetFontOffsetForIndex(m_TTFile->data(), 0));
	return *this;
}

void egx::FontAtlas::BuildAtlas(float fontSize, bool sdf, bool multithreaded)
{
	vector<tuple<wchar_t, int, int, vector<uint8_t>>> unordered_bitmaps;
	vector<tuple<wchar_t, int, int, vector<uint8_t>>> bitmaps;
	int total_pixel_area = 0;

	mutex m;
	auto gen_codepoint = [&](wchar_t ch) {
		uint32_t w, h;
		vector<uint8_t> character_bitmap;
		if (sdf)
			character_bitmap = _GenerateSdfCodepoint(fontSize, ch, &w, &h);
		else
			character_bitmap = _GenerateCodepoint(fontSize, ch, &w, &h);
		m.lock();
		unordered_bitmaps.push_back({ ch, w, h, move(character_bitmap) });
		total_pixel_area += w * h;
		m.unlock();
	};

	if (multithreaded) {
		for_each(execution::par_unseq, m_CharacterSet.begin(), m_CharacterSet.end(), gen_codepoint);
	}
	else {
		for_each(execution::seq, m_CharacterSet.begin(), m_CharacterSet.end(), gen_codepoint);
	}

	// order bitmaps
	for (auto& ch : m_CharacterSet) {
		auto item = find_if(unordered_bitmaps.begin(), unordered_bitmaps.end(), [ch](tuple<wchar_t, int, int, vector<uint8_t>>& e) { return ch == get<0>(e); });
		auto& [_, w, h, bp] = *item;
		bitmaps.push_back({ ch, w, h, move(bp) });
		unordered_bitmaps.erase(item);
	}
	bitmaps.shrink_to_fit();

	// 1) determine bitmap resolution
	int dimension = 1 << 8;
	while (dimension * dimension <= total_pixel_area) dimension <<= 1;
	dimension <<= 1;

	CharMap.clear();
	map<wchar_t, CharacterInfo>& character_map = CharMap;

	vector<uint8_t> font_bitmap(dimension * dimension);
	int cursor_x = 0;
	int cursor_y = 0;
	int line_max_height = 0;
	int line_character_count = 0;
	int font_map_height = 0;
	for (auto& [ch, w, h, ch_bitmap] : bitmaps) {
		CharacterInfo cinfo{};
		cinfo.ch = ch, cinfo.width = w, cinfo.height = h;

		// Load Font Metrics
		int glyphIndex = stbtt_FindGlyphIndex(&m_StbFontInfoStructure, ch);
		int advanceWidth, leftSideBearing;
		stbtt_GetGlyphHMetrics(&m_StbFontInfoStructure, glyphIndex, &advanceWidth, &leftSideBearing);

		int x0, y0, x1, y1;
		stbtt_GetGlyphBitmapBox(&m_StbFontInfoStructure, glyphIndex, fontSize, fontSize, &x0, &y0, &x1, &y1);

		cinfo.leftSideBearing = leftSideBearing * fontSize; // Equivalent to bitmap_left
		cinfo.topSideBearing = y1; // Equivalent to bitmap_top

		cinfo.advanceX = advanceWidth * fontSize; // Equivalent to advance.x

		if (dimension - cursor_x >= w) {
			// add character at end of line
			cinfo.start_x = cursor_x;
			cinfo.start_y = cursor_y;
			line_max_height = std::max(h, line_max_height);
			line_character_count++;
		}
		else {
			// go to next line
			line_character_count = 0;
			cursor_x = 0;
			cursor_y += line_max_height;
			line_max_height = 0;
			// add character
			cinfo.start_x = cursor_x;
			cinfo.start_y = cursor_y;
		}

		// optimize cursor_y for character
		if (cursor_y > 0) {
			// loop through all character that are behind this character
			vector<CharacterInfo> overlapping_characters;
			overlapping_characters.reserve(5);
			for (int32_t i = (int32_t)character_map.size() - line_character_count - 1; i >= 0; i--) {
				const auto& previous_character = character_map[i];
				int x_min = cinfo.start_x;
				int x_max = x_min + cinfo.width;
				int px_min = previous_character.start_x;
				int px_max = px_min + previous_character.width;
				if (/* is x_min inside [px_min, px_max] */
					(x_min >= px_min && x_min < px_max) ||
					/* is x_max inside [px_min, px_max] */
					(x_max >= px_min && x_max < px_max)) {
					overlapping_characters.push_back(previous_character);
				}
				if (px_min == 0) {
					// we are at the begining of the previous line
					// therefore we can break
					break;
				}
			}
			// adjust start_y from the maximum height of all overlappying characters
			int temp = cinfo.start_y;
			cinfo.start_y = 0;
			for (auto& overlapping_ch : overlapping_characters) {
				int start_y = overlapping_ch.start_y + overlapping_ch.height;
				cinfo.start_y = std::max(cinfo.start_y, start_y) + 2;
			}
		}
		// copy character bitmap to font bitmap
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				uint8_t value = ch_bitmap[y * w + x];
				size_t index = dimension * (cinfo.start_y + y) + (cinfo.start_x + x);
				font_bitmap[index] = value;
			}
		}
		font_map_height = std::max(font_map_height, cinfo.start_y + cinfo.height);
		cursor_x += w + 2;
		character_map[ch] = cinfo;
		ch_bitmap.clear();
	}

	// shrink font map
	if (m_OptimalMemoryAccess) {
		int reduced_height = dimension;
		while (font_map_height < reduced_height) {
			reduced_height >>= 1;
			if (reduced_height < font_map_height) {
				reduced_height <<= 1;
				break;
			}
		}
		font_map_height = reduced_height;
		font_bitmap.resize(/* width */ dimension * /* height */ font_map_height);
	}
	else {
		font_bitmap.resize(/* width */ dimension * /* height */ font_map_height);
	}

	AtlasBmp = move(font_bitmap);
	AtlasWidth = dimension;
	AtlasHeight = font_map_height;
}

std::vector<uint8_t> egx::FontAtlas::_GenerateCodepoint(float fontSize, wchar_t ch, uint32_t* pWidth, uint32_t* pHeight)
{
	int w = 0, h = 0;
	auto bmp_raw = stbtt_GetCodepointBitmap(&m_StbFontInfoStructure, 0, stbtt_ScaleForPixelHeight(&m_StbFontInfoStructure, fontSize), ch, &w, &h, 0, 0);
	vector<uint8_t> bmp(w * h);
	::memcpy(bmp.data(), bmp_raw, static_cast<size_t>(w) * h);
	::free(bmp_raw);
	*pWidth = w, * pHeight = h;
	return bmp;
}

std::vector<uint8_t> egx::FontAtlas::_GenerateCodepointAligned(float fontSize, wchar_t ch, uint32_t* pWidth, uint32_t* pHeight, uint8_t alignment)
{
	int w = 0, h = 0;
	auto bmp_raw = stbtt_GetCodepointBitmap(&m_StbFontInfoStructure, 0, stbtt_ScaleForPixelHeight(&m_StbFontInfoStructure, fontSize), ch, &w, &h, 0, 0);
	vector<uint8_t> bmp((w + alignment) * h);
	for (int y = 0; y < h; y++) {
		memcpy(&bmp[y * (w + alignment)], &bmp_raw[y * w], w);
		//for (int x = 0; x < w; x++) {
		//bmp[y * (w + alignment) + x] = bmp_raw[y * w + x];
		//}
	}
	::free(bmp_raw);
	*pWidth = w, * pHeight = h;
	return bmp;
}

std::vector<uint8_t> egx::FontAtlas::_GenerateSdfCodepoint(float fontSize, wchar_t ch, uint32_t* pWidth, uint32_t* pHeight, uint32_t targetResolution)
{
	// develop sdf
	int upscale_resolution;
	if (targetResolution == 0) {
		upscale_resolution = std::min(int(fontSize) << 3, 2048);
		if (fontSize > upscale_resolution) {
			upscale_resolution = int(fontSize);
		}
	}
	else {
		upscale_resolution = targetResolution;
	}
	const int32_t spread = upscale_resolution / 2;
	int32_t up_w, up_h;

	auto upscale_bitmap = _GenerateCodepoint(float(upscale_resolution), ch, (uint32_t*)&up_w, (uint32_t*)&up_h);

	float widthScale = up_w / (float)upscale_resolution;
	float heightScale = up_h / (float)upscale_resolution;
	int characterWidth = int(fontSize * widthScale);
	int characterHeight = int(fontSize * heightScale);
	float bitmapScaleX = up_w / (float)characterWidth;
	float bitmapScaleY = up_h / (float)characterHeight;
	vector<uint8_t> sdf_bitmap(characterWidth * characterHeight);

	for (int y = 0; y < characterHeight; y++) {
		for (int x = 0; x < characterWidth; x++) {
			// map from [0, characterWidth] (font size scale) to [0, up_w]
			int32_t pixelX = int32_t((x / (float)characterWidth) * up_w);
			int32_t pixelY = int32_t((y / (float)characterHeight) * up_h);
			///////////////////// find nearest pixel

			auto read_pixel = [](const vector<uint8_t>& bitmap, int x, int y, int width, int height) -> bool {
				if (x < 0 || x >= width || y < 0 || y >= height) return false;
				uint8_t value = bitmap[y * width + x];
				return value == 0xFF;
			};
			int32_t minX = max(pixelX - spread, 0);
			int32_t maxX = min(pixelX + spread, up_w);
			int32_t minY = max(pixelY - spread, 0);
			int32_t maxY = min(pixelY + spread, up_h);
			int32_t minDistance = spread * spread;

			if (m_OptimalQuaility) {
				for (uint32_t yy = minY; yy < maxY; yy++) {
					for (uint32_t xx = minX; xx < maxX; xx++) {
						bool pixelState = upscale_bitmap[yy * up_w + xx] == 0xff;
						if (pixelState) {
							int32_t dxSquared = (xx - pixelX) * (xx - pixelX);
							int32_t dySquared = (yy - pixelY) * (yy - pixelY);
							int32_t distanceSquared = dxSquared + dySquared;
							minDistance = std::min(minDistance, distanceSquared);
							if (xx >= pixelX) {
								break;
							}
						}
					}
				}
			}
			else {
				for (int32_t yy = minY; yy < maxY; yy++) {
					bool pixelState = upscale_bitmap[yy * up_w + pixelX];
					if (pixelState) {
						int32_t dxSquared = 0;
						int32_t dySquared = (yy - pixelY) * (yy - pixelY);
						int32_t distanceSquared = dxSquared + dySquared;
						minDistance = std::min(minDistance, distanceSquared);
					}
				}
				for (uint32_t  xx = minX; xx < maxX; xx++) {
					bool pixelState = upscale_bitmap[pixelY * up_w + xx];
					if (pixelState) {
						int32_t dxSquared = (xx - pixelX) * (xx - pixelX);
						int32_t dySquared = 0;
						int32_t distanceSquared = dxSquared + dySquared;
						minDistance = std::min(minDistance, distanceSquared);
						if (xx >= pixelX) {
							break;
						}
					}
				}
			}
			float minimumDistance = sqrtf((float)minDistance);
			bool state = read_pixel(upscale_bitmap, pixelX, pixelY, up_w, up_h);
			float output = (minimumDistance - 0.5f) / (spread - 0.5f);
			output *= state == 0 ? -1 : 1;
			// Map from [-1, 1] to [1, 1]
			output = (output + 1.0f) * 0.5f;

			// store pixel
			sdf_bitmap[y * characterWidth + x] = uint8_t(output * 255.0f);
		}
	}
	*pWidth = characterWidth;
	*pHeight = characterHeight;
	return sdf_bitmap;
}

void FontAtlas::SaveBmp(const std::string& fileName) const {
	if (AtlasWidth <= 0 || AtlasHeight <= 0) return;
	stbi_write_bmp(fileName.data(), AtlasWidth, AtlasHeight, 1, AtlasBmp.data());
}

void FontAtlas::SavePng(const std::string& fileName) const {
	if (AtlasWidth <= 0 || AtlasHeight <= 0) return;
	stbi_write_png(fileName.data(), AtlasWidth, AtlasHeight, 1, AtlasBmp.data(), AtlasWidth);
}

std::vector<FontAtlas::QuadVertex> egx::FontAtlas::GenerateTextMesh(const std::wstring& text, int x, int y, int screenWidth, int screenHeight, float pixelSize)
{
	vector<QuadVertex> mesh(text.size() * 6);

	int i = 0;
	float xoffset = (2.0f * x / screenWidth) - 1.0f;
	float yoffset = (2.0f * y / screenHeight) - 1.0f;
	for (const wchar_t c : text) {
		const auto& bmpInfo = CharMap[c];
		float cWidth  = ((pixelSize / screenWidth)) / 2.0f;
		float cHeight = ((pixelSize / screenHeight)) / 2.0f;

		float sx = bmpInfo.start_x / (float)AtlasWidth;
		float sy = bmpInfo.start_y / (float)AtlasHeight;
		float ex = (bmpInfo.width / (float)AtlasWidth) + sx;
		float ey = (bmpInfo.height / (float)AtlasHeight) + sy;
		
		auto& v1 = mesh[i++];
		auto& v2 = mesh[i++];
		auto& v3 = mesh[i++];
		auto& v4 = mesh[i++];
		auto& v5 = mesh[i++];
		auto& v6 = mesh[i++];

		v1.x = xoffset;
		v1.y = yoffset;
		v1.u = sx;//0;
		v1.v = ey;//1.0f;

		v2.x = xoffset + cWidth;
		v2.y = yoffset;
		v2.u = ex;
		v2.v = ey;

		v3.x = xoffset;
		v3.y = yoffset + cHeight;
		v3.u = sx;
		v3.v = sy;

		v4.x = xoffset;
		v4.y = yoffset + cHeight;
		v4.u = sx;
		v4.v = sy;

		v5.x = xoffset + cWidth;
		v5.y = yoffset;
		v5.u = ex;
		v5.v = ey;

		v6.x = xoffset + cWidth;
		v6.y = yoffset + cHeight;
		v6.u = ex;
		v6.v = sy;

		xoffset += cWidth;
	}

	return mesh;
}