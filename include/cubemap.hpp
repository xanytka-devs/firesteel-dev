#include "common.hpp"
#include "shader.hpp"

#include "utils/utils.hpp"

namespace LearningOpenGL {
    class Cubemap {
    public:
        Cubemap()
            : m_has_textures(false), m_id(0), m_skybox_vao(0), m_skybox_vbo(0) {}

        void load_m(const char* t_dir,
            const char* t_right = "right.png",
            const char* t_left = "left.png",
            const char* t_top = "top.png",
            const char* t_bottom = "bottom.png",
            const char* t_front = "front.png",
            const char* t_back = "back.png") {
            //Setup.
            m_dir = t_dir;
            m_has_textures = true;
            m_faces = { t_front, t_back, t_top, t_bottom, t_right, t_left };
            glActiveTexture(GL_TEXTURE11);
            glGenTextures(1, &m_id);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
            //Load faces.
            int w, h, channels;
            for (unsigned int i = 0; i < 6; i++) {
                unsigned char* data = stbi_load((m_dir + "/" + m_faces[i]).c_str(),
                    &w, &h, &channels, 0);
                //Get color mode.
                GLenum color_mode = GL_RED;
                switch (channels) {
                case 3:
                    color_mode = GL_RGB;
                    break;
                case 4:
                    color_mode = GL_RGBA;
                    break;
                }
                //Bind data.
                if (data)
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0, color_mode, w, h, 0, color_mode, GL_UNSIGNED_BYTE, data);
                else
                    LOG_ERRR(std::string("Failed to load texture at \"") + (const char*)m_faces[i] + "\".");
                //Free data.
                stbi_image_free(data);
            }
            //Communicate texture properties to OpenGL.
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glActiveTexture(GL_TEXTURE0);
        }

        void bind() const {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
        }

        unsigned int getID() const { return m_id; }

        //void load(const char* t_cb_file_path) {
        //    clear();
        //    std::vector<std::string> files = split_str(&read_from_file(t_cb_file_path), '\n');
        //    if (files.size() != 7) return;
        //    //Readress function.
        //    load_m(files[0].c_str(), files[1].c_str(), files[2].c_str(),
        //        files[3].c_str(), files[4].c_str(), files[5].c_str(), files[6].c_str());
        //}

        void initialize(float t_size) {
            // set up vertices
            float skybox_vert[] = {
                // positions          
                -1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size, -1.0f * t_size,

                -1.0f * t_size, -1.0f * t_size,  1.0f * t_size,
                -1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                -1.0f * t_size, -1.0f * t_size,  1.0f * t_size,

                 1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size, -1.0f * t_size,

                -1.0f * t_size, -1.0f * t_size,  1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size,  1.0f * t_size,
                -1.0f * t_size, -1.0f * t_size,  1.0f * t_size,

                -1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size,  1.0f * t_size,
                -1.0f * t_size,  1.0f * t_size, -1.0f * t_size,

                -1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size, -1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size, -1.0f * t_size,
                -1.0f * t_size, -1.0f * t_size,  1.0f * t_size,
                 1.0f * t_size, -1.0f * t_size,  1.0f * t_size
            };
            //Create buffers and arrays.
            glGenVertexArrays(1, &m_skybox_vao);
            glGenBuffers(1, &m_skybox_vbo);
            glBindVertexArray(m_skybox_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_skybox_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vert), &skybox_vert, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        }

        void draw(Shader* t_shader) const {
            glDepthFunc(GL_LEQUAL);
            t_shader->enable();
            //Skybox cube.
            glBindVertexArray(m_skybox_vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS);
        }

        void clear() {
            glDeleteTextures(1, &m_id);
            m_faces.clear();
        }

        void remove() {
            glDeleteVertexArrays(1, &m_skybox_vao);
            glDeleteBuffers(1, &m_skybox_vbo);
            clear();
        }

        ~Cubemap() { remove(); }
    private:
        unsigned int m_skybox_vao, m_skybox_vbo;
        unsigned int m_id;
        std::string m_dir;
        std::vector<const char*> m_faces;
        bool m_has_textures;
    };
}
