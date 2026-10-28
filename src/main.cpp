#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QDir>
#include <QFont>

#include "app/AppController.h"
#include "audio/AudioRecorder.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("KutFast");
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("kutfast");

    // Tipografía moderna predeterminada
    QFont appFont;
    appFont.setFamilies({
        QStringLiteral("Inter"),
        QStringLiteral("Ubuntu"),
        QStringLiteral("Cantarell"),
        QStringLiteral("SF Pro Text"),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Roboto"),
        QStringLiteral("Noto Sans"),
        QStringLiteral("sans-serif")
    });
    appFont.setPointSize(10);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);

    // El icono puede estar bajo distintos prefijos según la versión de Qt;
    // probamos los dos y usamos el primero que exista.
    for (const char *path : {
             ":/qt/qml/kutfast/resources/icons/kutfast.svg",
             ":/kutfast/resources/icons/kutfast.svg"
         }) {
        if (QFile::exists(path)) {
            app.setWindowIcon(QIcon(path));
            break;
        }
    }

    QQuickStyle::setStyle("Material");

    qmlRegisterUncreatableType<AudioRecorder>("kutfast", 1, 0, "AudioRecorder",
                                              "Usa App.recorder");

    AppController controller;

    const QStringList args = app.arguments();
    if (args.size() > 1) {
        QString candidate = args.at(1);
        if (candidate.startsWith("file://")) {
            candidate = QUrl(candidate).toLocalFile();
        }
        if (QFile::exists(candidate)) {
            controller.setInitialAudioFile(QFileInfo(candidate).absoluteFilePath());
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("App", &controller);

    // Qt 6.5+: loadFromModule resuelve la ruta correcta sin importar
    // el prefijo qrc que use la versión instalada de Qt.
    engine.loadFromModule("kutfast", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
