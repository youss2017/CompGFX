#pragma once

// For some reason #define did not work, for example instead of 0xc the compiler would put 0x0c for variables but 0xc in if statement

constexpr int COMMAND_SWITCH_MATERIAL      = 0xc;
constexpr int COMMAND_STOP_SHADER_PROGRAM  = 0xb;
constexpr int COMMAND_SET_VIEWPORT         = 0xe;
constexpr int COMMAND_SET_SCISSOR          = 0xd;
constexpr int COMMAND_SET_RENDERSTATE      = 0xf;
constexpr int COMMAND_DRAW_ARRAYS          = 0x07;
constexpr int COMMAND_DRAW_INDEXED         = 0x071;
constexpr int COMMAND_SET_PUSHCONSTANT     = 0x072;
constexpr int COMMAND_DRAW_DEFAULT_ENTITIES= 0xfce;
