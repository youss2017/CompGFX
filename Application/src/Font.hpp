#pragma once
#include <vector>

struct FontCharacterInfo
{
    bool characterExists; // flag to determine whether the character exists!
    char ID;
    int x;
    int y;
    int width;
    int height;
    int xoffset;
    int yoffset;
    int xadvance;
};

class Font
{
public:
    unsigned int* texturePixels;
    int textureWidth, textureHeight;
    std::vector<FontCharacterInfo> characters;
    int characterCount;
    int lineHeight;

    Font(const char* fntFile, const char* fntTextureDirectory);
    ~Font();

    FontCharacterInfo GetCharacter(char c);

};