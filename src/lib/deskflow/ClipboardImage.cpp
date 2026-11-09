/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "deskflow/ClipboardImage.h"

#include "base/Log.h"
#include "deskflow/Clipboard.h"

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QImageReader>
#include <QtEndian>
#include <limits>

namespace {
using Format = IClipboard::Format;
constexpr uint32_t kBiBitfields = 3;
constexpr uint32_t kBiAlphabitfields = 6;
constexpr uint32_t kInfoHeaderSize = 40;

bool hasImageData(const Clipboard &clipboard, Format format)
{
  return clipboard.has(format) && !clipboard.get(format).empty();
}

size_t marshalledSize(const Clipboard &clipboard)
{
  size_t size = 4;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    auto format = static_cast<Format>(i);
    if (clipboard.has(format)) {
      size += 8 + clipboard.get(format).size();
    }
  }
  return size;
}

size_t estimatedDibSize(const std::string &png)
{
  QByteArray bytes = QByteArray::fromRawData(png.data(), static_cast<qsizetype>(png.size()));
  QBuffer buffer(&bytes);
  QImageReader reader(&buffer, "PNG");
  const QSize size = reader.size();
  if (!size.isValid()) {
    return 0;
  }
  const auto width = static_cast<size_t>(size.width());
  const auto height = static_cast<size_t>(size.height());
  if (width > (std::numeric_limits<size_t>::max() - kInfoHeaderSize) / 4 / height) {
    return 0;
  }
  return kInfoHeaderSize + width * height * 4;
}
} // namespace

uint32_t ClipboardImage::dibPixelOffset(const uint8_t *dib, size_t dibSize)
{
  uint32_t pixelOffset = 0;
  if (dibSize >= 16) {
    const auto biSize = qFromLittleEndian<quint32>(dib);
    if (biSize >= 12 && biSize <= dibSize) {
      pixelOffset = biSize;
      if (biSize >= kInfoHeaderSize) {
        const auto biBitCount = qFromLittleEndian<quint16>(dib + 14);
        const auto biCompression = qFromLittleEndian<quint32>(dib + 16);
        if (biSize == kInfoHeaderSize) {
          if (biCompression == kBiBitfields) {
            pixelOffset += 12;
          } else if (biCompression == kBiAlphabitfields) {
            pixelOffset += 16;
          }
        }
        if (biBitCount > 0 && biBitCount <= 8) {
          const auto biClrUsed = qFromLittleEndian<quint32>(dib + 32);
          const auto numColors = biClrUsed != 0 ? biClrUsed : (1u << biBitCount);
          pixelOffset += numColors * 4;
        }
      }
    }
  }
  return pixelOffset;
}

std::string ClipboardImage::pngFromDib(const std::string &dib)
{
  const auto offset = dibPixelOffset(reinterpret_cast<const uint8_t *>(dib.data()), dib.size());
  if (offset == 0 || dib.size() > std::numeric_limits<quint32>::max() - 14) {
    return {};
  }
  QByteArray bmp(14, '\0');
  bmp[0] = 'B';
  bmp[1] = 'M';
  qToLittleEndian<quint32>(static_cast<quint32>(14 + dib.size()), bmp.data() + 2);
  qToLittleEndian<quint32>(14 + offset, bmp.data() + 10);
  bmp.append(dib.data(), static_cast<qsizetype>(dib.size()));
  QImage image;
  if (!image.loadFromData(bmp, "BMP")) {
    LOG_WARN("failed to decode clipboard bitmap");
    return {};
  }
  QByteArray png;
  QBuffer buffer(&png);
  if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
    LOG_WARN("failed to encode clipboard image as png");
    return {};
  }
  return std::string(png.constData(), static_cast<size_t>(png.size()));
}

std::string ClipboardImage::dibFromPng(const std::string &png)
{
  QImage image;
  if (!image.loadFromData(reinterpret_cast<const uchar *>(png.data()), static_cast<qsizetype>(png.size()), "PNG")) {
    LOG_WARN("failed to decode clipboard png");
    return {};
  }
  image = image.convertToFormat(QImage::Format_ARGB32);
  const auto width = static_cast<size_t>(image.width());
  const auto height = static_cast<size_t>(image.height());
  if (image.isNull() || width > (std::numeric_limits<quint32>::max() - kInfoHeaderSize) / 4 / height) {
    return {};
  }
  const auto imageSize = static_cast<quint32>(width * height * 4);
  std::string dib(kInfoHeaderSize + imageSize, '\0');
  auto *data = reinterpret_cast<uchar *>(dib.data());
  qToLittleEndian<quint32>(kInfoHeaderSize, data);
  qToLittleEndian<quint32>(static_cast<quint32>(width), data + 4);
  qToLittleEndian<quint32>(static_cast<quint32>(height), data + 8);
  qToLittleEndian<quint16>(1, data + 12);
  qToLittleEndian<quint16>(32, data + 14);
  qToLittleEndian<quint32>(imageSize, data + 20);
  qToLittleEndian<quint32>(2835, data + 24);
  qToLittleEndian<quint32>(2835, data + 28);
  auto *out = data + kInfoHeaderSize;
  for (int y = image.height() - 1; y >= 0; --y) {
    const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      qToLittleEndian<quint32>(row[x], out);
      out += 4;
    }
  }
  return dib;
}

void ClipboardImage::addMissingImageFormats(Clipboard &clipboard)
{
  clipboard.open(0);
  if (hasImageData(clipboard, Format::Bitmap) && !hasImageData(clipboard, Format::PNG)) {
    const auto png = pngFromDib(clipboard.get(Format::Bitmap));
    if (!png.empty())
      clipboard.add(Format::PNG, png);
  } else if (hasImageData(clipboard, Format::PNG) && !hasImageData(clipboard, Format::Bitmap)) {
    const auto dib = dibFromPng(clipboard.get(Format::PNG));
    if (!dib.empty())
      clipboard.add(Format::Bitmap, dib);
  }
  clipboard.close();
}

void ClipboardImage::prepareForSending(Clipboard &clipboard, size_t maximumBytes)
{
  clipboard.open(0);
  if (hasImageData(clipboard, Format::Bitmap) && !hasImageData(clipboard, Format::PNG)) {
    const auto png = pngFromDib(clipboard.get(Format::Bitmap));
    if (!png.empty())
      clipboard.add(Format::PNG, png);
  } else if (hasImageData(clipboard, Format::PNG) && !hasImageData(clipboard, Format::Bitmap)) {
    const auto png = clipboard.get(Format::PNG);
    const auto estimate = estimatedDibSize(png);
    const auto size = marshalledSize(clipboard);
    if (estimate != 0 && size < maximumBytes && maximumBytes - size > 8 && estimate < maximumBytes - size - 8) {
      const auto dib = dibFromPng(png);
      if (!dib.empty())
        clipboard.add(Format::Bitmap, dib);
    }
  }
  if (hasImageData(clipboard, Format::Bitmap) && hasImageData(clipboard, Format::PNG) &&
      marshalledSize(clipboard) >= maximumBytes) {
    const auto pngSize = clipboard.get(Format::PNG).size();
    clipboard.remove(Format::Bitmap);
    LOG_DEBUG("removed bitmap from clipboard to fit limit, png size: %zu bytes", pngSize);
  }
  clipboard.close();
}
