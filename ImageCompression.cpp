#include "ImageCompression.h"
#include <stdexcept>
#include <QFile>
#include <fstream>

namespace ImageCompression {

// === === Клас для роботи з потоком бітів === ===

class BitStream {
public:
    BitStream() : m_buffer(1, 0), m_bitPos(0), m_bytePos(0) {}

    void writeBit(bool bit) {    // Записує один біт у потік
        if (m_bitPos == 8) {
            m_buffer.push_back(0);
            m_bytePos++;
            m_bitPos = 0;
        }
        if (bit) {
            m_buffer[m_bytePos] |= (1 << (7 - m_bitPos));
        }
        m_bitPos++;
    }

    void writeByte(unsigned char byte) {    // Записує байт у потік (8 біт)
        for (int i = 7; i >= 0; --i) {
            writeBit((byte >> i) & 1);
        }
    }

    bool readBit() {    // Зчитує один біт з потоку
        if (m_bytePos >= m_buffer.size()) {
            throw std::out_of_range("Attempted to read past end of bitstream.");
        }
        bool bit = (m_buffer[m_bytePos] >> (7 - m_bitPos)) & 1;
        m_bitPos++;
        if (m_bitPos == 8) {
            m_bitPos = 0;
            m_bytePos++;
        }
        return bit;
    }

    unsigned char readByte() {    // Зчитує байт з потоку (8 біт)
        uint8_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 1) | readBit();
        }
        return value;
    }

    const std::vector<uint8_t>& data() const {    // Отримує внутрішній буфер
        return m_buffer;
    }

    void setBuffer(const std::vector<uint8_t>& buffer) {    // Встановлює буфер та скидає позицію читання
        m_buffer = buffer;
        m_bytePos = 0;
        m_bitPos = 0;
    }

private:
    std::vector<uint8_t> m_buffer;
    int m_bitPos;       // Поточна позиція біта в поточному байті (0-7)
    size_t m_bytePos;   // Поточна позиція байта в буфері
};

// === === Робота з bmp форматом === ===

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
    /*
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
    */

    /* */
    std::ifstream file(path.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Читаємо заголовок BMP (спрощена версія для grayscale)
    uint8_t header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    if (header[0] != 'B' || header[1] != 'M') {
        return false; // Не BMP файл
    }

    // Отримуємо розміри зображення
    outImage.width = *reinterpret_cast<int32_t*>(&header[18]);
    outImage.height = *reinterpret_cast<int32_t*>(&header[22]);

    // Для спрощення припускаємо, що це 8-бітне grayscale зображення
    int dataSize = outImage.width * outImage.height;
    outImage.data.resize(dataSize);

    // Пропускаємо палітру кольорів (для 8-бітного зображення)
    uint32_t offset = *reinterpret_cast<int32_t*>(&header[10]);
    file.seekg(offset, std::ios::beg);

    // Читаємо дані зображення (BMP зберігає рядки знизу вверх)
    for (int y = outImage.height - 1; y >= 0; y--) {
        file.read(reinterpret_cast<char*>(outImage.data.data() + y * outImage.width), outImage.width);
        // Пропускаємо padding
        int padding = (4 - (outImage.width % 4)) % 4;
        file.seekg(padding, std::ios::cur);
    }
    return true;
}

bool saveBmp(const QString& path, const RawImageData& image) {
    /*
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
    */

    /* */
    std::ofstream file(path.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Заголовок BMP файлу для 8-бітного grayscale
    uint8_t header[54] = {0};

    // Файлова сигнатура
    header[0] = 'B';
    header[1] = 'M';

    // Розмір файлу
    int padding = (4 - (image.width % 4)) % 4;
    int imageSize = (image.width + padding) * image.height;
    int fileSize = 54 + 1024 + imageSize; // 54 + палітра + дані

    *reinterpret_cast<uint32_t*>(&header[2]) = fileSize;
    *reinterpret_cast<uint32_t*>(&header[10]) = 54 + 1024; // Зміщення до даних

    // Інформаційний заголовок
    *reinterpret_cast<uint32_t*>(&header[14]) = 40; // Розмір заголовка
    *reinterpret_cast<int32_t*>(&header[18]) = image.width;
    *reinterpret_cast<int32_t*>(&header[22]) = image.height;
    *reinterpret_cast<uint16_t*>(&header[26]) = 1; // Кількість площин
    *reinterpret_cast<uint16_t*>(&header[28]) = 8; // Біт на піксель
    *reinterpret_cast<uint32_t*>(&header[34]) = imageSize;

    file.write(reinterpret_cast<char*>(header), 54);

    // Записуємо grayscale палітру
    for (int i = 0; i < 256; i++) {
        uint8_t color[4] = {static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 0};
        file.write(reinterpret_cast<char*>(color), 4);
    }

    // Записуємо дані зображення (BMP зберігає рядки знизу вверх)
    for (int y = image.height - 1; y >= 0; y--) {
        file.write(reinterpret_cast<const char*>(image.data.data() + y * image.width), image.width);

        // Додаємо padding
        for (int p = 0; p < padding; p++) {
            file.put(0);
        }
    }

    return true;
}

// === === Функції стискання та розтискання === ===

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
    const int rowMaskSize = (rowCount + 7) / 8; // 1 біт на рядок
    std::vector<uint8_t> rowMask(rowMaskSize, 0);

    // Поток кодованих даних
    BitStream payloadBitStream;

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

            if (group[0] == 0xFF && group[1] == 0xFF && group[2] == 0xFF && group[3] == 0xFF) {
                payloadBitStream.writeBit(0); // 0b0
            } else if (group[0] == 0x00 && group[1] == 0x00 && group[2] == 0x00 && group[3] == 0x00) {
                payloadBitStream.writeBit(1); // 0b10
                payloadBitStream.writeBit(0);
            } else {
                payloadBitStream.writeBit(1); // 0b11
                payloadBitStream.writeBit(1);
                for (int k = 0; k < count; ++k)
                    payloadBitStream.writeByte(group[k]);
            }
        }
    }

    result.insert(result.end(), rowMask.begin(), rowMask.end());
    result.insert(result.end(), payloadBitStream.data().begin(), payloadBitStream.data().end());
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

    std::vector<uint8_t> imageData(width * height, 0xFF);

    BitStream dataBitStream;
    dataBitStream.setBuffer(compressedData);
    for (int i = 0; i < 10 + rowMaskSize; ++i) {
        dataBitStream.readByte(); // відступаемо до стисненних даних рядків
    }
    for (int j = 0; j < height; ++j) {
        bool isEmpty = rowMask[j / 8] & (1 << (j % 8));
        if (isEmpty) continue;

        for (int i = 0; i < width;) {
            if (dataBitStream.readBit() == 0) {         // Код 0: чотири білих пікселі
                for (int k = 0; (k < 4) && (i + k < width); ++k)
                    imageData[j * rowSize + i + k] = 0xFF;
                i += 4;
            } else if (dataBitStream.readBit() == 0) {  // Код 10: чотири чорних пікселі
                for (int k = 0; (k < 4) && (i + k < width); ++k)
                    imageData[j * rowSize + i + k] = 0x00;
                i += 4;
            } else {                                    // Код 11: чотири будь-які інші пікселі
                for (int k = 0; (k < 4) && (i + k < width); ++k)
                    imageData[j * rowSize + i + k] = dataBitStream.readByte();
                i += 4;
            }
        }
    }

    return RawImageData{width, height, std::move(imageData)};
}

}

