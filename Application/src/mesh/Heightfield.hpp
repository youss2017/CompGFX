#pragma once
#include <string_view>

class Heightfield {

public:
	Heightfield(std::string_view path);
	~Heightfield();

	Heightfield(const Heightfield& other) = delete;
	Heightfield(const Heightfield&& other) = delete;

private:

};
