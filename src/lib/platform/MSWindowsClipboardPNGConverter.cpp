/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/MSWindowsClipboardPNGConverter.h"
#include <cstring>

MSWindowsClipboardPNGConverter::MSWindowsClipboardPNGConverter() : m_format(RegisterClipboardFormat(L"PNG"))
{
}
IClipboard::Format MSWindowsClipboardPNGConverter::getFormat() const
{
  return IClipboard::Format::PNG;
}
UINT MSWindowsClipboardPNGConverter::getWin32Format() const
{
  return m_format;
}
HANDLE MSWindowsClipboardPNGConverter::fromIClipboard(const std::string &data) const
{
  HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, data.size());
  if (!handle)
    return nullptr;
  void *bytes = GlobalLock(handle);
  if (!bytes) {
    GlobalFree(handle);
    return nullptr;
  }
  std::memcpy(bytes, data.data(), data.size());
  GlobalUnlock(handle);
  return handle;
}
std::string MSWindowsClipboardPNGConverter::toIClipboard(HANDLE data) const
{
  const auto *bytes = static_cast<const char *>(GlobalLock(data));
  if (!bytes)
    return {};
  std::string result(bytes, GlobalSize(data));
  GlobalUnlock(data);
  return result;
}
