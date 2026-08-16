/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3DimSum class declaration - the 10%-per-launch dim sum startup surprise.
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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3DimSum_h
#define FEQT_INCLUDED_SRC_md3_UIMd3DimSum_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

/* IPRT compatibility macros: */
#include <iprt/cdefs.h>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QWidget;

/**
 * One bundled dim sum dish: a stable id plus a bilingual display name.
 *
 * The field shape mirrors the bilingual `name.en` / `name.zhHant` pattern of
 * the public dim-sum photo catalogue this house contract otherwise points
 * every such surface at.  This feature does not fetch that catalogue over
 * the network -- see doc/md3/DimSum.md for exactly why -- so every dish here
 * is a small, honestly-labelled offline subset compiled into the binary
 * rather than a live sync of the public source.
 */
struct UIMd3DimSumDish
{
    /** Stable dish identifier. */
    QString strId;
    /** English dish name (catalogue `name.en`). */
    QString strNameEnglish;
    /** Traditional-Chinese dish name (catalogue `name.zhHant`). */
    QString strNameCantonese;
};

/**
 * Non-blocking, un-optable, 10%-per-launch dim sum startup surprise.
 *
 * A process-wide singleton, created once from main() alongside the other
 * Material 3 services.  It rolls one fresh 10% chance the moment it is
 * constructed -- so the draw genuinely happens once per process launch, not
 * once per call -- then, when a top-level surface invites it to, shows one
 * small corner-anchored, auto-dismissing toast naming a randomly chosen
 * bundled dish, in the active language mode, styled by the active per-
 * language funny-level sliders.
 *
 * It never blocks the caller, never steals keyboard focus, never appears
 * during the very first launch of a fresh profile, never appears while a
 * modal dialog owns the screen, and carries no setting that can turn it off.
 */
class SHARED_LIBRARY_STUFF UIMd3DimSum : public QObject
{
    Q_OBJECT;

signals:

    /**
     * Announces that a dish surprise was actually shown, naming its id.
     * Nothing in this codebase currently listens; kept for the same reason
     * every sibling Material 3 service publishes a change signal, so a later
     * surface (history, notifications) can observe it without this class
     * needing to know that surface exists.
     */
    void sigDishShown(const QString &strDishId);

public:

    /** Returns the process-wide singleton, or 0 before create(). */
    static UIMd3DimSum *instance();
    /** Creates the singleton.  Must run after UIMd3Language::create(). */
    static void create();
    /** Destroys the singleton. */
    static void destroy();

    /** Returns the bundled dish catalogue, in declaration order. */
    static QList<UIMd3DimSumDish> catalogue();
    /** Returns the display name for @a dish in the active language mode. */
    static QString dishDisplayName(const UIMd3DimSumDish &dish);

    /**
     * Invites the service to show its startup surprise, anchored to the
     * screen that @a pParent currently lives on.
     *
     * Safe to call more than once, and from more than one top-level window;
     * only the very first invitation received by the process can ever have
     * an effect.  Even that one only actually shows a toast when every one
     * of these holds:
     *
     *   - this is not the first launch of a fresh profile (there is nothing
     *     else in this codebase to hook a "first run" signal off, so this
     *     service tracks that itself, in its own tiny persisted state file);
     *   - the one-shot 10% roll taken at construction time won;
     *   - no modal dialog is in front of the screen at the moment the
     *     (short, deferred) attempt actually runs, so the surprise never
     *     fights an error path, a medium-enumeration warning, or any other
     *     blocking flow for the user's attention.
     *
     * Never blocks: this only arms a short deferred timer and returns
     * immediately, and the resulting toast never activates or steals focus.
     */
    void maybeShowAtStartup(QWidget *pParent);

    /** Returns whether this process's one-shot roll won.  Exposed only so a
      * local, non-shipping harness can assert the roll happened exactly
      * once per launch; production code has no reason to call this. */
    bool wonThisLaunch() const { return m_fWon; }
    /** Returns whether this is the first launch of a fresh profile. */
    bool isFirstLaunchEver() const { return m_fFirstLaunchEver; }

private:

    /** Constructs the service and takes its one-shot roll. */
    UIMd3DimSum();
    /** Destructs the service. */
    virtual ~UIMd3DimSum() RT_OVERRIDE RT_FINAL;

    /** Registers this service's own chrome strings with UIMd3Language. */
    void registerText();
    /** Retries @a pParent after @a iAttemptsLeft more short deferrals when a
      * modal dialog currently owns the screen; shows the surprise otherwise. */
    void attemptShow(QPointer<QWidget> pParent, int iAttemptsLeft);
    /** Builds and shows the actual corner-anchored toast for @a dish. */
    void showToast(QWidget *pParent, const UIMd3DimSumDish &dish);

    /** Returns the small persisted-state file path, or an empty string when
      * the platform has no writable application-data location. */
    static QString storagePath();
    /** Loads the one persisted flag; returns false when nothing valid has
      * ever been saved -- i.e. on a genuinely fresh profile. */
    static bool loadHasLaunchedBefore();
    /** Persists that a launch has now happened, best-effort. */
    static void saveHasLaunchedBefore();

    static UIMd3DimSum *s_pInstance;
    /** Whether some top-level surface has already invited this service to
      * show its surprise this launch; guards against firing twice. */
    bool m_fAlreadyInvited;
    /** Whether this is the first launch of a fresh profile. */
    bool m_fFirstLaunchEver;
    /** Whether this launch's one-shot 10% draw won. */
    bool m_fWon;
};

/** Convenience accessor mirroring uiCommon(). */
inline UIMd3DimSum *md3DimSum() { return UIMd3DimSum::instance(); }

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3DimSum_h */
