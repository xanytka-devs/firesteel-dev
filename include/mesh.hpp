#ifndef MESH_H
#define MESH_H

#include "common.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"

#include <string>
#include <vector>

namespace LearningOpenGL {
#define MAX_BONE_INFLUENCE 4

    struct Vertex {
        // position
        glm::vec3 Position;
        // normal
        glm::vec3 Normal;
        // texCoords
        glm::vec2 UVs;
        // tangent
        glm::vec3 Tangent;
        // bitangent
        glm::vec3 Bitangent;
        // Bone indexes which will influence this vertex.
        int BoneIDs[MAX_BONE_INFLUENCE];
        // Weights from each bone.
        float Weights[MAX_BONE_INFLUENCE];
    };

    struct Texture {
        unsigned int id=0;
        std::string type;
        std::string path;
    };

    class Mesh {
    public:
        /// Mesh Data.
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;
        glm::vec3 ambient{ 0.2f };
        glm::vec3 diffuse{ 1.f };
        glm::vec3 specular{ 0.5f };
        glm::vec3 emission{ 0.f };
        glm::vec3 height{ 0.f };

        /// Constructor with textures.
        Mesh(std::vector<Vertex> tVertices, std::vector<unsigned int> tIndices, std::vector<Texture> tTextures)
            : vertices(tVertices), indices(tIndices), textures(tTextures) {
            mNoTextures = (tTextures.size()==0);
            setupMesh();
        }

        /// Constructor without textures.
        Mesh(std::vector<Vertex> t_vertices, std::vector<unsigned int> t_indices,
            glm::vec3 t_diffuse, glm::vec3 t_specular, glm::vec3 t_emis, glm::vec3 t_height)
            : vertices(t_vertices), indices(t_indices),
            diffuse(t_diffuse), specular(t_specular), emission(t_emis), height(t_height) {
            mNoTextures = true;
            setupMesh();
        }

        /// Render the mesh.
        void draw(Shader& shader) {
            // Bind appropriate textures.
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;
            unsigned int heightNr = 1;
            shader.enable();
            shader.setVec3("material.ambient", ambient);
            shader.setVec3("material.diffuse", diffuse);
            shader.setVec3("material.specular", specular);
            shader.setVec3("material.emission", emission);
            shader.setBool("noTextures", true);
            if(!mNoTextures) {
                shader.setBool("noTextures", false);
                for (unsigned int i = 0; i < textures.size(); i++) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    // Retrieve texture number.
                    std::string number;
                    std::string name = textures[i].type;
                    if (name == "texture_diffuse")
                        number = std::to_string(diffuseNr++);
                    else if (name == "texture_specular")
                        number = std::to_string(specularNr++);
                    else if (name == "texture_normal")
                        number = std::to_string(normalNr++);
                    else if (name == "texture_height")
                        number = std::to_string(heightNr++);
                    // Now set the sampler to the correct texture unit.
                    glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
                    glBindTexture(GL_TEXTURE_2D, textures[i].id);
                }
            }

            // Draw mesh.
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // Always good practice to set everything back to defaults once configured.
            glActiveTexture(GL_TEXTURE0);
        }

    private:
        /// Render data.
        unsigned int VAO, VBO, EBO;
        bool mNoTextures = false;

        /// Initializes all the buffer objects/arrays.
        void setupMesh() {
            // Create buffers/arrays.
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            glBindVertexArray(VAO);
            // Load data into vertex buffers.
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
            // Load data into entity buffers.
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            // Set the vertex attribute pointers.
            // vertex Positions.
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            // vertex Normals.
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
            // vertex UVs.
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, UVs));
            // vertex Tangent.
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
            // vertex Bitangent.
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
            // vertex Bones.
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIDs));
            // vertex Bone weights.
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weights));
            glBindVertexArray(0);
        }
    };
}
#endif