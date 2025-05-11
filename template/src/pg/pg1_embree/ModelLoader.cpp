#include "stdafx.h"
#include "ModelLoader.h"

#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "MaterialDielectric.h"
#include "MaterialLambert.h"
#include "MaterialMirror.h"
#include "MaterialNormal.h"
#include "MaterialPhong.h"
#include "MaterialTransparent.h"
#include "raytracer.h"



TextureDepr* ModelLoader::TextureProxy(const std::string& full_name,
	std::map<std::string, TextureDepr*>& already_loaded_textures, const int flip, const bool single_channel) {
	std::map<std::string, TextureDepr*>::iterator already_loaded_texture = already_loaded_textures.find(full_name);
	TextureDepr* texture = nullptr;
	if (already_loaded_texture != already_loaded_textures.end())
	{
		texture = already_loaded_texture->second;
	}
	else
	{
		texture = new TextureDepr(full_name.c_str(),BILINEAR,REPEAT);// , flip, single_channel);
		if (!texture->isValid()){
			delete texture;
			return nullptr;
		}
		already_loaded_textures[full_name] = texture;
	}
	return texture;
}



//load all materials
void ModelLoader::loadMaterials(const std::string& directory, const aiScene& scene, std::vector<Material*>& materials) {

	std::map<std::string, TextureDepr*> already_loaded_textures;

	//skip the first index since it's just a default material
	for (int i = 1; i < scene.mNumMaterials; ++i) {
		const aiMaterial* const assimpMaterial = scene.mMaterials[i];

		aiString name;
		if (assimpMaterial->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS) {

			float clearcoat{ 0 };
			if (assimpMaterial->Get(AI_MATKEY_CLEARCOAT_FACTOR, clearcoat) == aiReturn_SUCCESS){
				
			}

			Material* material{ nullptr };

			if (clearcoat == 0)
				material = new MaterialNormal();
			else if (clearcoat == 1)
				material = new MaterialLambert();
			else if (clearcoat == 2)
				material = new MaterialPhong();
			else if (clearcoat == 3)
				material = new MaterialMirror();
			else if (clearcoat == 4)
				material = new MaterialDielectric();
			else if (clearcoat == 5)
				material = new MaterialTransparent();
			else
				material = new Material();

			material->set_name(name.C_Str());
			material->setAssimpMatIndex(i);

			//====================================================
			//gamma correctable values first
			aiColor3D aiAmbient{};
			if (assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, aiAmbient) == aiReturn_SUCCESS) {
				material->ambient = { aiAmbient.r,aiAmbient.g,aiAmbient.b };
				if (Raytracer::gammaCorrect)
					Utils::expand(material->ambient);
			}

			aiColor3D aiDiffuse{};
			if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse) == aiReturn_SUCCESS) {
				material->diffuse = { aiDiffuse.r,aiDiffuse.g,aiDiffuse.b };
				if (Raytracer::gammaCorrect)
					Utils::expand(material->diffuse);
			}

			aiColor3D aiSpecular{};
			if (assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular) == aiReturn_SUCCESS) {
				material->specular = { aiSpecular.r,aiSpecular.g,aiSpecular.b };
				if (Raytracer::gammaCorrect)
					Utils::expand(material->specular);
			}
			//====================================================
	
			//====================================================
			//remaining material attributes
			aiColor3D aiEmissive{};
			if (assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissive) == aiReturn_SUCCESS)
				material->emission = { aiEmissive.r,aiEmissive.g,aiEmissive.b };

			float aiShininess{};
			if (assimpMaterial->Get(AI_MATKEY_SHININESS, aiShininess) == aiReturn_SUCCESS)
				material->shininess_ = aiShininess;

			float aiIOR{};
			if (assimpMaterial->Get(AI_MATKEY_REFRACTI, aiIOR) == aiReturn_SUCCESS)
				material->ior = aiIOR;

			aiColor3D aiAttenuation{};
			if (assimpMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, aiAttenuation) == aiReturn_SUCCESS)
				material->attenuation = { aiAttenuation.r, aiAttenuation.g, aiAttenuation.b };
			//====================================================

			//====================================================
			//textures
			aiString texName;

			if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texName) == AI_SUCCESS){
				std::string fullName{ directory +  std::string{texName.C_Str()} };
				material->set_texture(Material::kDiffuseMapSlot, TextureProxy(fullName, already_loaded_textures));
				if (material->get_texture(Material::kDiffuseMapSlot) && Raytracer::gammaCorrect)
					material->get_texture(material->kDiffuseMapSlot)->expand();
			}

			
			if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texName) == AI_SUCCESS) {
				std::string fullName{ directory + std::string{texName.C_Str()} };
				material->set_texture(Material::kSpecularMapSlot, TextureProxy(fullName, already_loaded_textures));
				if (material->get_texture(Material::kSpecularMapSlot) && Raytracer::gammaCorrect)
					material->get_texture(material->kSpecularMapSlot)->expand();
			}
			
			if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), texName) == AI_SUCCESS) {
				std::string fullName{ directory + std::string{texName.C_Str()} };
				material->set_texture(Material::kShininessMapSlot, TextureProxy(fullName, already_loaded_textures));
			}

			if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texName) == AI_SUCCESS) {
				std::string fullName{ directory + std::string{texName.C_Str()} };
				material->set_texture(Material::kNormalMapSlot, TextureProxy(fullName, already_loaded_textures));
			}
			//====================================================

			materials.push_back(material);
		}
	}
}



void ModelLoader::loadOBJ(const char* file_name, std::vector<Surface*>& surfaces, std::vector<Material*>& materials,
                          const bool flip_yz, const glm::vec3 default_color) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(file_name, 
		aiProcess_Triangulate | 
		//aiProcess_GenSmoothNormals | 
		aiProcess_JoinIdenticalVertices | 
		aiProcess_OptimizeGraph | 
		aiProcess_OptimizeMeshes |
		aiProcess_CalcTangentSpace);

	if (scene == nullptr){
		std::cerr << importer.GetErrorString() << std::endl;
		exit(1);
	}

	auto lastSlashPos = std::string{ file_name }.find_last_of('/');
	std::string directory{file_name};
	directory = directory.substr(0, lastSlashPos + 1);

	loadMaterials(directory,*scene, materials);
	//loadMaterials("",*scene, materials);

	//aiMesh = surface
	for (int i = 0; i < scene->mNumMeshes; ++i) {
		const aiMesh* modelMesh = scene->mMeshes[i];
		
		std::vector<Vertex> surfaceVertices{};
		for (int j = 0; j < modelMesh->mNumFaces; ++j) {

			const aiFace face = modelMesh->mFaces[j];

			for (int k = 0; k < face.mNumIndices; ++k){
				auto triIndex = face.mIndices[k];

				auto aiPos = modelMesh->mVertices[triIndex];
				auto aiNormal = modelMesh->mNormals[triIndex];
				auto aiTangent = modelMesh->mTangents[triIndex];


				glm::vec2 texCoords{};
				if (modelMesh->mTextureCoords[0]) {
					auto aiTexCoords = modelMesh->mTextureCoords[0][triIndex];
					texCoords = { aiTexCoords.x, aiTexCoords.y };
				}

				glm::vec3 pos{ aiPos.x, aiPos.y, aiPos.z };
				glm::vec3 normal{ aiNormal.x, aiNormal.y, aiNormal.z };
				glm::vec3 tangent{ aiTangent.x, aiTangent.y, aiTangent.z };

				Vertex vertex{ pos,normal,{1,0,1},tangent, &texCoords,};
				surfaceVertices.push_back(vertex);
			}
		}
		Surface* surface = BuildSurface(std::string(modelMesh->mName.C_Str()), surfaceVertices);
		surface->set_material(materials.at(modelMesh->mMaterialIndex - 1));
		surfaces.push_back(surface);
	}
}

std::pair<std::vector<Triangle*>, std::vector<Triangle*>> ModelLoader::loadScene(const char* file_name, std::vector<Surface*>& surfaces, std::vector<Material*>& materials, const RTCDevice& device, const RTCScene& scene){
	loadOBJ(file_name, surfaces, materials);

	std::vector<Triangle*> tris{};
	std::vector<Triangle*> emissiveTris{};

	uint32_t triIdCtr{0};

	// surfaces loop
	for (auto surface : surfaces) {

		//if (surface->get_material()->isEmitter() && surface->get_material()->emission != glm::vec3{ 1.5f, 1.5f, 4.0f } /*&& surface->get_material()->emission != glm::vec3{ 0.00, 4.0, 4.0, }*/)
			//continue;

		RTCGeometry mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

		glm::vec3* vertices = static_cast<glm::vec3*>(rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
			sizeof(glm::vec3), 3 * surface->no_triangles()));

		glm::vec<3, uint32_t>* triangles = static_cast<glm::vec<3, uint32_t>*>(rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
			sizeof(glm::vec<3, uint32_t>), surface->no_triangles()));


		rtcSetGeometryUserData(mesh, (void*)(surface->get_material()));

		rtcSetGeometryVertexAttributeCount(mesh, 4);

		glm::vec3* normals = static_cast<glm::vec3*>(rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
			sizeof(glm::vec3), 3 * surface->no_triangles()));

		glm::vec2* tex_coords = static_cast<glm::vec2*>(rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
			sizeof(glm::vec2), 3 * surface->no_triangles()));

		float* ids = static_cast<float*>(rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, RTC_FORMAT_FLOAT,
			sizeof(float), 3 * surface->no_triangles()));

		glm::vec3* tangents = static_cast<glm::vec3*>(rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 3, RTC_FORMAT_FLOAT3,
			sizeof(glm::vec3), 3 * surface->no_triangles()));


		float areaOfObject{ 0 };

		// triangles loop
		for (int i = 0, k = 0; i < surface->no_triangles(); ++i)
		{
			Triangle& triangle = surface->get_triangle(i);

			// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex& vertex = triangle.vertex(j);

				vertices[k].x = vertex.position.x;
				vertices[k].y = vertex.position.y;
				vertices[k].z = vertex.position.z;

				normals[k].x = vertex.normal.x;
				normals[k].y = vertex.normal.y;
				normals[k].z = vertex.normal.z;

				tex_coords[k].x = vertex.texture_coords[0].x;
				tex_coords[k].y = vertex.texture_coords[0].y;

				tangents[k].x = vertex.tangent.x;
				tangents[k].y = vertex.tangent.y;
				tangents[k].z = vertex.tangent.z;

				if (surface->get_material()->isEmissive()) 
					ids[k] = static_cast<float>(triIdCtr);

				//triPointers[k] = &triangle;
			} // end of vertices loop

			triangles[i][0] = k - 3;
			triangles[i][1] = k - 2;
			triangles[i][2] = k - 1;

			if (surface->get_material()->isEmissive()){
				emissiveTris.push_back(&triangle);
				tris.push_back(&triangle);
				triIdCtr += 1;
				areaOfObject += triangle.getSurfaceArea();
			}

			//if (surface->get_material()->isEmitter() && surface->get_material()->emission == glm::vec3{ 0.00, 4.0, 4.0, } && i == 0)
				//break;

		} // end of triangles loop

		std::cout << areaOfObject << std::endl;

		rtcCommitGeometry(mesh);
		unsigned int geom_id = rtcAttachGeometry(scene, mesh);
		rtcReleaseGeometry(mesh);
	} // end of surfaces loop

	return { tris,emissiveTris };
}