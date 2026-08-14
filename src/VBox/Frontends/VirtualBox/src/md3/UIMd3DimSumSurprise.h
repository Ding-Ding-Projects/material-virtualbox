/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3DimSumSurprise class declaration - a rare, non-blocking startup delight.
 */

/*
 * Copyright (C) 2026 Material Virtual Machine contributors.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3DimSumSurprise_h
#define FEQT_INCLUDED_SRC_md3_UIMd3DimSumSurprise_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QWidget;

/** One bilingual dim sum dish name compiled in for the startup surprise. */
struct UIMd3DimSumDish
{
    UIMd3DimSumDish() {}
    UIMd3DimSumDish(const QString &strEnglish, const QString &strCantonese)
        : strEnglish(strEnglish), strCantonese(strCantonese) {}

    /** English dish name, e.g. "Shrimp dumpling". */
    QString strEnglish;
    /** Playful Hong Kong-style Cantonese (romanized) name, e.g. "Har Gow". */
    QString strCantonese;

    /** Returns the bilingual display form, e.g. "Shrimp dumpling - Har Gow". */
    QString displayName() const;
};

/**
 * A rare, non-blocking, un-opt-out-able "dim sum of the moment" startup delight.
 *
 * On roughly one launch in ten -- and never on a launch that skips the draw
 * (see shouldSkipThisLaunch()) -- a small auto-dismissing toast names one
 * randomly chosen dish, bilingually (English and playful Hong Kong-style
 * Cantonese), shortly after startup has had time to settle. It is scheduled
 * with a plain delayed QTimer::singleShot() so it can never gate startup,
 * never steal focus (the toast is a Qt::Tool window that does not accept
 * focus and is shown with Qt::WA_ShowWithoutActivating), and never delay the
 * application becoming usable. At most one dish is drawn, and at most one
 * toast is ever shown, per process launch.
 *
 * This class ships no setting that disables it -- there is deliberately no
 * API to opt out.
 *
 * This repository intentionally carries no dim-sum photography: catalog
 * images belong to a separate public-catalog project and must never be
 * vendored, generated, or fetched into this repository (this build
 * environment also has no network access regardless). Every dish therefore
 * renders with an explicit, honest "photo not included in this build"
 * placeholder in place of any picture -- see doc/md3/DimSumSurprise.md.
 */
class SHARED_LIBRARY_STUFF UIMd3DimSumSurprise : public QObject
{
    Q_OBJECT;

public:

    /** Returns the process-wide singleton, or 0 before create()/after destroy(). */
    static UIMd3DimSumSurprise *instance();
    /** Creates the singleton; on the ~10% draw, schedules the toast. Idempotent. */
    static void create();
    /** Destroys the singleton and closes any visible toast. */
    static void destroy();

    /** Returns every dish this build can draw from, in a stable order. */
    static QList<UIMd3DimSumDish> dishes();

private slots:

    /** Shows the toast for the dish drawn at construction time, if any was drawn.
      * A no-op if no dish was drawn, or if a toast is already showing. */
    void sltShowToast();

private:

    /** Draws (or does not draw) this launch's dish. */
    UIMd3DimSumSurprise();
    /** Closes any still-visible toast. */
    virtual ~UIMd3DimSumSurprise() RT_OVERRIDE RT_FINAL;

    /**
     * Returns whether this particular process launch must not draw a dish at
     * all: a first run (no prior launch was ever recorded) or the first
     * launch to observe a different build version than last time (an
     * update). Either way, this launch's build version is recorded for next
     * time as a side effect, so the very next ordinary launch is eligible.
     */
    static bool shouldSkipThisLaunch();
    /** Returns the local marker file path recording the last-seen build version. */
    static QString markerFilePath();
    /** Best-effort reduced-motion query; returns false (motion allowed) when the
      * running Qt cannot report a system preference. */
    static bool prefersReducedMotion();

    static UIMd3DimSumSurprise *s_pInstance;
    /** Any currently visible toast; null once dismissed or before one is shown. */
    QPointer<QWidget> m_pToast;
    /** Index into dishes() drawn for this launch, or -1 when none was drawn. */
    int m_iDrawnDishIndex;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3DimSumSurprise_h */
