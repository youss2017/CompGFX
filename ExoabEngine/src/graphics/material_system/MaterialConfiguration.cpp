#include "MaterialConfiguration.hpp"
#include "../../core/backend/VkGraphicsCard.hpp"
#include "../../core/pipeline/Pipeline.hpp"
#include "../../utils/common.hpp"
#include "../../utils/StringUtils.hpp"
#include <iostream>
#include <string>

#define BasicError                                                                                                              \
    else                                                                                                                        \
    {                                                                                                                           \
        std::string error_msg = "Could not parse: [" + mc_comments[i] + "], line " + to_string(i + 1) + ", invalid parameter!"; \
        cout << error_msg << endl;                                                                                              \
        logerror(error_msg.c_str());                                                                                            \
        throw runtime_error(error_msg);                                                                                         \
    }

MaterialConfiguration::MaterialConfiguration(const char *material_config)
{
    std::string mc_string = Utils::LoadTextFile(material_config);
    std::vector<std::string> mc_comments = Utils::StringSplitter("\n", mc_string);
    std::vector<std::string> mc;
    // 1) Remove Comments
    for (int i = 0; i < mc_comments.size(); i++)
    {
        std::string &line = mc_comments[i];
        if (line.size() <= 1)
        {
            mc.push_back("\n"); // to have an accurate line count
            continue;
        }
        int comment_start_pos = 0;
        while (line[comment_start_pos++] != ';')
            if (comment_start_pos == line.size())
                break;
        if (comment_start_pos != 0)
        {
            std::string data = Utils::StrRemoveAll(Utils::StrTrim(line.substr(0, comment_start_pos)), ";");
            std::string temp = Utils::StrLowerCase(data);
            // To keep the case sensitivity of shader filenames/path
            if (Utils::StrStartsWith(temp, "vertexshader"))
            {
                std::string result = "vertexshader:" + data.substr(13);
                mc.push_back(result);
            }
            else if (Utils::StrStartsWith(temp, "fragmentshader"))
            {
                std::string result = "fragmentshader:" + data.substr(14);
                mc.push_back(result);
            }
            else
            {
                mc.push_back(temp);
            }
        }
    }
#if 0 // Print how the comments were removed.
    for(int i = 0; i < mc.size(); i++) {
        std::cout << mc[i] << std::endl;
    }
    std::cout << "================================" << std::endl;
#endif
    // 2) Parse Info

    /*
        Blend State Settings
    */
    MaterialBlendStateSettings CurrentBlend;
    bool InsideBlendSetting = false;
    /*
        Framebuffer
    */
    bool InsideFramebuffer = false, DetectedFramebuffer = false, InsideFramebufferAttachment = false;
    MaterialAttachment CurrentAttachment;
    int IndexCounter = 0;
    auto GetImageLayoutFromString = [](std::string input) throw()->VkImageLayout
    {
        input = Utils::StrUpperCase(input);
        if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_UNDEFINED"))
            return VK_IMAGE_LAYOUT_UNDEFINED;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_GENERAL"))
            return VK_IMAGE_LAYOUT_GENERAL;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL"))
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL"))
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL"))
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL"))
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL"))
            return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        else if (Utils::StrContains(input, "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR"))
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        else
        {
            assert(0);
            return VK_IMAGE_LAYOUT_MAX_ENUM;
        }
    };
#if 0
    /*
        Descriptor Sets
    */
    bool InsideSet = false, InsideBinding = false;
    MaterialBindingDesc CurrentBinding;
    MaterialSetDesc CurrentSet;
#endif
    using namespace Utils;
    using namespace std;
    for (int i = 0; i < mc.size(); i++)
    {
        std::string &line = mc[i];
        if (line.size() < 2)
            continue;
        if (!InsidePipeline && !InsideBlendSetting)
        {
            if (Utils::StrStartsWith(line, "vertexshader:"))
            {
                vertex_shader = Utils::StrTrim(line.substr(13));
                continue;
            }
            if (Utils::StrStartsWith(line, "fragmentshader:"))
            {
                fragment_shader = Utils::StrTrim(line.substr(16));
                continue;
            }
            if (Utils::StrStartsWith(line, "mc_id:"))
            {
                mc_id = std::stoi(Utils::StrTrim(line.substr(6)));
                continue;
            }
        }
        if (InsidePipeline)
        {
            if (Utils::StrStartsWith(line, "!use_default"))
            {
                // set up default
                cullmode = CullMode::CULL_BACK;
                CCWFrontFace = true;
                polygonmode = PolygonMode::FILL;
                DepthTestEnable = true;
                DepthWriteEnable = true;
                compareop = DepthFunction::LESS;
                samples = TextureSamples::MSAA_1;
                SampleShading = false;
                minSampleShading = 0.0f;
            }
            else if (Utils::StrStartsWith(line, "cull_mode"))
            {
                if (Utils::StrContains(line, "front"))
                    cullmode = CullMode::CULL_FRONT;
                else if (Utils::StrContains(line, "back"))
                    cullmode = CullMode::CULL_BACK;
                else if (Utils::StrContains(line, "front_and_back"))
                    cullmode = CullMode::CULL_FRONT_AND_BACK;
                else if (Utils::StrContains(line, "none"))
                    cullmode = CullMode::CULL_NONE;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "front_face"))
            {
                if (Utils::StrContains(line, "ccw"))
                    CCWFrontFace = true;
                else if (Utils::StrContains(line, "cw"))
                    CCWFrontFace = false;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "polygon_mode"))
            {
                if (Utils::StrContains(line, "fill"))
                    polygonmode = PolygonMode::FILL;
                else if (Utils::StrContains(line, "line"))
                    polygonmode = PolygonMode::LINE;
                else if (Utils::StrContains(line, "point"))
                    polygonmode = PolygonMode::POINT;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "depth_test_enable"))
            {
                if (Utils::StrContains(line, "true"))
                    DepthTestEnable = true;
                else if (Utils::StrContains(line, "false"))
                    DepthTestEnable = false;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "depth_write_enable"))
            {
                if (Utils::StrContains(line, "true"))
                    DepthWriteEnable = true;
                else if (Utils::StrContains(line, "false"))
                    DepthWriteEnable = false;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "depth_compare_op"))
            {
                if (Utils::StrContains(line, "less"))
                    compareop = DepthFunction::LESS;
                else if (Utils::StrContains(line, "never"))
                    compareop = DepthFunction::NEVER;
                else if (Utils::StrContains(line, "equal"))
                    compareop = DepthFunction::EQUAL;
                else if (Utils::StrContains(line, "lequal"))
                    compareop = DepthFunction::LESS_EQUAL;
                else if (Utils::StrContains(line, "greater"))
                    compareop = DepthFunction::GREATER;
                else if (Utils::StrContains(line, "nequal"))
                    compareop = DepthFunction::NOT_EQUAL;
                else if (Utils::StrContains(line, "gequal"))
                    compareop = DepthFunction::GREATER_EQUAL;
                else if (Utils::StrContains(line, "always"))
                    compareop = DepthFunction::ALWAYS;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "samples"))
            {
                if (Utils::StrContains(line, "1"))
                    samples = TextureSamples::MSAA_1;
                else if (Utils::StrContains(line, "2"))
                    samples = TextureSamples::MSAA_2;
                else if (Utils::StrContains(line, "4"))
                    samples = TextureSamples::MSAA_4;
                else if (Utils::StrContains(line, "8"))
                    samples = TextureSamples::MSAA_8;
                else if (Utils::StrContains(line, "16"))
                    samples = TextureSamples::MSAA_16;
                else if (Utils::StrContains(line, "32"))
                    samples = TextureSamples::MSAA_32;
                else if (Utils::StrContains(line, "64"))
                    samples = TextureSamples::MSAA_64;
                BasicError
            }
            else if (Utils::StrStartsWith(line, "sample_shading"))
            {
                if (Utils::StrContains(line, "disabled"))
                {
                    SampleShading = false;
                    minSampleShading = 0.0f;
                }
                else
                {
                    std::string val = Utils::StrTrim(line.substr(15));
                    SampleShading = true;
                    minSampleShading = ::atof(val.c_str());
                }
            }
            else if (Utils::StrStartsWith(line, "end"))
            {
                InsidePipeline = false;
            }
            else
            {
                cout << "Invalid contents inside Pipeline Settings, [" << mc_comments[i] << "], line: " << i << endl;
                throw runtime_error("Invalid contents inside Pipeline Settings, [" + mc_comments[i] + "], line: " + to_string(i));
            }
        }
        else if (InsideBlendSetting)
        {
            auto ParseBlendFactor = [](const std::string &src) throw()->VkBlendFactor
            {
                if (StrContains(src, "zero"))
                    return VK_BLEND_FACTOR_ZERO;
                else if (StrContains(src, "one"))
                    return VK_BLEND_FACTOR_ONE;
                else if (StrContains(src, "src_color"))
                    return VK_BLEND_FACTOR_SRC_COLOR;
                else if (StrContains(src, "one_minus_src_color"))
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                else if (StrContains(src, "dst_color"))
                    return VK_BLEND_FACTOR_DST_COLOR;
                else if (StrContains(src, "one_minus_dst_color"))
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                else if (StrContains(src, "src_alpha"))
                    return VK_BLEND_FACTOR_SRC_ALPHA;
                else if (StrContains(src, "one_minus_src_alpha"))
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                else if (StrContains(src, "dst_alpha"))
                    return VK_BLEND_FACTOR_DST_ALPHA;
                else if (StrContains(src, "one_minus_dst_alpha"))
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                else
                {
                    logerror("Invalid blend factor!");
                }
                return VK_BLEND_FACTOR_ONE;
            };

            auto ParseBlendOp = [](const std::string &src) throw()->VkBlendOp
            {
                if (StrContains(src, "add"))
                    return VK_BLEND_OP_ADD;
                else if (StrContains(src, "subtract"))
                    return VK_BLEND_OP_SUBTRACT;
                else if (StrContains(src, "reverse_subtract"))
                    return VK_BLEND_OP_REVERSE_SUBTRACT;
                else if (StrContains(src, "min"))
                    return VK_BLEND_OP_MIN;
                else if (StrContains(src, "max"))
                    return VK_BLEND_OP_MAX;
                else
                {
                    logerror("Invalid blend factor!");
                }
                return VK_BLEND_OP_ADD;
            };

            if (StrStartsWith(line, "!use_default"))
            {
                CurrentBlend = MaterialBlendStateSettings();
            }
            else if (StrStartsWith(line, "blend_enable"))
            {
                if (Utils::StrContains(line, "true"))
                    CurrentBlend.blend_enable = true;
                else if (Utils::StrContains(line, "false"))
                    CurrentBlend.blend_enable = false;
                BasicError
            }
            else if (StrStartsWith(line, "src_color_blend_factor"))
            {
                CurrentBlend.src_color_blend_factor = ParseBlendFactor(line);
            }
            else if (StrStartsWith(line, "dst_color_blend_factor"))
            {
                CurrentBlend.dst_color_blend_factor = ParseBlendFactor(line);
            }
            else if (StrStartsWith(line, "color_blend_op"))
            {
                CurrentBlend.color_blend_op = ParseBlendOp(line);
            }
            else if (StrStartsWith(line, "src_alpha_blend_factor"))
            {
                CurrentBlend.src_alpha_blend_factor = ParseBlendFactor(line);
            }
            else if (StrStartsWith(line, "dst_alpha_blend_factor"))
            {
                CurrentBlend.dst_alpha_blend_factor = ParseBlendFactor(line);
            }
            else if (StrStartsWith(line, "alpha_blend_op"))
            {
                CurrentBlend.alpha_blend_op = ParseBlendOp(line);
            }
            else if (StrStartsWith(line, "color_write_mask"))
            {
                CurrentBlend.ColorWriteMask_r = CurrentBlend.ColorWriteMask_g = CurrentBlend.ColorWriteMask_b = CurrentBlend.ColorWriteMask_a = false;
                if (StrContains(line, "r"))
                    CurrentBlend.ColorWriteMask_r = true;
                if (StrContains(line, "g"))
                    CurrentBlend.ColorWriteMask_g = true;
                if (StrContains(line, "b"))
                    CurrentBlend.ColorWriteMask_b = true;
                if (StrContains(line, "a"))
                    CurrentBlend.ColorWriteMask_a = true;
            }
            else if (StrStartsWith(line, "end"))
            {
                InsideBlendSetting = false;
                m_blend_settings.push_back(CurrentBlend);
            }
            BasicError
        }
        else if (InsideFramebuffer)
        {
            if (InsideFramebufferAttachment)
            {
                if (StrStartsWith(line, "blend_state"))
                {
                    bool fail;
                    CurrentAttachment.blend_state_id = StrGetFirstNumber(line, fail);
                    assert(!fail);
                }
                else if (StrStartsWith(line, "loadop"))
                {
                    if (StrContains(line, "dont_care"))
                    {
                        CurrentAttachment.m_loadop = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    }
                    else if (StrContains(line.substr(7), "load"))
                    {
                        CurrentAttachment.m_loadop = VK_ATTACHMENT_LOAD_OP_LOAD;
                    }
                    else if (StrContains(line, "clear"))
                    {
                        CurrentAttachment.m_loadop = VK_ATTACHMENT_LOAD_OP_CLEAR;
                        int index = StrFindFirstIndexOf(line, "clear") + 5;
                        std::string temp = StrRemoveAll(StrTrim(line.substr(index)), "f");
                        std::vector<std::string> inital_split = StringSplitter(",", temp);
                        std::string combined_split = StrReplaceAll(StringCombiner(inital_split), "  ", " ");
                        std::vector<std::string> clear_vals = StringSplitter(" ", combined_split);
                        CurrentAttachment.m_clearvalue.color = {};
                        for (int i = 0; i < clear_vals.size() && i < 4; i++)
                        {
                            CurrentAttachment.m_clearvalue.color.float32[i] = std::stof(clear_vals[i]);
                        }
                    }
                    BasicError
                }
                else if (StrStartsWith(line, "storeop"))
                {
                    if (StrContains(line, "dont_care"))
                    {
                        CurrentAttachment.m_storeop = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    }
                    else if (StrContains(line, "store"))
                    {
                        CurrentAttachment.m_storeop = VK_ATTACHMENT_STORE_OP_STORE;
                    }
                    BasicError
                }
                else if (StrStartsWith(line, "initiallayout"))
                {
                    CurrentAttachment.m_initialLayout = GetImageLayoutFromString(line);
                }
                else if (StrStartsWith(line, "imagelayout"))
                {
                    CurrentAttachment.m_imageLayout = GetImageLayoutFromString(line);
                }
                else if (StrStartsWith(line, "finallayout"))
                {
                    CurrentAttachment.m_finialLayout = GetImageLayoutFromString(line);
                }
                else if (StrStartsWith(line, "format"))
                {
                    std::string format_str = StrTrim(line.substr(8));
                    CurrentAttachment.format = VK_FORMAT_UNDEFINED;//Textures_StringToTextureFormat(format_str);
                }
                else if (StrStartsWith(line, "enda"))
                {
                    m_attachments.push_back(CurrentAttachment);
                    InsideFramebufferAttachment = false;
                }
                BasicError
            }
            else
            {
                if (StrStartsWith(line, "width"))
                {
                    std::string temp = StrTrim(line.substr(6));
                    if (StrContains(temp, "!resolution"))
                        m_framebuffer_width = -1;
                    else
                    {
                        m_framebuffer_width = ::atoi(temp.c_str());
                    }
                }
                else if (StrStartsWith(line, "height"))
                {
                    std::string temp = StrTrim(line.substr(7));
                    if (StrContains(temp, "!resolution"))
                        m_framebuffer_height = -1;
                    else
                    {
                        m_framebuffer_height = ::atoi(temp.c_str());
                    }
                }
                else if (StrStartsWith(line, "!attachment"))
                {
                    CurrentAttachment = MaterialAttachment();
                    int a = StrFindFirstIndexOf(line, "(") + 1;
                    int b = StrFindFirstIndexOf(line, ")");
                    std::string temp = StrTrim(line.substr(a, b));
                    CurrentAttachment.m_reserve_id = ::atoi(temp.c_str());
                    InsideFramebufferAttachment = true;
                }
                else if (StrStartsWith(line, "attachment"))
                {
                    CurrentAttachment = MaterialAttachment();
                    CurrentAttachment.m_reserve_id = (uint32_t)-1;
                    InsideFramebufferAttachment = true;
                }
                else if (StrStartsWith(line, "end"))
                {
                    InsideFramebuffer = false;
                }
                BasicError
            }
        }
#if 0
        else if (InsideSet)
        {
            auto GetArraySize = [](std::string & input) throw()->int
            {
                int a = StrFindFirstIndexOf(input, "[");
                if (a == -1)
                    return 1;
                int b = StrFindFirstIndexOf(input, "]");
                std::string temp = StrTrim(input.substr(a + 1, b));
                return ::atoi(temp.c_str());
            };

            if (StrStartsWith(line, "binding"))
            {
                if (InsideBinding) {
                    if (InsideBinding) {
                        std::string message = "Invalid Syntax, starting a new binding while inside binding, [" + line + " :: " + std::to_string(i + 1) + "], file: " + std::string(material_config);
                        logerrors(message);
                        assert(0);
                    }
                }
                bool StageFound = false;
                InsideBinding = true;
                CurrentBinding = MaterialBindingDesc();
                if (StrContains(line, "vert")) {
                    StageFound = true;
                    CurrentBinding.m_stages |= VK_SHADER_STAGE_VERTEX_BIT;
                }
                if (StrContains(line, "frag")) {
                    StageFound = true;
                    CurrentBinding.m_stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
                }
                if (!StageFound) {
                    logwarning("No Shader Stage specified for Binding! Settings VK_SHADER_STAGE_ALL_GRAPHICS");
                    CurrentBinding.m_stages = VK_SHADER_STAGE_ALL_GRAPHICS;
                }
                CurrentBinding.m_istexture = false;
                bool fail;
                int num = StrGetFirstNumber(line, fail);
                CurrentBinding.m_arraysize = fail ? 1 : num;
            }
            else if (StrStartsWith(line, "texture2d_binding"))
            {
                CurrentBinding = MaterialBindingDesc();
                CurrentBinding.m_istexture = true;
                CurrentBinding.m_texturedimensions = 2;
                CurrentBinding.m_stages = VK_SHADER_STAGE_FRAGMENT_BIT;
                CurrentBinding.m_arraysize = GetArraySize(line);
                CurrentSet.m_bindings.push_back(CurrentBinding);
            }
            /////////////////////////// DATA TYPES
            else if (StrContains(line, "mat4"))
            {
                MaterialBindingElement e;
                e.m_size = sizeof(float) * 4 * 4;
                e.m_type = MaterialBindingElementType::mat4;
                e.m_arraysize = GetArraySize(line);
                CurrentBinding.m_size += e.m_size * e.m_arraysize;
                CurrentBinding.m_elements.push_back(e);
            }
            else if (StrContains(line, "vec4"))
            {
                MaterialBindingElement e;
                e.m_size = sizeof(float) * 4;
                e.m_type = MaterialBindingElementType::vec4;
                e.m_arraysize = GetArraySize(line);
                CurrentBinding.m_size += e.m_size * e.m_arraysize;
                CurrentBinding.m_elements.push_back(e);
            }
            else if (StrContains(line, "vec2"))
            {
                MaterialBindingElement e;
                e.m_size = sizeof(float) * 2;
                e.m_type = MaterialBindingElementType::vec2;
                e.m_arraysize = GetArraySize(line);
                CurrentBinding.m_size += e.m_size * e.m_arraysize;
                CurrentBinding.m_elements.push_back(e);
            }
            /////////////////////////// DATA TYPES
            else if (StrStartsWith(line, "endb"))
            {
                InsideBinding = false;
                CurrentSet.m_bindings.push_back(CurrentBinding);
            }
            else if (StrStartsWith(line, "end"))
            {
                if (InsideBinding) {
                    std::string message = "Invalid Syntax, ending set while inside binding, [" + line + " :: " + std::to_string(i + 1) + "], file: " + std::string(material_config);
                    logerrors(message);
                    assert(0);
                }
                m_sets.push_back(CurrentSet);
                InsideSet = false;
            }
            BasicError
        }
#endif
        else
        {
            if (StrStartsWith(line, "pipeline"))
            {
                InsidePipeline = true;
                if (DetectedPipeline)
                    throw runtime_error("Can only have one pipeline in material configuration!");
                DetectedPipeline = true;
            }
            else if (StrStartsWith(line, "blendstate"))
                InsideBlendSetting = true;
            else if (StrStartsWith(line, "framebuffer"))
            {
                if (DetectedFramebuffer)
                    throw runtime_error("Can only have one framebuffer in material configuration!");
                DetectedFramebuffer = true;
                InsideFramebuffer = true;
            }
#if 0
            else if (StrStartsWith(line, "set"))
            {
                InsideSet = true;
                CurrentSet = MaterialSetDesc();
            }
#endif
            else
            {
                cout << "Invalid section in material configuration! [" + mc_comments[i] + ", " + to_string(i) + "]" << endl;
                throw runtime_error("Invalid section in material configuration! [" + mc_comments[i] + ", " + to_string(i) + "]");
            }
        }
    }
}
