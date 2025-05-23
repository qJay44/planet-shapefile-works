#include "MiniMesh.hpp"

#include "shapefile.hpp"

MiniMesh MiniMesh::loadShapefile(const fspath &folder) {
  constexpr std::endian elittle = std::endian::little;
  constexpr std::endian ebig = std::endian::big;

  using namespace shapefile;
  ShapefileReader shp(folder);
  shapefile_t type = shp.getType();

  std::vector<vec2> vertices;
  std::vector<GLuint> indices;
  GLuint idx = 0;

  switch (type) {
    case shapefile_t::Polygon: {
      s32 recordsSize = shp.getRecordsSizeInBytes();
      s32 recordOffset = 0;

      while (recordOffset < recordsSize) {
        const char* recordHeader = shp.getFirstRecordPtr() + recordOffset;

        s32 recordNumber = shp.toInt32(recordHeader + 0, ebig);
        s32 contentLength = shp.toInt32(recordHeader + 4, ebig);

        const char* recordContent = recordHeader + 8;
        shapefile_t rType = (shapefile_t)shp.toInt32(recordContent, elittle);

        if (type != rType)
          error("[Mesh] Shapefile record type differs from the type in the main file header");

        // double Xmin = shp.toDouble(recordContent + 4, elittle);
        // double Ymin = shp.toDouble(recordContent + 12, elittle);
        // double Xmax = shp.toDouble(recordContent + 20, elittle);
        // double Ymax = shp.toDouble(recordContent + 28, elittle);
        s32 numParts = shp.toInt32(recordContent + 36, elittle);
        s32 numPoints = shp.toInt32(recordContent + 40, elittle);
        s32 parts[numParts];
        parts[0] = 0;

        const char* pointsPtr = recordContent + 44 + numParts * 4;

        for (u32 i = 0; i < numParts; i++) {
          s32& currPart = parts[i];

          u32 currPartPoints;
          if (i + 1 < numParts) {
            s32& nextPart = parts[i + 1];
            nextPart = shp.toInt32(recordContent + 44 + (i + 1) * 4, elittle);
            currPartPoints = nextPart - currPart;
          } else {
            currPartPoints = numPoints - currPart;
          }

          for (u32 j = currPart; j < currPart + currPartPoints; j++) {
            double x = shp.toDouble(pointsPtr + 0 + 16 * j , elittle);
            double y = shp.toDouble(pointsPtr + 8 + 16 * j , elittle);

            vertices.push_back({x, y});
            indices.push_back(idx);
            if (j - currPart && j + 1 < currPart + currPartPoints)
              indices.push_back(idx);

            idx++;
          }
        }

        recordOffset += 8 + contentLength * 2;
      }

      break;
    }
    default:
      error("[Mesh] Unhandled shape type");
  }

  return MiniMesh(vertices, indices, GL_LINES);
}

MiniMesh::MiniMesh(const std::vector<vec2>& vertices, const std::vector<GLuint>& indices, const GLenum& mode)
  : vertices(vertices),
    indices(indices),
    mode(mode)
{
  // VAO
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // VBO
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

  // EBO
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

  // Attribs
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(0));

  // Unbind
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MiniMesh::draw() const {
  glBindVertexArray(vao);

  if (indices.size())
    glDrawElements(mode, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
  else
    glDrawArrays(mode, 0, (GLsizei)vertices.size());

  glBindVertexArray(0);
}

