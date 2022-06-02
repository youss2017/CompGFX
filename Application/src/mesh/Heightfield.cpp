#include "Heightfield.hpp"
#include <stdio.h>
#include <Windows.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/ProgressHandler.hpp>
#include <chrono>
#include <stb/stb_image.h>
#include <Logger.hpp>

using namespace glm;

Heightfield::Heightfield(std::string_view path) {

	Assimp::Importer imp;
	auto scene = imp.ReadFile(path.data(), aiProcessPreset_TargetRealtime_MaxQuality);

	// 1) Load Embedded Texture inside glTF 2.0 data file

	struct {
		int width = 0, height = 0;
		void* pixels = nullptr;
		bool stbMemory = false;
	} DiffuseTexture;

	if(scene->mNumTextures > 0) {
		auto m = scene->mMaterials[0];
		aiString name;
		aiGetMaterialString(m, AI_MATKEY_TEXTURE_DIFFUSE(0), &name);
		auto eTex = scene->GetEmbeddedTexture(name.C_Str());
		if (eTex) {
			if (eTex->mHeight == 0) {
				int c;
				DiffuseTexture.pixels = stbi_load_from_memory((stbi_uc*)eTex->pcData, eTex->mWidth, &DiffuseTexture.width, &DiffuseTexture.height, &c, 4);
				DiffuseTexture.stbMemory = true;
			}
			else {
				DiffuseTexture.width = eTex->mWidth;
				DiffuseTexture.height = eTex->mHeight;
				DiffuseTexture.pixels = eTex->pcData;
			}
		}
	}

	if (DiffuseTexture.stbMemory && DiffuseTexture.pixels)
		stbi_image_free(DiffuseTexture.pixels);

	if (scene->mNumMeshes != 1) {
		log_error("Map mesh must not contain more than 1 mesh or at least 1 mesh!", __FILE__, __LINE__, true);
	}

	auto mesh = scene->mMeshes[0];
	
	// sort vertices


	//printf("COL: %d\n", col);
	//exit(5);

	imp.FreeScene();
}

Heightfield::~Heightfield()
{
}
