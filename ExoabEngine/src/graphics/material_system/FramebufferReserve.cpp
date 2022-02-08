#include "FramebufferReserve.hpp"
#include "../../utils/common.hpp"
#include "../../utils/StringUtils.hpp"
#include <cassert>
#include <iostream>

FramebufferReserve::FramebufferReserve(GraphicsContext context, ConfigurationSettings& cfg, const char* reserve_path)
{
	std::string raw_input = Utils::LoadTextFile(reserve_path);
	// 1) remove comments
	std::string input_commentless;
	input_commentless.reserve(raw_input.size());
	bool inside_comment = false;
	for (int i = 0; i < raw_input.length(); i++) {
		if (raw_input[i] == ';')
			inside_comment = true;
		if (raw_input[i] == '\n')
			inside_comment = false;
		if (!inside_comment)
			input_commentless += raw_input[i];
	}
	//std::cout << input_commentless << std::endl;
	// 2) Lower Case
	input_commentless = Utils::LowerCaseString(input_commentless);
	// 3) Split by lines
	std::vector<std::string> input_split = Utils::StringSplitter("\n", input_commentless);
	// 4) Parse Data
	struct ReserveAttachment {
		uint32_t width = -1, height = -1;
		TextureFormat format = TextureFormat::UNDEFINED;
		TextureSamples samples = TextureSamples::MSAA_1;
	};
	std::vector<ReserveAttachment> attachments;
	ReserveAttachment CurrentAttachment;
	using namespace Utils;
	for (int i = 0; i < input_split.size(); i++) {
		std::string& line = input_split[i];
		if (line.size() <= 2)
			continue;
		line = StrTrim(line);
		if (StrStartsWith(line, "attachment")) {
			CurrentAttachment = ReserveAttachment();
		}
		else if (StrStartsWith(line, "width")) {
			std::string temp = line.substr(7);
			if (StrContains(temp, "!resolution")) {
				CurrentAttachment.width = -1;
			}
			else {
				temp = StrTrim(temp);
				CurrentAttachment.width = ::atoi(temp.c_str());
			}
		}
		else if (StrStartsWith(line, "height")) {
			std::string temp = line.substr(8);
			if (StrContains(temp, "!resolution")) {
				CurrentAttachment.width = -1;
			}
			else {
				temp = StrTrim(temp);
				CurrentAttachment.width = ::atoi(temp.c_str());
			}
		}
		else if (StrStartsWith(line, "format")) {
			std::string temp = StrTrim(line.substr(8));
			CurrentAttachment.format = Textures_StringToTextureFormat(temp);
		}
		else if (StrStartsWith(line, "samples")) {
			if (StrContains(line, "1")) CurrentAttachment.samples = TextureSamples::MSAA_1;
			if (StrContains(line, "2")) CurrentAttachment.samples = TextureSamples::MSAA_2;
			if (StrContains(line, "4")) CurrentAttachment.samples = TextureSamples::MSAA_4;
			if (StrContains(line, "8")) CurrentAttachment.samples = TextureSamples::MSAA_8;
			if (StrContains(line, "16")) CurrentAttachment.samples = TextureSamples::MSAA_16;
			if (StrContains(line, "32")) CurrentAttachment.samples = TextureSamples::MSAA_32;
			if (StrContains(line, "64")) CurrentAttachment.samples = TextureSamples::MSAA_64;
		}
		else if (StrStartsWith(line, "end")) {
			attachments.push_back(CurrentAttachment);
		}
		else {
			std::vector<std::string> original_split = StringSplitter("\n", raw_input);
			std::string err_msg = "Invalid syntax [" + original_split[i + 1] + ", " + std::to_string(i + 1) + "] <" + std::string(reserve_path) + ">";
			std::cout << err_msg << std::endl;
			logerror(err_msg.c_str());
			throw std::runtime_error(err_msg);
		}
	}

	// 5) Create Attachments
	for (int i = 0; i < attachments.size(); i++) {
		GPUTexture2DSpecification spec;
		spec.m_Width = attachments[i].width == (uint32_t)-1 ? cfg.ResolutionWidth : attachments[i].width;
		spec.m_Height = attachments[i].height == (uint32_t)-1 ? cfg.ResolutionHeight : attachments[i].height;
		spec.m_TextureUsage = attachments[i].format == TextureFormat::D32F ? TextureUsage::DEPTH_ATTACHMENT : TextureUsage::COLOR_ATTACHMENT;
		spec.m_Samples = attachments[i].samples;
		spec.m_Format = attachments[i].format;
		spec.m_GenerateMipMapLevels = false;
		spec.m_CreatePerFrame = true;
		spec.m_LazilyAllocate = false; // TODO: Support Lazily Allocate
		m_attachments.push_back(GPUTexture2D_Create(context, &spec));
	}

}

FramebufferReserve::~FramebufferReserve()
{
	for (auto& a : m_attachments)
		GPUTexture2D_Destroy(a);
}
