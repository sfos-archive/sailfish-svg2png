#include <stdio.h>

// before qt to avoid keyword clashes
#include <librsvg/rsvg.h>
#include <glib.h>
#include <cairo.h>

#include <png.h>

#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QDebug>

#define NUM_ICON_CATEGORIES 7

static int iconSourceSizes[NUM_ICON_CATEGORIES] = {24, 32, 48, 64, 96, 128, 86};
static int iconTargetSizes[NUM_ICON_CATEGORIES] = {};
static bool iconTargetSizesSet = false;
static int expectedWidth = -1;
static qreal zoomFactor = 0.0;

static int usage(const char *name)
{
    printf("Usage: %s [-z zoom] [-f grayscale|rgb|rgba] [-s <list of icon category size>] sourceDir targetDir\n", name);
    printf("       Renders SVG files to PNGs\n");
    printf("\n");
    printf("  -z   zoom factor, if not given defaults to 1.0\n");
    printf("  -f   color format, if not given defaults to rgba\n");
    printf("  -w   expected width of the target display in pixels\n");
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

static int even(qreal value)
{
    return 2*qRound(value / 2.0);
}

static QSize getAdjustedSize(QSize size)
{
    if (iconTargetSizesSet) {
        bool sizeSet = false;
        for (int i = 0; i < NUM_ICON_CATEGORIES; i++) {
            if (size.width() == iconSourceSizes[i] && size.height() == iconSourceSizes[i]) {
                size.setWidth(iconTargetSizes[i]);
                size.setHeight(iconTargetSizes[i]);
                sizeSet = true;
                break;
            }
        }
        if (!sizeSet) {
            // Not in any group -> Fallback to zoom factor based scaling
            size = QSize(even(size.width() * zoomFactor), even(size.height() * zoomFactor));
        }
    } else if (expectedWidth > 0 && size.height() == iconSourceSizes[NUM_ICON_CATEGORIES - 1 /* launcher icon */]) {
        int widthRatio = qreal(expectedWidth) / 540;
        size = QSize(even(size.width() * widthRatio), even(size.height() * widthRatio));
    } else {
        size = QSize(even(size.width() * zoomFactor), even(size.height() * zoomFactor));
    }

    return size;
}


static void pngErrorCallback(png_structp png, png_const_charp errorMsg)
{
    Q_UNUSED(png)
    qWarning() << "PNG error" << errorMsg;
}

static void pngWarningCallback(png_structp png, png_const_charp warningMsg)
{
    Q_UNUSED(png)
    qWarning() << "PNG warning" << warningMsg;
}

// N.B. not any kind of grayscale, needs to be different transparency levels of white
static bool surfaceIsGrayscale(cairo_surface_t *cairoSurface)
{
    int height = cairo_image_surface_get_height(cairoSurface);
    int width = cairo_image_surface_get_width(cairoSurface);
    int stride = cairo_image_surface_get_stride(cairoSurface);

    for (int i = 0; i < height; ++i) {
        uint32_t *rowPointer = (uint32_t*) (cairo_image_surface_get_data(cairoSurface) + i * stride);
        for (int j = 0; j < width; ++j) {
            unsigned int pixel = rowPointer[j];
            int alpha = (pixel & 0xff000000) >> 24;
            int red = (pixel & 0xff0000) >> 16;
            int green = (pixel & 0xff00) >> 8;
            int blue = pixel & 0xff;

            if (red != green || red != blue
                || (alpha > 0 && red != alpha)) {
                return false;
            }
        }
    }
    return true;
}

static void transformPixels(png_structp png, png_row_infop rowInfo, png_bytep data)
{
    Q_UNUSED(png)
    // unapply alpha premultiplication and adjust the color channel order ARGB -> RGBA
    for (unsigned int i = 0; i < rowInfo->rowbytes; i += 4) {
        unsigned char *pixelPtr = &data[i];
        uint32_t pixel;
        memcpy(&pixel, pixelPtr, sizeof(uint32_t));
        unsigned char alpha = (pixel & 0xff000000) >> 24;

        if (alpha == 0) {
            pixel = 0;
            memcpy(pixelPtr, &pixel, sizeof(uint32_t));
        } else {
            pixelPtr[0] = ((pixel & 0xff0000) >> 16) * 255 / alpha;
            pixelPtr[1] = ((pixel & 0xff00) >> 8) * 255 / alpha;
            pixelPtr[2] = (pixel & 0xff) * 255 / alpha;
            pixelPtr[3] = alpha;
        }
    }
}

static bool writeCairoToPng(cairo_surface_t *cairoSurface, const char *file)
{
    FILE *output = fopen(file, "wb");
    if (!output) {
        qWarning() << "Failed to create file" << file;
        return false;
    }

    volatile bool success = false;
    png_infop pngInfo = NULL;
    int width = cairo_image_surface_get_width(cairoSurface);
    int height = cairo_image_surface_get_height(cairoSurface);

    png_struct *png = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL,
                                              &pngErrorCallback, &pngWarningCallback);
    if (!png) {
        qWarning() << "SVG2PNG: Failed to allocate PNG struct";
        goto out;
    }

    pngInfo = png_create_info_struct(png);
    if (!pngInfo) {
        qWarning() << "SVG2PNG: Failed to create PNG info";
        goto out;
    }

    if (setjmp(png_jmpbuf(png))) {
        goto out;
    }
    png_init_io(png, output);

    png_set_IHDR(png, pngInfo, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    cairo_surface_flush(cairoSurface);

    if (surfaceIsGrayscale(cairoSurface)) {
        png_text texts[1];
        char grayscale[] = "Grayscale";
        char trueVal[] = "true";
        texts[0].key = grayscale;
        texts[0].text = trueVal;
        texts[0].compression = PNG_TEXT_COMPRESSION_NONE;
        texts[0].itxt_length = 0;
        texts[0].lang = NULL;
        texts[0].lang_key = NULL;
        png_set_text(png, pngInfo, texts, 1);
    }

    {
        png_byte **rows = (png_byte**) malloc(height * sizeof(png_bytep*));
        for (int i = 0; i < height; ++i) {
            rows[i] = cairo_image_surface_get_data(cairoSurface) + i*cairo_image_surface_get_stride(cairoSurface);
        }

        png_set_write_user_transform_fn(png, transformPixels);
        png_set_rows(png, pngInfo, rows);
        png_write_png(png, pngInfo, PNG_TRANSFORM_IDENTITY, NULL);
        free(rows);
    }
    success = true;

out:
    png_destroy_write_struct(&png, &pngInfo);
    fclose(output);
    return success;
}

// assuming rendering is to be done with argb
static bool renderWithCairo(const QFileInfo &fileInfo, const QString &targetDir)
{
    GError *gerror = NULL;
    RsvgHandle *rsvg = rsvg_handle_new_from_file(qPrintable(fileInfo.filePath()), &gerror);

    if (gerror) {
        qWarning() << "SVG2PNG: Error opening SVG file:" << gerror->message;
        g_error_free(gerror);
        return false;
    }

    RsvgDimensionData dimensions;
    rsvg_handle_get_dimensions(rsvg, &dimensions);

    QSize size = QSize(dimensions.width, dimensions.height);
    if (!size.isValid()) {
        qWarning() << "SVG2PNG: Failed to read default size, skipping file" << fileInfo.fileName();
        g_object_unref(rsvg);
        return false;
    }

    size = getAdjustedSize(size);

    cairo_format_t cairoFormat = CAIRO_FORMAT_ARGB32;
    cairo_surface_t *cairoSurface = cairo_image_surface_create(cairoFormat, size.width(), size.height());
    cairo_t *cairoContext = cairo_create(cairoSurface);

    double scaleFactor = static_cast<double>(size.width()) / dimensions.width;
    cairo_scale(cairoContext, scaleFactor, scaleFactor);

    bool success = false;
    if (!rsvg_handle_render_cairo(rsvg, cairoContext)) {
        qWarning() << "SVG2PNG: SVG rendering failed for" << fileInfo.fileName();
    } else {
        QString outFilePath = targetDir + QDir::separator() + fileInfo.completeBaseName() + QLatin1String(".png");
        if (writeCairoToPng(cairoSurface, qPrintable(outFilePath))) {
            qDebug() << "SVG2PNG: Saved" << outFilePath << size;
            success = true;
        }
    }

    cairo_destroy(cairoContext);
    cairo_surface_destroy(cairoSurface);
    g_object_unref(rsvg);

    return success;
}

static bool renderWithQt(const QFileInfo &fileInfo, const QString &targetDir, QImage::Format format)
{
    QSvgRenderer renderer(fileInfo.filePath());

    QSize size = renderer.defaultSize();
    if (!size.isValid()) {
        qWarning() << "SVG2PNG: Failed to read default size, skipping file" << fileInfo.fileName();
        return false;
    }

    size = getAdjustedSize(size);

    QImage out(size, format);
    out.fill(0);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    renderer.render(&painter, out.rect());

    // Pre-calculate grayscale information for the inverted ambiences
    out.setText("Grayscale", out.isGrayscale() ? "true" : "false");

    QString filePath = targetDir + QDir::separator() + fileInfo.completeBaseName() + QLatin1String(".png");
    if (!out.save(filePath)) {
        qWarning() << "SVG2PNG: Failed to save" << filePath;
        return false;
    } else {
        qDebug() << "SVG2PNG: Saved" << filePath << size;
        return true;
    }
}

int main(int argc, char ** argv)
{
    setenv("QT_QPA_PLATFORM", "minimal", 1);

    // Use GUI application just to be sure QImage and QPainter work fine
    QGuiApplication app(argc, argv);

    QString sourceDir;
    QString targetDir;

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
        } else if (argument == QLatin1String("-w")) {
            i++;
            expectedWidth = QString(argv[i]).toInt();
        } else if (argument == QLatin1String("-s")) {
            if (i + NUM_ICON_CATEGORIES >= argc) {
                return usage(argv[0]);
            } else {
                for (int j = 0; j < NUM_ICON_CATEGORIES; j++) {
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
        QFileInfo fileInfo(sourceDir + QDir::separator() + file);

        if (format != QImage::Format_ARGB32_Premultiplied) {
            renderWithQt(fileInfo, targetDir, format);
        } else {
            renderWithCairo(fileInfo, targetDir);
        }
    }
    return 0;
}
