#include "Font.hpp"
#include "../utils/StringUtils.hpp"
#include "../utils/common.hpp"
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <stb_image.h>

Font::Font(const char* fntFile, const char* fntTextureDirectory)
{
    // open file
    assert(fntFile);
    assert(fntTextureDirectory);
    assert(Utils::Exists(fntFile) && "Font File does not exist");
    FILE* handle = fopen(fntFile, "r");
    assert(handle);
    // load file into memory
    std::vector<std::string> fntLines;
    while (!feof(handle))
        fntLines.push_back(Utils::StrTrim(Utils::ReadLine(handle)));
    fclose(handle);
    // parse file
    std::string image_name;
    for (const auto& line : fntLines)
    {
        // parse character information
        if (Utils::StrStartsWith(line, "char id"))
        {
            bool fail;
            int charID = Utils::StrGetFirstNumber(line, fail);
            assert(!fail);

            auto nextNumberLine = line.substr(Utils::StrFindFirstIndexOf(line, "x"));
            int xpos = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            nextNumberLine = nextNumberLine.substr(Utils::StrFindFirstIndexOf(nextNumberLine, "y"));
            int ypos = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            nextNumberLine = nextNumberLine.substr(Utils::StrFindFirstIndexOf(nextNumberLine, "width"));
            int width = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            nextNumberLine = nextNumberLine.substr(Utils::StrFindFirstIndexOf(nextNumberLine, "height"));
            int height = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            nextNumberLine = nextNumberLine.substr(Utils::StrFindFirstIndexOf(nextNumberLine, "xoffset"));
            int xoffset = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            nextNumberLine = nextNumberLine.substr(Utils::StrFindFirstIndexOf(nextNumberLine, "yoffset"));
            int yoffset = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            nextNumberLine = nextNumberLine.substr(Utils::StrFindFirstIndexOf(nextNumberLine, "xadvance"));
            int xadvance = Utils::StrGetFirstNumber(nextNumberLine, fail);
            assert(!fail);

            //std::cout << "Character ID=" << (char)charID << " x=" << xpos 
            //<< " y=" << ypos << " width=" << width << " height=" << height << " xoffset=" << xoffset
            //<< " yoffset=" << yoffset << " xadvance=" << xadvance << "\n";

            FontCharacterInfo characterInfo;
            characterInfo.characterExists = true;
            characterInfo.ID = charID;
            characterInfo.x = xpos;
            characterInfo.y = ypos;
            characterInfo.width = width;
            characterInfo.height = height;
            characterInfo.xoffset = xoffset;
            characterInfo.yoffset = yoffset;
            characterInfo.xadvance = xadvance;
            characters.push_back(characterInfo);
        }
        // get line height
        if (Utils::StrStartsWith(line, "common") && Utils::StrContains(line, "lineHeight"))
        {
            bool fail;
            lineHeight = Utils::StrGetFirstNumber(line.substr(Utils::StrFindFirstIndexOf(line, "lineHeight")), fail);
            assert(!fail && "Could not parse lineHeight in .fnt file!");
        }
        // determine texture image name
        if (Utils::StrContains(line, "file"))
        {
            image_name = line.substr(Utils::StrFindFirstIndexOf(line, "file") + strlen("file=\""));
            image_name = image_name.substr(0, Utils::StrFindFirstIndexOf(image_name, "\""));
            std::cout << "Font ---> '" << fntFile << "' texture file name = '" << image_name << "'\n";
        }
        // get the font character count
        if (Utils::StrStartsWith(line, "chars count="))
        {
            std::string characterCountStr = line.substr(Utils::StrFindFirstIndexOf(line, "chars count=" + 1));
            this->characterCount = atoi(characterCountStr.c_str());
        }
    }
    assert(image_name.size() > 0);
    // load texture
    if (strlen(fntTextureDirectory) == 0)
    {
        std::string error_image = "Could not load font texture --> '" + image_name + "'\n";
        if (!Utils::Exists(image_name.c_str()))
        {
            std::cout << error_image;
            assert(0 && "Could not load font texture!");
        }
        int channels;
        texturePixels = (uint32_t*)stbi_load(image_name.c_str(), &textureWidth, &textureHeight, &channels, 4);
        assert(texturePixels);
        std::cout << "Loaded Font Texture --> '" << image_name << "'\n";
    }
    else
    {
        std::string image_path = fntTextureDirectory;
        if (image_path[image_path.size() - 1] != '/' && image_path[image_path.size() - 1] != '\\')
        {
            image_path += '/';
        }
        image_path += image_name;
        std::string error_image = "Could not load font texture --> '" + image_path + "'\n";
        if (!Utils::Exists(image_path.c_str()))
        {
            std::cout << error_image;
            assert(0 && "Could not load font texture!");
        }
        int channels;
        texturePixels = (uint32_t*)stbi_load(image_path.c_str(), &textureWidth, &textureHeight, &channels, 4);
        assert(texturePixels);
        std::cout << "Loaded Font Texture --> '" << image_path << "'\n";
    }
}

Font::~Font()
{
    free(texturePixels);
}

FontCharacterInfo Font::GetCharacter(char c)
{
    for (const auto& info : characters)
    {
        if (info.ID == c)
            return info;
    }
    return { false };
}