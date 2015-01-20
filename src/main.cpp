#include <stdio.h>

#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <qmath.h>


int usage(const char *name)
{
    printf("Usage: %s [-z zoom] sourceDir targetDir\n", name);
    printf("       Renders SVG files to PNGs\n");
    printf("\n");
    printf("  -z   zoom factor, if not given defaults to 1.0\n");
    return -1;
}

int main(int argc, char ** argv)
{
    setenv("QT_QPA_PLATFORM", "minimal", 1);

    // Use GUI application just to be sure QImage and QPainter work fine
    QGuiApplication app(argc, argv);

    qreal zoomFactor = 0.0;
    QString sourceDir;
    QString targetDir;

    int i = 1;
    while (i < argc) {
        if (QLatin1String(argv[i]) == QLatin1String("-z")) {
            i++;
            if (i < argc) {
                zoomFactor = QString(argv[i]).toFloat();
            }
            if (zoomFactor <= 0.0) {
                return usage(argv[0]);
            }
        } else if (sourceDir.isEmpty()) {
            sourceDir = QLatin1String(argv[i]);
        } else if (targetDir.isEmpty()) {
            targetDir = QLatin1String(argv[i]);
        } else {
            return usage(argv[0]);
        }
        i++;
    }

    if (sourceDir.isEmpty() || targetDir.isEmpty()) {
        return usage(argv[0]);
    }

    QStringList fileList = QDir(sourceDir, "*.svg").entryList();

    if (!fileList.count()) {
        qWarning() << "SVG2PNG: No SVG files found in" << sourceDir;
        return 0; // Not an error
    }

    QDir target(targetDir);
    if (!target.mkpath(".")) {
        qWarning() << "SVG2PNG: Could not create target directory" << targetDir;
        return -1;
    }

    if (zoomFactor <= 0.0) {
        qWarning() << "SVG2PNG: No zoom factor given, defaulting to 1.0";
        zoomFactor = 1.0;
    }

    foreach (QString file, fileList) {
        QSvgRenderer renderer(sourceDir + QDir::separator() + file);

        QSize size = renderer.defaultSize();
        if (!size.isValid()) {
            qWarning() << "SVG2PNG: Failed to read default size, skipping file" << file;
            continue;
        }
        // Floor target dimension to even number
        size.setWidth(2 * qFloor(size.width() * zoomFactor * 0.5));
        size.setHeight(2 * qFloor(size.height() * zoomFactor * 0.5));

        QImage out(size, QImage::Format_ARGB32_Premultiplied);
        out.fill(0);
        QPainter painter(&out);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        renderer.render(&painter, out.rect());

        QString filePath = targetDir + QDir::separator() + file;
        filePath.replace("svg", "png");
        if (!out.save(filePath)) {
            qWarning() << "SVG2PNG: Failed to save" << filePath;
        } else {
            qDebug() << "SVG2PNG: Saved" << filePath << size;
        }
    }
    return 0;
}
