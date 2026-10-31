// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <GLES2/gl2.h>

#include <string>

namespace gazeora {

// An OpenGL ES program linked from a vertex and a fragment shader. Made, used and released on
// the thread whose context is current.
class ShaderProgram {
public:
    // Where the vertex shader's `position` attribute is bound.
    static constexpr GLuint kPositionAttribute = 0;

    ShaderProgram() = default;
    ~ShaderProgram() { Release(); }
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // False with the driver's log in `error` if a shader doesn't compile or they don't link.
    bool Build(const char* vertexSource, const char* fragmentSource, std::string& error);
    void Use() const { glUseProgram(m_program); }
    GLint Uniform(const char* name) const { return glGetUniformLocation(m_program, name); }
    // Any attribute but `position`; -1 if the program has none by that name.
    GLint Attribute(const char* name) const { return glGetAttribLocation(m_program, name); }
    // Before the context goes.
    void Release();

private:
    static GLuint Compile(GLenum type, const char* source, std::string& error);
    bool Link(GLuint vertex, GLuint fragment, std::string& error);

    GLuint m_program = 0;
};

}  // namespace gazeora
