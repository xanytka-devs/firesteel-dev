#include "common.hpp"
#include "shader.hpp"

namespace LearningOpenGL {
    class Framebuffer {
    public:
        Framebuffer() {

        }

        Framebuffer(int tWidth, int tHeight) {
            createBuffers(tWidth, tHeight);
        }
        Framebuffer(glm::vec2 tSize) {
            createBuffers(static_cast<GLuint>(tSize.x), static_cast<GLuint>(tSize.y));
        }

        void scale(int tWidth, int tHeight) const {
            scaleBuffers(tWidth, tHeight);
        }

        void scale(glm::vec2 tSize) const {
            scaleBuffers(static_cast<GLsizei>(tSize.x), static_cast<GLsizei>(tSize.y));
        }

        bool isComplete() const {
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            bool result = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return result;
        }
        void bind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            glEnable(GL_DEPTH_TEST);
        }
        void unbind() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_DEPTH_TEST);
        }

        void quad() {
            float quadVertices[] = {
                // positions       UVs
                -1.0f,  1.0f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,

                -1.0f,  1.0f,  0.0f, 1.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,
                 1.0f,  1.0f,  1.0f, 1.0f
            };
            //Screen quad VAO.
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        }
        void drawQuad(Shader* tShader) const {
            tShader->enable();
            glBindVertexArray(quadVAO);
            glBindTexture(GL_TEXTURE_2D, FBOtexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    private:
        unsigned int quadVAO = 0, quadVBO = 0;
        unsigned int FBO = 0, FBOtexture = 0, RBO = 0;
        unsigned int FBOtextures[11];
        size_t FBOtexturesSize = 1;

        void createBuffers(int tWidth, int tHeight) {
            //FBO creation.
            glGenFramebuffers(1, &FBO);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
            //Framebuffer's texture.
            glGenTextures(1, &FBOtexture);
            glBindTexture(GL_TEXTURE_2D, FBOtexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, tWidth, tHeight, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtexture, 0);
            //Framebuffer's render buffer.
            glGenRenderbuffers(1, &RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tWidth, tHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
        }
        void scaleBuffers(int tWidth, int tHeight) const {
            //Resize FBO.
            glBindTexture(GL_TEXTURE_2D, FBOtexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, tWidth, tHeight, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtexture, 0);
            //Framebuffer's render buffer.
            glBindRenderbuffer(GL_RENDERBUFFER, RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tWidth, tHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
            //Unbind framebuffer.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
    };
}