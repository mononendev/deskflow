/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/OSXClipboardPNGConverter.h"

IClipboard::Format OSXClipboardPNGConverter::getFormat() const
{
  return IClipboard::Format::PNG;
}
CFStringRef OSXClipboardPNGConverter::getOSXFormat() const
{
  return CFSTR("public.png");
}
std::string OSXClipboardPNGConverter::fromIClipboard(const std::string &data) const
{
  return data;
}
std::string OSXClipboardPNGConverter::toIClipboard(const std::string &data) const
{
  return data;
}
