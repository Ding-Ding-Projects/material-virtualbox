/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3NotificationCentre class declaration - persistent local notices.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3NotificationCentre_h
#define FEQT_INCLUDED_SRC_md3_UIMd3NotificationCentre_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

/* Forward declarations: */
class QWidget;

/** One local notice. */
struct UIMd3Notice
{
    QString   strId;
    QString   strTitle;
    QString   strDetail;
    QString   strCategory;
    QDateTime when;
    bool      fError;
    bool      fUnread;
};

/** QObject extension owning every local notice.
  *
  * Notices are never lost when a toast times out: the toast is only the
  * transient presentation, while the centre keeps the reviewable record and its
  * own search field. */
class UIMd3NotificationCentre : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners that a notice was posted. */
    void sigNoticePosted(const QString &strId);
    /** Notifies listeners that the notice list changed. */
    void sigChanged();

public:

    /** Returns the centre singleton. */
    static UIMd3NotificationCentre *instance();
    /** Creates the singleton. */
    static void create();
    /** Destroys the singleton. */
    static void destroy();

    /** Posts a notice and shows a toast over @a pParent. */
    QString post(QWidget *pParent, const QString &strTitle, const QString &strDetail,
                 const QString &strCategory = QString(), bool fError = false);
    /** Returns every notice, newest first. */
    QList<UIMd3Notice> notices() const { return m_notices; }
    /** Marks every notice read. */
    void markAllRead();
    /** Clears the reviewable history. */
    void clear();
    /** Returns whether any notice is unread. */
    bool hasUnread() const;
    /** Shows the centre dialog over @a pParent. */
    void showCentre(QWidget *pParent);

private:

    UIMd3NotificationCentre();
    virtual ~UIMd3NotificationCentre() RT_OVERRIDE;

    /** Loads persisted notices from extradata. */
    void load();
    /** Saves notices to extradata. */
    void save() const;

    static UIMd3NotificationCentre *s_pInstance;
    QList<UIMd3Notice>              m_notices;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3NotificationCentre_h */
