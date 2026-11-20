/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/FileRotation.hpp"

#include <array>
#include <cstdint>
#include <exception>

#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QSaveFile>
#include <QTransform>

#include <KLocalizedString>
#include <exiv2/exiv2.hpp>

namespace
{

// An EXIF orientation (1–8) as what is done to the stored pixels to show them: mirrored left to right first,
// then turned clockwise.
struct Orientation
{
    bool mirrored = false;
    int clockwiseDegrees = 0;
};

constexpr std::array<Orientation, 9> orientations{{
    {},            // 0: not a value; read as 1
    {false, 0},    // 1: as stored
    {true, 0},     // 2: mirrored
    {false, 180},  // 3
    {true, 180},   // 4: mirrored top to bottom
    {true, 270},   // 5
    {false, 90},   // 6
    {true, 90},    // 7
    {false, 270},  // 8
}};

int orientationValue(const Orientation& orientation)
{
    for (int value = 1; value < static_cast<int>(orientations.size()); ++value)
    {
        const Orientation& candidate = orientations.at(value);
        if (candidate.mirrored == orientation.mirrored && candidate.clockwiseDegrees == orientation.clockwiseDegrees)
        {
            return value;
        }
    }

    return 1;
}

void initializeExiv2Once()
{
    static const bool initialized = [] {
        Exiv2::XmpParser::initialize();
        Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);
        return true;
    }();

    Q_UNUSED(initialized)
}

std::expected<void, QString> turnOrientationTag(const QString& filePath, int clockwiseDegrees)
{
    initializeExiv2Once();

    try
    {
        const auto image = Exiv2::ImageFactory::open(QFile::encodeName(filePath).toStdString());
        image->readMetadata();
        Exiv2::ExifData& exif = image->exifData();
        const Exiv2::ExifKey key("Exif.Image.Orientation");
        const auto found = exif.findKey(key);
        const std::int64_t value = found == exif.end() ? 1 : found->toInt64();

        Orientation orientation = value >= 1 && value <= 8 ? orientations.at(static_cast<int>(value)) : Orientation{};
        orientation.clockwiseDegrees = (orientation.clockwiseDegrees + clockwiseDegrees) % 360;
        exif[key.key()] = static_cast<std::uint16_t>(orientationValue(orientation));
        image->writeMetadata();
        return {};
    }
    catch (const std::exception& error)
    {
        return std::unexpected(QString::fromLocal8Bit(error.what()));
    }
}

// Read as stored, turned and written again in the same format; replaced only once completely written.
std::expected<void, QString> turnPixels(const QString& filePath, int clockwiseDegrees)
{
    QImageReader reader(filePath);
    reader.setAutoTransform(false);
    const QByteArray format = reader.format();
    const QImage image = reader.read();
    if (image.isNull())
    {
        return std::unexpected(reader.errorString());
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        return std::unexpected(file.errorString());
    }

    QImageWriter writer(&file, format);
    if (!writer.write(image.transformed(QTransform().rotate(clockwiseDegrees))))
    {
        file.cancelWriting();
        return std::unexpected(writer.errorString());
    }

    if (!file.commit())
    {
        return std::unexpected(file.errorString());
    }

    return {};
}

}  // namespace

namespace pictorio::image_actions
{

RotationSaving rotationSavingFor(const QString& mimeType)
{
    if (mimeType == QLatin1String("image/jpeg") || mimeType == QLatin1String("image/tiff"))
    {
        return RotationSaving::OrientationTag;
    }

    if (mimeType == QLatin1String("image/png") || mimeType == QLatin1String("image/bmp"))
    {
        return RotationSaving::Pixels;
    }

    return RotationSaving::NotPossible;
}

std::expected<void, QString> saveRotation(const QString& filePath, const QString& mimeType, int clockwiseDegrees)
{
    if (clockwiseDegrees != 90 && clockwiseDegrees != 180 && clockwiseDegrees != 270)
    {
        return std::unexpected(i18nc("@info", "Only quarter turns can be saved."));
    }

    switch (rotationSavingFor(mimeType))
    {
    case RotationSaving::OrientationTag:
        return turnOrientationTag(filePath, clockwiseDegrees);
    case RotationSaving::Pixels:
        return turnPixels(filePath, clockwiseDegrees);
    case RotationSaving::NotPossible:
        break;
    }

    return std::unexpected(
        i18nc("@info", "The rotation of this type of image cannot be saved without losing quality."));
}

}  // namespace pictorio::image_actions
