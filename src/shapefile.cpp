#include "shapefile.hpp"

#include <fstream>
#include <ios>

using namespace shapefile;

constexpr std::endian elittle = std::endian::little;
constexpr std::endian ebig = std::endian::big;
constexpr std::endian enative = std::endian::native;

static const fspath rootFolder = "res/shapefile";

s32 ShapefileReader::toInt32(const char* ptr, std::endian endianness) {
  if constexpr (enative != elittle) endianness = ebig;

  s32 result;
  char bytes[4];

  if (endianness == ebig) {
    bytes[0] = ptr[3];
    bytes[1] = ptr[2];
    bytes[2] = ptr[1];
    bytes[3] = ptr[0];
  } else {
    bytes[0] = ptr[0];
    bytes[1] = ptr[1];
    bytes[2] = ptr[2];
    bytes[3] = ptr[3];
  }

  memcpy(&result, bytes, sizeof(s32));

  return result;
}

double ShapefileReader::toDouble(const char* ptr, std::endian endianness) {
  if constexpr (enative != elittle) endianness = ebig;

  double result;
  char bytes[8];

  if (endianness == ebig) {
    bytes[0] = ptr[7];
    bytes[1] = ptr[6];
    bytes[2] = ptr[5];
    bytes[3] = ptr[4];
    bytes[4] = ptr[3];
    bytes[5] = ptr[2];
    bytes[6] = ptr[1];
    bytes[7] = ptr[0];
  } else {
    bytes[0] = ptr[0];
    bytes[1] = ptr[1];
    bytes[2] = ptr[2];
    bytes[3] = ptr[3];
    bytes[4] = ptr[4];
    bytes[5] = ptr[5];
    bytes[6] = ptr[6];
    bytes[7] = ptr[7];
  }

  memcpy(&result, bytes, sizeof(double));

  return result;
}

ShapefileReader::ShapefileReader(const fspath& folderName, bool printInfo) {
  std::string namePrefix = (rootFolder / folderName.string() / folderName.string()).string();

  std::ifstream infile(namePrefix + ".shp");

  infile.seekg(0, std::ios::end);
  size_t length = infile.tellg();
  infile.seekg(0, std::ios::beg);

  buffer = new char[length];
  infile.read(buffer, length);

  // Byte 0
  fileCode = toInt32(buffer, ebig);
  if (fileCode != 9994)
    error("[ShapefileReader] file code is not 9994");

  // Bytes 4 - 23 Unused

  // Byte 24
  fileLength = toInt32(buffer + 24, ebig);

  // Byte 28
  version = toInt32(buffer + 28, elittle);
  if (version != 1000)
    error("[ShapefileReader] version is not 1000");

  // Byte 32
  type = (shapefile_t)toInt32(buffer + 32, elittle);

  bb.Xmin = toDouble(buffer + 36, elittle); // Byte 36
  bb.Ymin = toDouble(buffer + 44, elittle); // Byte 44
  bb.Xmax = toDouble(buffer + 52, elittle); // Byte 52
  bb.Ymax = toDouble(buffer + 60, elittle); // Byte 60
  bb.Zmin = toDouble(buffer + 68, elittle); // Byte 68
  bb.Zmax = toDouble(buffer + 76, elittle); // Byte 76
  bb.Mmin = toDouble(buffer + 84, elittle); // Byte 84
  bb.Mmax = toDouble(buffer + 92, elittle); // Byte 92

  if (printInfo) {
    puts("\n==================== [ShapeReader] Main file header ====================\n");
    printf("File code: %d\n", fileCode);
    printf("File length: %d\n", fileLength);
    printf("Version: %d\n", version);
    printf("Shape type: %d\n", type);
    bb.print();
    puts("\n========================================================================\n");
  }

  if (bb.empty())
    warning("[ShapefileReader] has no records");
}

ShapefileReader::~ShapefileReader() {
  delete[] buffer;
}

const s32& ShapefileReader::getLength() const {
  return fileLength;
}

const shapefile_t& ShapefileReader::getType() const {
  return type;
}

const char* ShapefileReader::getFirstRecordPtr() const {
  return buffer + getMainFileHeaderSizeInBytes();
}

constexpr s32 ShapefileReader::getMainFileHeaderSizeInBytes() const {
  return 100;
}

s32 ShapefileReader::getRecordsSizeInBytes() const {
  return getLength() * 2 - getMainFileHeaderSizeInBytes();
}

bool ShapefileReader::BoundingBox::empty() const {
  return (
    Xmin == 0.0 && Ymin == 0.0 &&
    Xmax == 0.0 && Ymax == 0.0
  );
}

void ShapefileReader::BoundingBox::print() const {
  puts("BoundingBox:");
  printf("\tXmin: %f\n", Xmin);
  printf("\tYmin: %f\n", Ymin);
  printf("\tXmax: %f\n", Xmax);
  printf("\tYmax: %f\n", Ymax);
  printf("\tZmin: %f\n", Zmin);
  printf("\tZmax: %f\n", Zmax);
  printf("\tMmin: %f\n", Mmin);
  printf("\tMmax: %f\n", Mmax);
}

