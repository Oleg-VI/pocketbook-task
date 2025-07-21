#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QDebug>
#include "FileModel.h"

/*
 * УВАГА !!! УВАГА !!! УВАГА !!! УВАГА !!! УВАГА !!!
 * Цей код не є оптимальним, як і структура проекту.
 * Це зроблено щоб код не був повністю комерційним,
 * так як був зроблений безоплатно, як завдання,
 * а не як комерційна оплачена розробка.
 * Oleg Virnyi c *
 * УВАГА !!! УВАГА !!! УВАГА !!! УВАГА !!! УВАГА !!!
 */

int main(int argc, char *argv[])
{
    // Встановлюємо стиль, який підтримує кастомізацію, через змінну середовища ПЕРЕД створенням QGuiApplication
    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion"); // "Basic" або "Material", "Fusion"

    QGuiApplication app(argc, argv);

    // Налаштування додатку
    app.setApplicationName("Image Compression Tool");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Oleg Virnyi C");

    // Парсинг командного рядка
    QString workingDirectory;
    if (argc > 1 && QDir(argv[1]).exists()) {
        workingDirectory = argv[1];
    } else {
        workingDirectory = QDir::currentPath();
    }
    qDebug() << "Working Directory:" << workingDirectory;

    // Реєструємо типи для QML
    qmlRegisterType<FileModel>("PocketBookTaskNoCommercialUse03", 1, 0, "FileModel");

    // Створюємо QML движок
    QQmlApplicationEngine engine;

    // Встановлюємо властивість робоча директорія для QML
    engine.rootContext()->setContextProperty("workingDirectory", workingDirectory);

    // Завантажуємо головний QML файл
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("PocketBookTaskNoCommercialUse03", "Main");

    return app.exec();
}

