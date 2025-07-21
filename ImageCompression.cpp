#include "ImageCompression.h"
#include <stdexcept>
#include <QFile>

namespace ImageCompression {

#pragma pack(push, 1)
struct BmpFileHeader {
    quint16 bfType;
    quint32 bfSize;
    quint16 bfReserved1;
    quint16 bfReserved2;
    quint32 bfOffBits;
};

struct BmpInfoHeader {
    quint32 biSize;
    qint32  biWidth;
    qint32  biHeight;
    quint16 biPlanes;
    quint16 biBitCount;
    quint32 biCompression;
    quint32 biSizeImage;
    qint32  biXPelsPerMeter;
    qint32  biYPelsPerMeter;
    quint32 biClrUsed;
    quint32 biClrImportant;
};
#pragma pack(pop)

bool loadBmp(const QString& path, RawImageData& outImage) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    BmpFileHeader fileHeader;
    BmpInfoHeader infoHeader;

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.bfType != 0x4D42 || infoHeader.biBitCount != 8 || infoHeader.biCompression != 0)
        return false;

    int width = infoHeader.biWidth;
    int height = std::abs(infoHeader.biHeight);
    int stride = ((width + 3) / 4) * 4; // доповнення до 4 байтів

    file.seek(fileHeader.bfOffBits);

    outImage.width = width;
    outImage.height = height;
    outImage.data.resize(width * height);

    for (int y = height - 1; y >= 0; --y) {
        file.read(reinterpret_cast<char*>(outImage.data.data() + y * width), width);
        if (stride > width)
            file.read(nullptr, stride - width); // пропустити відступ
    }
    return true;
}

bool saveBmp(const QString& path, const RawImageData& image) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    int stride = ((image.width + 3) / 4) * 4;
    int padding = stride - image.width;
    int imageSize = stride * image.height;

    BmpFileHeader fileHeader = {0x4D42, static_cast<quint32>(54 + imageSize), 0, 0, 54};
    BmpInfoHeader infoHeader = {
        40,
        image.width,
        image.height,
        1,
        8,
        0,
        static_cast<quint32>(imageSize),
        2835, 2835,
        256, 256
    };

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    // записуємо палітру сірих тонів
    for (int i = 0; i < 256; ++i) {
        char entry[4] = { static_cast<char>(i), static_cast<char>(i), static_cast<char>(i), 0 };
        file.write(entry, 4);
    }

    // запис пікселів знизу вгору
    char pad[3] = {0};
    for (int y = image.height - 1; y >= 0; --y) {
        file.write(reinterpret_cast<const char*>(image.data.data() + y * image.width), image.width);
        file.write(pad, padding);
    }

    return true;
}

std::vector<uint8_t> compress(const RawImageData &image) {
    std::vector<uint8_t> result;

    // Заголовок: 'B' 'A'
    result.push_back('B');
    result.push_back('A');

    // Ширина та висота (по 4 байти кожна, little-endian)
    for (int i = 0; i < 4; ++i) {
        result.push_back((image.width >> (8 * i)) & 0xFF);
        result.push_back((image.height >> (8 * i)) & 0xFF);
    }

    const int rowCount = image.height;
    const int rowSize = image.width;

    // Маска рядка
    const int rowMaskSize = (rowCount + 7) / 8; // 1 біт на рідок
    std::vector<uint8_t> rowMask(rowMaskSize, 0);

    std::vector<uint8_t> payload;

    for (int j = 0; j < rowCount; ++j) {
        bool isEmpty = true;
        for (int i = 0; i < rowSize; ++i) {
            if (image.data[j * rowSize + i] != 0xFF) {
                isEmpty = false;
                break;
            }
        }
        if (isEmpty) {
            rowMask[j / 8] |= (1 << (j % 8));
            continue;
        }

        // Обробка рядка фрагментами по 4 пікселі
        for (int i = 0; i < rowSize; i += 4) {
            int count = std::min(4, rowSize - i);
            uint8_t group[4] = {0xFF, 0xFF, 0xFF, 0xFF};
            for (int k = 0; k < count; ++k) {
                group[k] = image.data[j * rowSize + i + k];
            }

            // !!! Тут біти (0b0, 0b10, 0b11) можа було б стиснути, не виділяючи окремий байт під кожну комбінацію,
            // !!! та залишимо такий оптимізований варіант для платної розробки =)
            if (group[0] == 0xFF && group[1] == 0xFF && group[2] == 0xFF && group[3] == 0xFF) {
                payload.push_back(0b00000000); // 0b0
            } else if (group[0] == 0x00 && group[1] == 0x00 && group[2] == 0x00 && group[3] == 0x00) {
                payload.push_back(0b00000010); // 0b10
            } else {
                payload.push_back(0b00000011); // 0b11
                for (int k = 0; k < count; ++k)
                    payload.push_back(group[k]);
            }
        }
    }

    result.insert(result.end(), rowMask.begin(), rowMask.end());
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
}

RawImageData decompress(const std::vector<uint8_t> &compressedData) {
    if (compressedData.size() < 10 || compressedData[0] != 'B' || compressedData[1] != 'A')
        throw std::runtime_error("Invalid format");

    int width = 0;
    int height = 0;
    for (int i = 0; i < 4; ++i) {
        width |= compressedData[2 + i * 2] << (8 * i);
        height |= compressedData[3 + i * 2] << (8 * i);
    }

    const int rowSize = width;
    const int rowMaskSize = (height + 7) / 8;
    const uint8_t* rowMask = compressedData.data() + 10;
    const uint8_t* data = rowMask + rowMaskSize;

    std::vector<uint8_t> imageData(width * height, 0xFF);

    size_t dataIndex = 0;
    for (int j = 0; j < height; ++j) {
        bool isEmpty = rowMask[j / 8] & (1 << (j % 8));
        if (isEmpty) continue;

        for (int i = 0; i < width;) {
            if (dataIndex >= compressedData.size() - 10 - rowMaskSize)
                throw std::runtime_error("Unexpected end of data");

            uint8_t code = data[dataIndex++];
            if ((code & 0b11) == 0b00) {
                for (int k = 0; k < 4 && i + k < width; ++k)
                    imageData[j * rowSize + i + k] = 0xFF;
                i += 4;
            } else if ((code & 0b11) == 0b10) {
                for (int k = 0; k < 4 && i + k < width; ++k)
                    imageData[j * rowSize + i + k] = 0x00;
                i += 4;
            } else if ((code & 0b11) == 0b11) {
                for (int k = 0; k < 4 && i + k < width; ++k)
                    imageData[j * rowSize + i + k] = data[dataIndex++];
                i += 4;
            } else {
                throw std::runtime_error("Unknown code block");
            }
        }
    }

    return RawImageData{width, height, std::move(imageData)};
}

}

