// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "shaders/shader_program.h"

namespace gazeora {

namespace {

constexpr GLsizei kLogSize = 512;

}  // namespace

bool ShaderProgram::Build(const char* vertexSource, const char* fragmentSource,
                          std::string& error) {
    Release();
    GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource, error);
    GLuint fragment = vertex ? Compile(GL_FRAGMENT_SHADER, fragmentSource, error) : 0;
    bool linked = fragment && Link(vertex, fragment, error);
    if (vertex) glDeleteShader(vertex);
    if (fragment) glDeleteShader(fragment);
    return linked;
}

void ShaderProgram::Release() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

GLuint ShaderProgram::Compile(GLenum type, const char* source, std::string& error) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[kLogSize] = {};
        glGetShaderInfoLog(shader, kLogSize, nullptr, log);
        error = std::string(type == GL_VERTEX_SHADER ? "vertex" : "fragment") + " shader: " + log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderProgram::Link(GLuint vertex, GLuint fragment, std::string& error) {
    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glBindAttribLocation(m_program, kPositionAttribute, "position");
    glLinkProgram(m_program);
    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[kLogSize] = {};
        glGetProgramInfoLog(m_program, kLogSize, nullptr, log);
        error = std::string("shader program didn't link: ") + log;
        Release();
        return false;
    }
    return true;
}

}  // namespace gazeora
