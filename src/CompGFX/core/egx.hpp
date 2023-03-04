#pragma once
#include "memory/egxmemory.hpp"
#include "pipeline/egxframebuffer.hpp"
#include "pipeline/egxsampler.hpp"
#include "pipeline/egxpipeline.hpp"
#include "shaders/egxshader2.hpp"
#include "shaders/egxshaderset.hpp"
#include "egxengine.hpp"
#include "../cmd/cmd.hpp"

template<typename T>
using Ref = egx::ref<T>;

using Buffer = egx::Buffer;
using Image = egx::Image;

using RefBuffer = Ref<egx::Buffer>;
using RefImage = Ref<egx::Image>;

