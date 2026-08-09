/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 local menu search helper.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3MenuSearch_h
#define FEQT_INCLUDED_SRC_md3_UIMd3MenuSearch_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QString>

/* GUI includes: */
#include "UILibraryDefs.h"

class QMenu;

/** Adds a plain-text-first local search field and anchored regex builder to @a pMenu. */
SHARED_LIBRARY_STUFF void md3PrepareSearchableMenu(QMenu *pMenu,
                                                    const QString &strFieldId,
                                                    const QString &strPlaceholder,
                                                    const QString &strAccessibleName);

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3MenuSearch_h */
