#include "FileModel.h"
#include "ImageCompression.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

FileModel::FileModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_directory(QDir::currentPath())
{
    loadFiles();
}

int FileModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_files.size();
}

QVariant FileModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_files.size()) {
        return QVariant();
    }

    const FileItem& item = m_files[index.row()];

    switch (role) {
    case NameRole:
        return item.name;
    case PathRole:
        return item.path;
    case ExtensionRole:
        return item.extension;
    case SizeRole:
        return item.size;
    case IsProcessingRole:
        return item.isProcessing;
    case ProcessingStatusRole:
        return item.processingStatus;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FileModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[PathRole] = "path";
    roles[ExtensionRole] = "extension";
    roles[SizeRole] = "size";
    roles[IsProcessingRole] = "isProcessing";
    roles[ProcessingStatusRole] = "processingStatus";
    return roles;
}

QString FileModel::directory() const {
    return m_directory;
}

void FileModel::setDirectory(const QString& path) {
    if (m_directory != path) {
        m_directory = path;
        loadFiles();
        emit directoryChanged();
    }
}

void FileModel::loadFiles() {
    beginResetModel();
    m_files.clear();

    QDir dir(m_directory);
    if (!dir.exists()) {
        endResetModel();
        return;
    }

    QStringList filters;
    filters << "*.bmp" << "*.png" << "*.barch" << "*.txt"; // *.txt тут додано для перевірки ErrorDialog.qml

    QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo& fileInfo : fileInfoList) {
        FileItem item;
        item.name = fileInfo.fileName();
        item.path = fileInfo.absoluteFilePath();
        item.extension = fileInfo.suffix().toLower();
        item.size = fileInfo.size();
        item.isProcessing = false;

        m_files.append(item);
    }

    endResetModel();
}

void FileModel::processFile(int index) {
    if (index < 0 || index >= m_files.size()) {
        emit errorOccurred("Невідомий файл");
        return;
    }

    const FileItem& item = m_files[index];

    if (item.extension != "bmp" && item.extension != "barch") {
        emit errorOccurred("Невідомий файл");
        return;
    }

    if (item.isProcessing) {
        return; // Файл вже обробляється
    }

    QString status = (item.extension == "bmp") ? "Кодується" : "Розкодовується";
    setFileProcessing(item.path, true, status);

    FileProcessor* processor = new FileProcessor(item.path, this);
    connect(processor, &FileProcessor::finished, this, &FileModel::onFileProcessed);
    connect(processor, &FileProcessor::finished, processor, &FileProcessor::deleteLater);
    processor->start();
}

void FileModel::refreshDirectory() {
    loadFiles();
}

void FileModel::onFileProcessed(const QString& filePath, bool success, const QString& message) {
    setFileProcessing(filePath, false);

    if (!success) {
        emit errorOccurred(message);
    }

    // Оновлюємо список файлів
    loadFiles();
}

void FileModel::setFileProcessing(const QString& filePath, bool processing, const QString& status) {
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_files.size(); ++i) {
        if (m_files[i].path == filePath) {
            m_files[i].isProcessing = processing;
            m_files[i].processingStatus = status;

            QModelIndex idx = index(i, 0);
            emit dataChanged(idx, idx, {IsProcessingRole, ProcessingStatusRole});
            break;
        }
    }
}

// FileProcessor implementation
FileProcessor::FileProcessor(const QString& filePath, QObject* parent)
    : QThread(parent), m_filePath(filePath)
{
}

void FileProcessor::run() {
    QThread::msleep(500); // !!! !!! емуляція тривалої обробки для можливості помітити зміну стану обробки файлу !!! !!!

    QFileInfo fileInfo(m_filePath);
    QString extension = fileInfo.suffix().toLower();

    bool success = false;
    QString message;

    if (extension == "bmp") {
        QString outputPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + "packed.barch";
        success = processBmpFile(m_filePath, outputPath);
        message = success ? "Файл успішно закодовано" : "Помилка кодування файлу";
    } else if (extension == "barch") {
        QString outputPath = fileInfo.absolutePath() + "/" + fileInfo.baseName() + "unpacked.bmp";
        success = processBarchFile(m_filePath, outputPath);
        message = success ? "Файл успішно розкодовано" : "Помилка розкодування файлу";
    }

    emit finished(m_filePath, success, message);
}

bool FileProcessor::processBmpFile(const QString& inputPath, const QString& outputPath) {
    try {
        // Завантажуємо BMP файл
        ImageCompression::RawImageData img;
        if (!ImageCompression::loadBmp(inputPath, img)) {
            return false;
        }

        // Кодуємо зображення
        std::vector<uint8_t> result = ImageCompression::compress(img);

        // Зберігаємо у файл
        QFile outFile(outputPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(reinterpret_cast<const char*>(result.data()), result.size());
            outFile.close();
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool FileProcessor::processBarchFile(const QString& inputPath, const QString& outputPath) {
    try {
        // Завантажуємо .barch файл
        QFile inFile(inputPath);
        if (!inFile.open(QIODevice::ReadOnly)) return false;
        QByteArray data = inFile.readAll();

        // Декодуємо зображення
        ImageCompression::RawImageData img = ImageCompression::decompress(std::vector<uint8_t>(data.begin(), data.end()));

        // Зберігаємо як BMP
        ImageCompression::saveBmp(outputPath, img);

        return true;
    } catch (...) {
        return false;
    }
}

