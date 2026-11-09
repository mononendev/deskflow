/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/OSXClipboardTIFFConverter.h"
#include "base/Log.h"
#include <ImageIO/ImageIO.h>

namespace {
std::string transcode(const std::string &data, CFStringRef type)
{
  CFDataRef input = CFDataCreateWithBytesNoCopy(
      kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(data.data()), static_cast<CFIndex>(data.size()),
      kCFAllocatorNull
  );
  CGImageSourceRef source = input ? CGImageSourceCreateWithData(input, nullptr) : nullptr;
  CGImageRef image = source ? CGImageSourceCreateImageAtIndex(source, 0, nullptr) : nullptr;
  CFMutableDataRef output = image ? CFDataCreateMutable(kCFAllocatorDefault, 0) : nullptr;
  CGImageDestinationRef destination = output ? CGImageDestinationCreateWithData(output, type, 1, nullptr) : nullptr;
  bool success = false;
  if (destination) {
    CGImageDestinationAddImage(destination, image, nullptr);
    success = CGImageDestinationFinalize(destination);
  }
  std::string result;
  if (success) {
    result.assign(
        reinterpret_cast<const char *>(CFDataGetBytePtr(output)), static_cast<size_t>(CFDataGetLength(output))
    );
  } else {
    LOG_WARN("failed to convert clipboard image with ImageIO");
  }
  if (destination)
    CFRelease(destination);
  if (output)
    CFRelease(output);
  if (image)
    CGImageRelease(image);
  if (source)
    CFRelease(source);
  if (input)
    CFRelease(input);
  return result;
}
} // namespace

IClipboard::Format OSXClipboardTIFFConverter::getFormat() const
{
  return IClipboard::Format::PNG;
}
CFStringRef OSXClipboardTIFFConverter::getOSXFormat() const
{
  return CFSTR("public.tiff");
}
std::string OSXClipboardTIFFConverter::fromIClipboard(const std::string &data) const
{
  return transcode(data, CFSTR("public.tiff"));
}
std::string OSXClipboardTIFFConverter::toIClipboard(const std::string &data) const
{
  return transcode(data, CFSTR("public.png"));
}
