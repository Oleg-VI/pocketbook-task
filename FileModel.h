#ifndef FILEMODEL_H
#define FILEMODEL_H

#include <QAbstractListModel>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

struct FileItem {
    QString name;
    QString path;
    QString extension;
    qint64 size;
    bool isProcessing;
    QString processingStatus;

    FileItem() : size(0), isProcessing(false) {}
};

class FileModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)

public:
    enum FileRoles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        ExtensionRole,
        SizeRole,
        IsProcessingRole,
        ProcessingStatusRole
    };

    explicit FileModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString directory() const;
    void setDirectory(const QString& path);

    Q_INVOKABLE void processFile(int index);
    Q_INVOKABLE void refreshDirectory();

signals:
    void directoryChanged();
    void errorOccurred(const QString& message);

private slots:
    void onFileProcessed(const QString& filePath, bool success, const QString& message);

private:
    void loadFiles();
    void setFileProcessing(const QString& filePath, bool processing, const QString& status = "");

    QString m_directory;
    QList<FileItem> m_files;
    QMutex m_mutex;
};

class FileProcessor : public QThread {
    Q_OBJECT

public:
    FileProcessor(const QString& filePath, QObject* parent = nullptr);

protected:
    void run() override;

signals:
    void finished(const QString& filePath, bool success, const QString& message);

private:
    QString m_filePath;

    bool processBmpFile(const QString& inputPath, const QString& outputPath);
    bool processBarchFile(const QString& inputPath, const QString& outputPath);
};

#endif // FILEMODEL_H
