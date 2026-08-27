#pragma once

#include <GL/glew.h>

#include <memory>
#include <stdexcept>
#include <string>

#include "GlError.h"

namespace OIV
{
    class GLGpuProgram
    {
      public:

        GLGpuProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
        {
            try
            {
                fVertexShader   = CreateShader(vertexShaderSource, GL_VERTEX_SHADER);
                fFragmentShader = CreateShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
                fShaderProgram  = glCreateProgram();
                if (fShaderProgram == 0)
                    throw std::runtime_error("Could not create an OpenGL shader program");

                glAttachShader(fShaderProgram, fVertexShader);
                glAttachShader(fShaderProgram, fFragmentShader);
                glBindFragDataLocation(fShaderProgram, 0, "outColor");
                glLinkProgram(fShaderProgram);

                GLint linked{};
                glGetProgramiv(fShaderProgram, GL_LINK_STATUS, &linked);
                if (linked != GL_TRUE)
                    throw std::runtime_error("Could not link OpenGL shaders: " + GetProgramLinkError(fShaderProgram));
            }
            catch (...)
            {
                Release();
                throw;
            }
        }

        GLGpuProgram(const GLGpuProgram&)            = delete;
        GLGpuProgram& operator=(const GLGpuProgram&) = delete;

        ~GLGpuProgram() { Release(); }

        void Bind() const { glUseProgram(fShaderProgram); }

        void SetUniform1I(const char* name, int value) const
        {
            const GLint location = glGetUniformLocation(fShaderProgram, name);
            if (location >= 0)
                glUniform1i(location, value);
        }

        void SetUniform1F(const char* name, float value) const
        {
            const GLint location = glGetUniformLocation(fShaderProgram, name);
            if (location >= 0)
                glUniform1f(location, value);
        }

        void SetUniform2F(const char* name, float first, float second) const
        {
            const GLint location = glGetUniformLocation(fShaderProgram, name);
            if (location >= 0)
                glUniform2f(location, first, second);
        }

        void SetUniform4F(const char* name, const float* values) const
        {
            const GLint location = glGetUniformLocation(fShaderProgram, name);
            if (location >= 0)
                glUniform4fv(location, 1, values);
        }

        GLuint GetProgram() const { return fShaderProgram; }

      private:

        void Release()
        {
            if (fShaderProgram != 0)
                glDeleteProgram(fShaderProgram);
            if (fFragmentShader != 0)
                glDeleteShader(fFragmentShader);
            if (fVertexShader != 0)
                glDeleteShader(fVertexShader);

            fShaderProgram  = 0;
            fFragmentShader = 0;
            fVertexShader   = 0;
        }

        static GLuint CreateShader(const char* shaderSource, GLenum type)
        {
            if (shaderSource == nullptr || shaderSource[0] == '\0')
                throw std::invalid_argument("OpenGL shader source is empty");

            const GLuint shader = glCreateShader(type);
            if (shader == 0)
                throw std::runtime_error("Could not create an OpenGL shader");

            glShaderSource(shader, 1, &shaderSource, nullptr);
            glCompileShader(shader);
            if (!IsShaderCompiled(shader))
            {
                const std::string error = GetShaderCompileError(shader);
                glDeleteShader(shader);
                throw std::runtime_error("Could not compile an OpenGL shader: " + error);
            }

            return shader;
        }

        GLuint fFragmentShader{};
        GLuint fVertexShader{};
        GLuint fShaderProgram{};
    };

    using GLGpuProgramUniquePtr = std::unique_ptr<GLGpuProgram>;
}  // namespace OIV
