#include <stdio.h>

#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QGuiApplication>
#include <QDir>
#include <QFile>

#include <QDebug>

#define NUM_ICON_CATETORIES 7

int usage(const char *name)
{
    printf("Usage: %s [-z zoom] [-f grayscale|rgb|rgba] [-s <list of icon category size>] sourceDir targetDir\n", name);
    printf("       Renders SVG files to PNGs\n");
    printf("\n");
    printf("  -z   zoom factor, if not given defaults to 1.0\n");
    printf("  -f   color format, if not given defaults to rgba\n");
    printf("  -s   icon category sizes in the following order:\n");
    printf("       - extra small\n");
    printf("       - small\n");
    printf("       - small plus\n");
    printf("       - medium\n");
    printf("       - large\n");
    printf("       - extra large\n");
    printf("       - launcher\n");

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

    int iconSourceSizes[NUM_ICON_CATETORIES] = {24, 32, 48, 64, 96, 128, 86};
    int iconTargetSizes[NUM_ICON_CATETORIES] = {};
    bool iconTargetSizesSet = false;
    QImage::Format format = QImage::Format_ARGB32_Premultiplied;

    int i = 1;
    while (i < argc) {
        const QLatin1String argument(argv[i]);

        if (argument == QLatin1String("-z")) {
            i++;
            if (i < argc) {
                zoomFactor = QString(argv[i]).toFloat();
            }
            if (zoomFactor <= 0.0) {
                return usage(argv[0]);
            }
        } else if (argument == QLatin1String("-s")) {
            if (i + NUM_ICON_CATETORIES >= argc) {
                return usage(argv[0]);
            } else {
                for (int j = 0; j < NUM_ICON_CATETORIES; j++) {
                    i++;
                    iconTargetSizes[j] = QString(argv[i]).toInt();
                    if (iconTargetSizes[j] <= 0) {
                        return usage(argv[0]);
                    }
                }
                iconTargetSizesSet = true;
            }
        } else if (argument == "-f") {
            ++i;

            const QLatin1String argument = i < argc ? QLatin1String(argv[i]) : QLatin1String();
            if (argument == "grayscale") {
                format = QImage::Format_Grayscale8;
            } else if (argument == "rgb") {
                format = QImage::Format_RGB32;
            } else if (argument == "rgba") {
                // This is the default.
            } else {
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

        if (iconTargetSizesSet) {
            bool sizeSet = false;
            for (int i = 0; i < NUM_ICON_CATETORIES; i++) {
                if (size.width() == iconSourceSizes[i] && size.height() == iconSourceSizes[i]) {
                    size.setWidth(iconTargetSizes[i]);
                    size.setHeight(iconTargetSizes[i]);
                    sizeSet = true;
                    break;
                }
            }
            if (!sizeSet) {
                // Not in any group -> Fallback to zoom factor based scaling
                size *= zoomFactor;
            }
        } else {
            size *= zoomFactor;
        }

        QImage out(size, format);
        out.fill(0);

        QPainter painter(&out);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        renderer.render(&painter, out.rect());

        // Pre-calculate grayscale information for the inverted ambiences
        out.setText("Grayscale", out.isGrayscale() ? "true" : "false");

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
