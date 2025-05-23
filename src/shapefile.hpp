#pragma once

namespace shapefile {

enum class shapefile_t : s32 {
  Null = 0,
  Point = 1,
  PolyLine = 3,
  Polygon = 5,
  MultiPoint = 8,
  PointZ = 11,
  PolyLineZ = 13,
  PolygonZ = 13,
  MultiPointZ = 18,
  PointM = 21,
  PolyLineM = 23,
  PolygonM = 25,
  MultiPointM = 28,
  MultiPatch = 31,
};

class ShapefileReader {
public:
  [[nodiscard]] static s32 toInt32(const char* ptr, std::endian endianness);
  [[nodiscard]] static double toDouble(const char* ptr, std::endian endianness);

  ShapefileReader() = delete;
  ShapefileReader(const fspath& folderName, bool printInfo = false);
  ~ShapefileReader();

  const s32& getLength() const;
  const shapefile_t& getType() const;
  const char* getFirstRecordPtr() const;
  constexpr s32 getMainFileHeaderSizeInBytes() const;
  s32 getRecordsSizeInBytes() const;

private:
  struct BoundingBox {
    double Xmin;
    double Ymin;
    double Xmax;
    double Ymax;
    double Zmin;
    double Zmax;
    double Mmin;
    double Mmax;

    bool empty() const;
    void print() const;
  };

  char* buffer = nullptr;
  s32 fileCode;
  s32 fileLength;
  s32 version;
  shapefile_t type;
  BoundingBox bb;
};

}

