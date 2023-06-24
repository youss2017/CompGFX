#pragma once
#include <string>
#include <vector>
#include <optional>
#include <stb/stb_truetype.h>

namespace egx {

	class FontAtlas {
	public:
		struct CharacterInfo {
			wchar_t ch;
			int start_x;
			int start_y;
			int width;
			int height;
		};

	public:
		FontAtlas& LoadTTFont(const std::string& file);
		FontAtlas& LoadTTFont(const void* pFileMemoryStream, size_t length);
		/// <summary>
		/// Creates bitmap using ideal resolution
		/// ex: 32, 64, 128, 256 ...
		/// </summary>
		/// <returns></returns>
		FontAtlas& SetAtlasForOptimalMemoryAccess() {
			m_OptimalMemoryAccess = true;
			return *this;
		}
		/// <summary>
		/// Allow's non-ideal width and height
		/// </summary>
		/// <returns></returns>
		FontAtlas& SetAtlasForMinimalMemoryUsage() {
			m_OptimalMemoryAccess = false;
			return *this;
		}

		FontAtlas& SetCharacterSet(const std::wstring& characterSet) {
			m_CharacterSet = characterSet;
			return *this;
		}

		void BuildAtlas(float fontSize, bool sdf, bool multithreaded);
		void SaveBmp(const std::string& fileName) const;
		void SavePng(const std::string& fileName) const;

	public:
		std::vector<CharacterInfo> CharMap;
		std::vector<uint8_t> AtlasBmp;
		uint32_t AtlasWidth = 0;
		uint32_t AtlasHeight = 0;

	private:
		std::vector<uint8_t> _GenerateCodepoint(float fontSize, wchar_t ch, int* pWidth, int* pHeight);
		std::vector<uint8_t> _GenerateSdfCodepoint(float fontSize, wchar_t ch, int* pWidth, int* pHeight, int targetResolution = 0);
	private:
		std::wstring m_CharacterSet;
		std::optional<std::vector<uint8_t>> m_TTFile;
		stbtt_fontinfo m_StbFontInfoStructure;
		bool m_OptimalMemoryAccess = true;
	};

}