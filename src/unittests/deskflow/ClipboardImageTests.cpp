/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "ClipboardImageTests.h"
#include "deskflow/Clipboard.h"
#include "deskflow/ClipboardImage.h"
#include <QBuffer>
#include <QImage>
#include <QtEndian>

using Format = IClipboard::Format;

namespace {
QImage fixture(int width, int height)
{
  QImage image(width, height, QImage::Format_ARGB32);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      image.setPixel(x, y, qRgb(x % 256, y % 256, (x + y) % 256));
  return image;
}
std::string pngFor(const QImage &image)
{
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  return bytes.toStdString();
}
QImage imageFor(const std::string &png)
{
  QImage image;
  image.loadFromData(reinterpret_cast<const uchar *>(png.data()), static_cast<qsizetype>(png.size()), "PNG");
  return image;
}
void put32(std::string &data, size_t offset, quint32 value)
{
  qToLittleEndian<quint32>(value, reinterpret_cast<uchar *>(data.data()) + offset);
}
void put16(std::string &data, size_t offset, quint16 value)
{
  qToLittleEndian<quint16>(value, reinterpret_cast<uchar *>(data.data()) + offset);
}
quint32 get32(const std::string &data, size_t offset)
{
  return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data.data()) + offset);
}
std::string header(int width, qint32 height, size_t size)
{
  std::string dib(size, '\0');
  put32(dib, 0, 40);
  put32(dib, 4, static_cast<quint32>(width));
  put32(dib, 8, static_cast<quint32>(height));
  put16(dib, 12, 1);
  put16(dib, 14, 32);
  return dib;
}
void init(Clipboard &clipboard, Format format, const std::string &data, bool text = false)
{
  clipboard.open(0);
  clipboard.empty();
  if (text)
    clipboard.add(Format::Text, "text");
  clipboard.add(format, data);
  clipboard.close();
}
} // namespace

void ClipboardImageTests::dibFromPngWritesCanonicalHeader()
{
  const auto dib = ClipboardImage::dibFromPng(pngFor(fixture(3, 2)));
  QCOMPARE(dib.size(), size_t(64));
  QCOMPARE(get32(dib, 0), quint32(40));
  QCOMPARE(get32(dib, 4), quint32(3));
  QCOMPARE(get32(dib, 8), quint32(2));
  QCOMPARE(qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(dib.data()) + 14), quint16(32));
  QCOMPARE(get32(dib, 16), quint32(0));
}
void ClipboardImageTests::pngRoundTripPreservesPixels()
{
  const auto image = fixture(3, 2);
  const auto dib = ClipboardImage::dibFromPng(pngFor(image));
  QCOMPARE(imageFor(ClipboardImage::pngFromDib(dib)).convertToFormat(QImage::Format_ARGB32), image);
}
void ClipboardImageTests::topDownDibIsUpright()
{
  auto dib = header(1, -2, 48);
  put32(dib, 40, 0x00ff0000);
  put32(dib, 44, 0x000000ff);
  const auto image = imageFor(ClipboardImage::pngFromDib(dib));
  QCOMPARE(image.pixel(0, 0), qRgb(255, 0, 0));
  QCOMPARE(image.pixel(0, 1), qRgb(0, 0, 255));
}
void ClipboardImageTests::bitfieldsDibUsesMaskOffset()
{
  auto dib = header(1, 1, 56);
  put32(dib, 16, 3);
  put32(dib, 40, 0x00ff0000);
  put32(dib, 44, 0x0000ff00);
  put32(dib, 48, 0x000000ff);
  put32(dib, 52, 0x0000ff00);
  const auto image = imageFor(ClipboardImage::pngFromDib(dib));
  QCOMPARE(image.pixel(0, 0), qRgb(0, 255, 0));
}
void ClipboardImageTests::malformedInputReturnsEmpty()
{
  QVERIFY(ClipboardImage::pngFromDib(std::string(10, '\0')).empty());
  QVERIFY(ClipboardImage::dibFromPng("not a png").empty());
}
void ClipboardImageTests::prepareAddsPngAndKeepsBitmapUnderLimit()
{
  Clipboard clipboard;
  init(clipboard, Format::Bitmap, ClipboardImage::dibFromPng(pngFor(fixture(4, 4))), true);
  ClipboardImage::prepareForSending(clipboard, 1024 * 1024);
  clipboard.open(0);
  QVERIFY(clipboard.has(Format::Bitmap));
  QVERIFY(clipboard.has(Format::PNG));
  QCOMPARE(clipboard.get(Format::Text), std::string("text"));
  clipboard.close();
}
void ClipboardImageTests::prepareDropsBitmapOverLimit()
{
  QImage image(200, 200, QImage::Format_ARGB32);
  image.fill(Qt::red);
  Clipboard clipboard;
  init(clipboard, Format::Bitmap, ClipboardImage::dibFromPng(pngFor(image)), true);
  ClipboardImage::prepareForSending(clipboard, 100 * 1024);
  clipboard.open(0);
  QVERIFY(!clipboard.has(Format::Bitmap));
  QVERIFY(clipboard.has(Format::PNG));
  QCOMPARE(clipboard.get(Format::Text), std::string("text"));
  clipboard.close();
}
void ClipboardImageTests::prepareSkipsBitmapFromLargePng()
{
  QImage image(200, 200, QImage::Format_ARGB32);
  image.fill(Qt::red);
  Clipboard clipboard;
  init(clipboard, Format::PNG, pngFor(image));
  ClipboardImage::prepareForSending(clipboard, 100 * 1024);
  clipboard.open(0);
  QVERIFY(!clipboard.has(Format::Bitmap));
  clipboard.close();
  ClipboardImage::prepareForSending(clipboard, 1024 * 1024);
  clipboard.open(0);
  QVERIFY(clipboard.has(Format::Bitmap));
  QCOMPARE(clipboard.get(Format::Bitmap).size(), size_t(40 + 200 * 200 * 4));
  clipboard.close();
}
void ClipboardImageTests::addMissingDerivesBothDirections()
{
  const auto image = fixture(4, 4);
  const auto png = pngFor(image);
  Clipboard first;
  init(first, Format::PNG, png);
  ClipboardImage::addMissingImageFormats(first);
  first.open(0);
  QVERIFY(first.has(Format::Bitmap));
  const auto dib = first.get(Format::Bitmap);
  first.close();
  Clipboard second;
  init(second, Format::Bitmap, dib);
  ClipboardImage::addMissingImageFormats(second);
  second.open(0);
  QVERIFY(second.has(Format::PNG));
  QCOMPARE(imageFor(second.get(Format::PNG)).convertToFormat(QImage::Format_ARGB32), image);
  second.close();
}
QTEST_GUILESS_MAIN(ClipboardImageTests)
