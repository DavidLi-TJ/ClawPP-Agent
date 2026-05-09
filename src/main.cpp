#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QLibraryInfo>
#include <QCoreApplication>
#include <QIcon>

#include "qml/qml_backend.h"

namespace {

QString startupLogPath() {
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dirPath.isEmpty()) {
        dirPath = QDir::tempPath();
    }
    QDir().mkpath(dirPath);
    return QDir(dirPath).filePath(QStringLiteral("startup_trace.log"));
}

QString qmlWarningLogPath() {
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dirPath.isEmpty()) {
        dirPath = QDir::tempPath();
    }
    QDir().mkpath(dirPath);
    return QDir(dirPath).filePath(QStringLiteral("qml_warnings.log"));
}

void appendStartupTrace(const QString& message) {
    QFile file(startupLogPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
        << " | " << message << "\n";
    out.flush();
}

}

int main(int argc, char *argv[]) {
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");
    const QString pluginsPath = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    if (!pluginsPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginsPath);
    }
    QQuickStyle::setStyle("Universal");
    appendStartupTrace(QStringLiteral("Main entry"));

    QGuiApplication app(argc, argv);
    app.setApplicationName("Claw++");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(QStringLiteral(":/app/icon.png")));
    appendStartupTrace(QStringLiteral("QGuiApplication initialized"));

    clawpp::QmlBackend backend;
    appendStartupTrace(QStringLiteral("QmlBackend initialized"));
    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
        [](const QList<QQmlError>& warnings) {
            QFile logFile(qmlWarningLogPath());
            if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                return;
            }

            QTextStream out(&logFile);
            for (const QQmlError& warning : warnings) {
                out << warning.toString() << "\n";
            }
            out.flush();
        });

    engine.rootContext()->setContextProperty("backend", &backend);
    const QUrl mainUrl(QStringLiteral("qrc:/qml/Main.qml"));
    appendStartupTrace(QStringLiteral("Loading QML: qrc:/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [mainUrl](QObject* object, const QUrl& objectUrl) {
            if (!object && objectUrl == mainUrl) {
                appendStartupTrace(QStringLiteral("QML root object creation failed"));
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty()) {
        appendStartupTrace(QStringLiteral("No root objects after engine.load"));
        return -1;
    }

    appendStartupTrace(QStringLiteral("Startup completed"));

    return app.exec();
}
