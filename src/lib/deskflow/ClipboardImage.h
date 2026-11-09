/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class Clipboard;

//! Converts clipboard images between the Bitmap (DIB) and PNG formats
class ClipboardImage
{
public:
  //! Byte offset of the pixel array from the start of \p dib, or 0 when the header is malformed
  static uint32_t dibPixelOffset(const uint8_t *dib, size_t dibSize);
  //! Encode a Bitmap-format DIB as PNG; returns empty on failure
  static std::string pngFromDib(const std::string &dib);
  //! Decode PNG to a bottom-up 32bpp BI_RGB DIB with a 40 byte header; returns empty on failure
  static std::string dibFromPng(const std::string &png);
  //! Receiver side: add Bitmap from PNG or PNG from Bitmap when only one is present
  static void addMissingImageFormats(Clipboard &clipboard);
  //! Add missing image format, then remove Bitmap if the marshalled clipboard reaches the limit
  static void prepareForSending(Clipboard &clipboard, size_t maximumBytes);
};
