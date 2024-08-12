#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stb/stb_truetype.h>
#include <mesh/MeshContainer.hpp>

namespace egx {

	class FontAtlas {
	public:
		struct CharacterInfo {
			wchar_t ch;
			int start_x;
			int start_y;
			int width;
			int height;
			int leftSideBearing;
			int topSideBearing;
			int advanceX;
		};

	public:
		FontAtlas& LoadTTFont(const std::string& file);
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

		/// <summary>
		/// A more accurate SDF at the cost
		/// of higher computation time.
		/// </summary>
		/// <returns></returns>
		FontAtlas& SetOptimalQuaility() {
			m_OptimalQuaility = true;
			return *this;
		}

		/// <summary>
		/// A significat reduction in atlas (for SDF) generation
		/// at the cost of visual artifacts.
		/// </summary>
		/// <returns></returns>
		FontAtlas& SetOptimalSpeed() {
			m_OptimalQuaility = false;
			return *this;
		}

		FontAtlas& SetCharacterSet(const std::wstring& characterSet) {
			m_CharacterSet = characterSet;
			return *this;
		}

		void BuildAtlas(float fontSize, bool sdf, bool multithreaded);
		void SaveBmp(const std::string& fileName) const;
		void SavePng(const std::string& fileName) const;

		struct QuadVertex {
			float x, y;
			float u, v;
		};

		std::vector<QuadVertex> GenerateTextMesh(const std::wstring& text, int x, int y, int screenWidth, int screenHeight, float pixelSize);

	public:
		std::map<wchar_t, CharacterInfo> CharMap;
		std::vector<uint8_t> AtlasBmp;
		uint32_t AtlasWidth = 0;
		uint32_t AtlasHeight = 0;

	private:
		std::vector<uint8_t> _GenerateCodepoint(float fontSize, wchar_t ch, uint32_t* pWidth, uint32_t* pHeight);
		std::vector<uint8_t> _GenerateCodepointAligned(float fontSize, wchar_t ch, uint32_t* pWidth, uint32_t* pHeight, uint8_t alignment);
		std::vector<uint8_t> _GenerateSdfCodepoint(float fontSize, wchar_t ch, uint32_t* pWidth, uint32_t* pHeight, uint32_t targetResolution = 0);
	private:
		std::wstring m_CharacterSet;
		std::optional<std::vector<uint8_t>> m_TTFile;
		stbtt_fontinfo m_StbFontInfoStructure;
		bool m_OptimalMemoryAccess = true;
		bool m_OptimalQuaility = true;
	};

}