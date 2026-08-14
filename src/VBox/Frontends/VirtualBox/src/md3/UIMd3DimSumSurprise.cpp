/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3DimSumSurprise class implementation - a rare, non-blocking startup delight.
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

/* Qt includes: */
#include <QAccessible>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QLabel>
#include <QPalette>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

/* GUI includes: */
#include "UIMd3DimSumSurprise.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

/* Other VBox includes: */
#include <iprt/buildconfig.h>

namespace
{
    QString md3DimSumText(const char *pszKey, const QString &strFallback)
    {
        UIMd3Language *pLanguage = UIMd3Language::instance();
        return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
    }
}

QString UIMd3DimSumDish::displayName() const
{
    if (strEnglish.isEmpty())
        return strCantonese;
    if (strCantonese.isEmpty())
        return strEnglish;
    /* House format: "English - Cantonese", e.g. "Shrimp dumpling - Har Gow". */
    return QString("%1 - %2").arg(strEnglish, strCantonese);
}


UIMd3DimSumSurprise *UIMd3DimSumSurprise::s_pInstance = 0;

/* static */
UIMd3DimSumSurprise *UIMd3DimSumSurprise::instance()
{
    return s_pInstance;
}

/* static */
void UIMd3DimSumSurprise::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3DimSumSurprise;
    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.dimsum.kicker"),
                                                 QStringLiteral("Dim sum of the moment"),
                                                 QStringLiteral("呢刻嘅點心"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.dimsum.photoPlaceholder"),
                                                 QStringLiteral("Photo not included\nin this build"),
                                                 QStringLiteral("呢個版本\n未有相"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.dimsum.accessibleName"),
                                                 QStringLiteral("Dim sum surprise"),
                                                 QStringLiteral("點心驚喜"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.dimsum.accessibleDescription"),
                                                 QStringLiteral("Dim sum of the moment: %1. Photo not included in this build."),
                                                 QStringLiteral("呢刻嘅點心：%1。呢個版本未有相。"));
    }
}

/* static */
void UIMd3DimSumSurprise::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

/* static */
QList<UIMd3DimSumDish> UIMd3DimSumSurprise::dishes()
{
    /* A small, stable, compiled-in bilingual dish list. No photography is
     * bundled -- see the class comment and doc/md3/DimSumSurprise.md for why. */
    QList<UIMd3DimSumDish> list;
    list << UIMd3DimSumDish("Shrimp dumpling", "Har Gow");
    list << UIMd3DimSumDish("Pork and shrimp dumpling", "Siu Mai");
    list << UIMd3DimSumDish("Barbecue pork bun", "Char Siu Bao");
    list << UIMd3DimSumDish("Rice noodle roll", "Cheung Fun");
    list << UIMd3DimSumDish("Turnip cake", "Law Bok Gou");
    list << UIMd3DimSumDish("Egg tart", "Daan Taat");
    list << UIMd3DimSumDish("Sticky rice in lotus leaf", "Lo Mai Gai");
    list << UIMd3DimSumDish("Spring roll", "Chun Guen");
    list << UIMd3DimSumDish("Phoenix claws", "Fung Zaau");
    list << UIMd3DimSumDish("Custard bun", "Nai Wong Bao");
    list << UIMd3DimSumDish("Pan-fried dumpling", "Woh Tip");
    list << UIMd3DimSumDish("Steamed spare ribs", "Pai Guat");
    return list;
}

/* static */
QString UIMd3DimSumSurprise::markerFilePath()
{
    const QString strLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strLocation.isEmpty())
        return QString();
    return QDir(strLocation).filePath(QStringLiteral("md3-dimsum-surprise-lastseen.txt"));
}

/* static */
bool UIMd3DimSumSurprise::shouldSkipThisLaunch()
{
    const QString strPath = markerFilePath();
    /* No writable location at all: play it safe and never draw. */
    if (strPath.isEmpty())
        return true;

    const QString strCurrentVersion = QString::fromUtf8(RTBldCfgVersion()).trimmed();

    QString strLastSeenVersion;
    QFile readFile(strPath);
    const bool fMarkerExisted = readFile.exists();
    if (fMarkerExisted && readFile.open(QIODevice::ReadOnly))
    {
        strLastSeenVersion = QString::fromUtf8(readFile.readAll()).trimmed();
        readFile.close();
    }

    /* Best-effort: record this launch's version for next time, regardless of
     * this launch's own outcome, so the very next ordinary launch is eligible. */
    const QFileInfo fileInfo(strPath);
    if (QDir().mkpath(fileInfo.absolutePath()))
    {
        QSaveFile writeFile(strPath);
        if (writeFile.open(QIODevice::WriteOnly))
        {
            writeFile.write(strCurrentVersion.toUtf8());
            writeFile.commit();
        }
    }

    /* First run ever (no marker could be read), or the first launch to see a
     * different build version than last time (an update): both stay quiet. */
    if (!fMarkerExisted || strLastSeenVersion.isEmpty())
        return true;
    if (strLastSeenVersion != strCurrentVersion)
        return true;

    return false;
}

/* static */
bool UIMd3DimSumSurprise::prefersReducedMotion()
{
    /* Qt exposes no reduced-motion accessor on QStyleHints at the version this
     * project builds against (6.8.3), so there is no reliable cross-platform
     * signal to read here and this returns false.
     *
     * This function previously called QStyleHints::reduceMotion() behind a
     * QT_VERSION_CHECK(6, 6, 0) guard. That guard was the defect rather than the
     * protection it looked like: the build Qt is NEWER than 6.6, so the guard
     * admitted the call, and the method still does not exist -- C2039, after a
     * fifty-one minute compile. A version guard only protects against a version
     * that is too OLD; it says nothing about whether the API was ever added.
     * Verify the symbol exists in the exact Qt being built against before
     * reintroducing it, and do not simply raise the number in the guard.
     *
     * When a real signal is available, this is the single place to add it: every
     * animation in this class is already routed through the caller of this
     * function. */
    return false;
}

UIMd3DimSumSurprise::UIMd3DimSumSurprise()
    : m_iDrawnDishIndex(-1)
{
    if (!shouldSkipThisLaunch())
    {
        const QList<UIMd3DimSumDish> list = dishes();
        if (!list.isEmpty())
        {
            /* One specific face of a ten-sided virtual die: a flat 10% chance. */
            if (QRandomGenerator::global()->bounded(10) == 0)
                m_iDrawnDishIndex = QRandomGenerator::global()->bounded(list.size());
        }
    }

    if (m_iDrawnDishIndex >= 0)
    {
        /* A generous fixed delay so the real application window has every
         * chance to be up and settled first. This can never gate startup:
         * the timer is scheduled here but only ever fires once the
         * application event loop is already running the rest of the app. */
        QTimer::singleShot(4000, this, &UIMd3DimSumSurprise::sltShowToast);
    }
}

UIMd3DimSumSurprise::~UIMd3DimSumSurprise()
{
    if (m_pToast)
        m_pToast->close();
}

void UIMd3DimSumSurprise::sltShowToast()
{
    /* Never twice in one launch, and never once the draw has been consumed: */
    if (m_iDrawnDishIndex < 0 || m_pToast)
        return;

    const QList<UIMd3DimSumDish> list = dishes();
    if (m_iDrawnDishIndex >= list.size())
    {
        m_iDrawnDishIndex = -1;
        return;
    }
    const UIMd3DimSumDish dish = list.at(m_iDrawnDishIndex);
    m_iDrawnDishIndex = -1;

    QFrame *pToast = new QFrame(0, Qt::Tool | Qt::FramelessWindowHint
                                    | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    pToast->setAttribute(Qt::WA_ShowWithoutActivating, true);
    pToast->setAttribute(Qt::WA_DeleteOnClose, true);
    pToast->setFocusPolicy(Qt::NoFocus);
    pToast->setObjectName(QStringLiteral("md3DimSumToast"));
    pToast->setFixedWidth(260);

    const QColor background = md3(UIMd3ColorRole_SurfaceContainerHigh);
    const QColor outline = md3(UIMd3ColorRole_OutlineVariant);
    const QColor onSurface = md3(UIMd3ColorRole_OnSurface);
    const QColor onSurfaceVariant = md3(UIMd3ColorRole_OnSurfaceVariant);
    pToast->setStyleSheet(QString("QFrame#md3DimSumToast { background-color: %1; "
                                   "border: 1px solid %2; border-radius: %3px; }")
                               .arg(background.name(), outline.name())
                               .arg(UIMd3Shape::Medium));

    QVBoxLayout *pLayout = new QVBoxLayout(pToast);
    const int iGutter = md3Theme().gutter();
    pLayout->setContentsMargins(iGutter, iGutter, iGutter, iGutter);
    pLayout->setSpacing(iGutter / 2);

    QLabel *pKicker = new QLabel(md3DimSumText("md3.dimsum.kicker", tr("Dim sum of the moment")), pToast);
    pKicker->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
    QPalette kickerPalette = pKicker->palette();
    kickerPalette.setColor(QPalette::WindowText, onSurfaceVariant);
    pKicker->setPalette(kickerPalette);
    pKicker->setWordWrap(true);

    QLabel *pPhoto = new QLabel(md3DimSumText("md3.dimsum.photoPlaceholder", tr("Photo not included\nin this build")), pToast);
    pPhoto->setObjectName(QStringLiteral("md3DimSumPhoto"));
    pPhoto->setAlignment(Qt::AlignCenter);
    pPhoto->setWordWrap(true);
    pPhoto->setFixedSize(96, 96);
    pPhoto->setFont(md3Theme().font(UIMd3TypeRole_BodySmall));
    pPhoto->setStyleSheet(QString("QLabel#md3DimSumPhoto { color: %1; "
                                   "border: 1px dashed %2; border-radius: %3px; }")
                               .arg(onSurfaceVariant.name(), outline.name())
                               .arg(UIMd3Shape::Small));

    QLabel *pHeading = new QLabel(dish.displayName(), pToast);
    pHeading->setFont(md3Theme().font(UIMd3TypeRole_TitleMedium));
    QPalette headingPalette = pHeading->palette();
    headingPalette.setColor(QPalette::WindowText, onSurface);
    pHeading->setPalette(headingPalette);
    pHeading->setWordWrap(true);

    pLayout->addWidget(pKicker);
    pLayout->addWidget(pPhoto, 0, Qt::AlignHCenter);
    pLayout->addWidget(pHeading);

    /* Meaningful accessible text carries the whole delight even though the
     * toast intentionally never takes focus and shows no real picture. */
    const QString strAccessible = md3DimSumText("md3.dimsum.accessibleDescription",
                                                 tr("Dim sum of the moment: %1. Photo not included in this build."))
                                       .arg(dish.displayName());
    pToast->setAccessibleName(md3DimSumText("md3.dimsum.accessibleName", tr("Dim sum surprise")));
    pToast->setAccessibleDescription(strAccessible);
    pHeading->setAccessibleName(strAccessible);

    pToast->adjustSize();

    if (QScreen *pScreen = QGuiApplication::primaryScreen())
    {
        const QRect avail = pScreen->availableGeometry();
        const int x = avail.right() - pToast->width() - 24;
        const int y = avail.bottom() - pToast->height() - 24;
        pToast->move(qMax(avail.left(), x), qMax(avail.top(), y));
    }

    m_pToast = pToast;
    pToast->show();

    /* Proactively announce the toast to assistive technology, since it never
     * takes focus and some screen readers only notice focused windows. */
    QAccessibleEvent accessibleEvent(pToast, QAccessible::Alert);
    QAccessible::updateAccessibility(&accessibleEvent);

    const int iVisibleMs = 6000;
    if (prefersReducedMotion())
    {
        QTimer::singleShot(iVisibleMs, pToast, &QWidget::close);
    }
    else
    {
        QGraphicsOpacityEffect *pEffect = new QGraphicsOpacityEffect(pToast);
        pEffect->setOpacity(1.0);
        pToast->setGraphicsEffect(pEffect);

        QPointer<QWidget> pToastGuard(pToast);
        QTimer::singleShot(iVisibleMs, pToast, [pToastGuard, pEffect]()
        {
            if (!pToastGuard)
                return;
            QPropertyAnimation *pFade = new QPropertyAnimation(pEffect, "opacity", pToastGuard);
            pFade->setDuration(UIMd3Motion::Medium2);
            pFade->setStartValue(1.0);
            pFade->setEndValue(0.0);
            QObject::connect(pFade, &QPropertyAnimation::finished, pToastGuard, &QWidget::close);
            pFade->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }
}
