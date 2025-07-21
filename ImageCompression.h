#ifndef IMAGECOMPRESSION_H
#define IMAGECOMPRESSION_H

#include <vector>
#include <QString>

namespace ImageCompression {

struct RawImageData {
    int width;
    int height;
    std::vector<uint8_t> data;
};

bool loadBmp(const QString& path, RawImageData& outImage);
bool saveBmp(const QString& path, const RawImageData& image);

std::vector<uint8_t> compress(const RawImageData &image);
RawImageData decompress(const std::vector<uint8_t> &compressedData);

}

#endif // IMAGECOMPRESSION_H
