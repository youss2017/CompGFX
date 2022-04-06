#pragma once
#include <string>
#include <vector>

namespace Utils {

	std::string CreateDialogFilter(std::vector<std::pair<std::string, std::string>> items);
	std::string SaveAsDialog(const std::string& filter, int& OutChoosenFilterIndex);

}
