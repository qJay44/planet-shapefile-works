#pragma once

#include <vector>

class MiniMesh {
public:
  static MiniMesh loadShapefile(const fspath& folder);

  MiniMesh(const std::vector<vec2>& vertices, const std::vector<GLuint>& indices, const GLenum& mode);

  void draw() const;

private:
  const std::vector<vec2> vertices;
  const std::vector<GLuint> indices;
  const GLenum mode;

  GLuint vao;
  GLuint vbo;
  GLuint ebo;
};

