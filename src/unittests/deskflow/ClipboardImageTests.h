/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include "base/Log.h"
#include <QTest>

class ClipboardImageTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void dibFromPngWritesCanonicalHeader();
  void pngRoundTripPreservesPixels();
  void topDownDibIsUpright();
  void bitfieldsDibUsesMaskOffset();
  void malformedInputReturnsEmpty();
  void prepareAddsPngAndKeepsBitmapUnderLimit();
  void prepareDropsBitmapOverLimit();
  void prepareSkipsBitmapFromLargePng();
  void addMissingDerivesBothDirections();

private:
  Log m_log;
};
