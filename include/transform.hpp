#ifndef MODEL_H
#define MODEL_H

#include "common.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "utils/utils.hpp"

#include "mesh.hpp"
#include "shader.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

namespace LearningOpenGL {

    class Transform {
    public:
        /// Model data.
        std::vector<Texture> texturesLoaded;
        std::vector<Mesh> meshes;
        std::string directory;
        bool gammaCorrection;

        Transform() : gammaCorrection(false) {

        }

        /// Constructor, expects a filepath to a 3D model.
        Transform(std::string const& tPath, bool tGamma = false) : gammaCorrection(tGamma) {
            loadModel(tPath);
        }

        /// Renders the model.
        void draw(Shader* tShader) {
            for(unsigned int i = 0; i < meshes.size(); i++)
                meshes[i].draw(tShader);
        }

    private:
        /// Loads a model with ASSIMP.
        void loadModel(std::string const& tPath) {
            // Read file via ASSIMP.
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(tPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            // Check for errors.
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                LOG_ERRR("Error while importing model: ", importer.GetErrorString());
                return;
            }
            // Retrieve the directory path of the filepath.
            directory = tPath.substr(0, tPath.find_last_of('/'));

            processNode(scene->mRootNode, scene);
            LOG_INFO("Loaded model at \"", tPath.c_str(), "\"")
        }

        /// Processes a node in a recursive fashion.
        void processNode(aiNode* tNode, const aiScene* tScene) {
            // Process each mesh located at the current node.
            for (unsigned int i = 0; i < tNode->mNumMeshes; i++) {
                aiMesh* mesh = tScene->mMeshes[tNode->mMeshes[i]];
                meshes.push_back(processMesh(mesh, tScene));
            }
            // Process each of the children nodes.
            for (unsigned int i = 0; i < tNode->mNumChildren; i++)
                processNode(tNode->mChildren[i], tScene);

        }

        /// Processes all vertex data to a Mesh.
        Mesh processMesh(aiMesh* tMesh, const aiScene* tScene) {
            // Data to fill.
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Texture> textures;

            // Walk through each of the mesh's vertices.
            for (unsigned int i = 0; i < tMesh->mNumVertices; i++) {
                Vertex vertex{};
                glm::vec3 vector{};
                // Positions.
                vector.x = tMesh->mVertices[i].x;
                vector.y = tMesh->mVertices[i].y;
                vector.z = tMesh->mVertices[i].z;
                vertex.Position = vector;
                // Normals.
                if(tMesh->HasNormals()) {
                    vector.x = tMesh->mNormals[i].x;
                    vector.y = tMesh->mNormals[i].y;
                    vector.z = tMesh->mNormals[i].z;
                    vertex.Normal = vector;
                }
                // UVs.
                if(tMesh->mTextureCoords[0]) {
                    glm::vec2 vec{};
                    // UVs.
                    vec.x = tMesh->mTextureCoords[0][i].x;
                    vec.y = tMesh->mTextureCoords[0][i].y;
                    vertex.UVs = vec;
                    // Tangent.
                    vector.x = tMesh->mTangents[i].x;
                    vector.y = tMesh->mTangents[i].y;
                    vector.z = tMesh->mTangents[i].z;
                    vertex.Tangent = vector;
                    // Bitangent.
                    vector.x = tMesh->mBitangents[i].x;
                    vector.y = tMesh->mBitangents[i].y;
                    vector.z = tMesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
                else
                    vertex.UVs = glm::vec2(0.0f, 0.0f);

                vertices.push_back(vertex);
            }
            // Retrieve indicies.
            for(unsigned int i = 0; i < tMesh->mNumFaces; i++) {
                aiFace face = tMesh->mFaces[i];
                for(unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
            // Process materials.
            aiMaterial* material = tScene->mMaterials[tMesh->mMaterialIndex];
            //Does the model even have textures?
            if(material->GetTextureCount(aiTextureType_DIFFUSE) == 0
                && material->GetTextureCount(aiTextureType_SPECULAR) == 0) {
                //Diffuse and specular color.
                aiColor4D def(1.0f);
                aiColor4D spec(1.0f);
                aiColor4D emis(1.0f);
                aiColor4D height(1.0f);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &def);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &spec);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emis);
                aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &height);
                //Output.
                glm::vec4 def_v = glm::vec4(def.r, def.g, def.b, def.a);
                glm::vec4 spec_v = glm::vec4(spec.r, spec.g, spec.b, spec.a);
                glm::vec4 emis_v = glm::vec4(emis.r, emis.g, emis.b, emis.a);
                glm::vec4 height_v = glm::vec4(height.r, height.g, height.b, height.a);
                return Mesh(vertices, indices, def_v, spec_v, emis_v, height_v);
            }
            std::vector<Texture> texs;
            // Diffuse maps.
            texs = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse");
            textures.insert(textures.end(), texs.begin(), texs.end());
            texs.clear();
            // Specular maps.
            texs = loadMaterialTextures(material, aiTextureType_SPECULAR, "specular");
            textures.insert(textures.end(), texs.begin(), texs.end());
            texs.clear();
            // Normal maps.
            texs = loadMaterialTextures(material, aiTextureType_HEIGHT, "normal");
            textures.insert(textures.end(), texs.begin(), texs.end());
            texs.clear();
            // Emission maps.
            texs = loadMaterialTextures(material, aiTextureType_EMISSIVE, "emission");
            textures.insert(textures.end(), texs.begin(), texs.end());
            texs.clear();
            // Height maps.
            texs = loadMaterialTextures(material, aiTextureType_AMBIENT, "height");
            textures.insert(textures.end(), texs.begin(), texs.end());
            texs.clear();

            return Mesh(vertices, indices, textures);
        }

        /// Checks all material textures of a given type and loads the textures if they're not loaded yet.
        std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
            std::vector<Texture> textures;
            for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                aiString str;
                mat->GetTexture(type, i, &str);
                // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
                bool skip = false;
                for(unsigned int j = 0; j < texturesLoaded.size(); j++) {
                    if(std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) != 0) continue;
                    // Optimization to disable multiple loads for texture.
                    textures.push_back(texturesLoaded[j]);
                    skip = true;
                    break;
                }
                if(skip) continue;
                // If texture hasn't been loaded already, load it.
                Texture texture;
                texture.id = TextureFromFile(this->directory + "/" + str.C_Str(), gammaCorrection);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                texturesLoaded.push_back(texture);
            }
            return textures;
        }
    };

}
#endif