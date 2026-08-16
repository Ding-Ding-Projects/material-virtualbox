/* $Id$ */
/** @file
 * VBox Qt GUI - the 10%-per-launch dim sum startup surprise.
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

/* Qt includes: */
#include <QAccessible>
#include <QAccessibleEvent>
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

/* GUI includes: */
#include "UIMd3Button.h"
#include "UIMd3DimSum.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"
#include "UIMd3Widget.h"

namespace
{
    /** Selects which small, originally-authored local illustration a dish
      * renders as.  Every shape below is drawn with QPainter primitives at
      * paint time -- nothing here is a generated, downloaded, stock, or
      * scraped picture; see doc/md3/DimSum.md for why that boundary matters
      * for this specific feature. */
    enum Md3DimSumArt
    {
        Md3DimSumArt_Dumpling,
        Md3DimSumArt_Siumai,
        Md3DimSumArt_Bun,
        Md3DimSumArt_Tart,
        Md3DimSumArt_Roll,
        Md3DimSumArt_LotusWrap,
        Md3DimSumArt_Cake,
        Md3DimSumArt_SpongeCake
    };

    struct Md3DimSumCatalogueEntry
    {
        const char *pszId;
        const char *pszEnglish;
        const char *pszCantonese;
        Md3DimSumArt enmArt;
    };

    /** The bundled offline subset.  See doc/md3/DimSum.md for the exact
      * reasoning: the house contract for this feature points at the public
      * `Ding-Ding-Projects/dim-sum-photos` catalogue over HTTPS, and this
      * feature's own contract requires bundled local assets with no
      * network of any kind.  Those two requirements cannot both be met by
      * fetching the public catalogue, so this ships a small, honestly
      * documented offline subset instead, in the same bilingual shape. */
    const Md3DimSumCatalogueEntry g_aDishes[] =
    {
        { "har-gow",      "Har Gow (Shrimp Dumpling)",              "蝦餃",     Md3DimSumArt_Dumpling  },
        { "siu-mai",      "Siu Mai (Pork and Shrimp Dumpling)",     "燒賣",     Md3DimSumArt_Siumai    },
        { "char-siu-bao", "Char Siu Bao (Barbecue Pork Bun)",       "叉燒包",   Md3DimSumArt_Bun       },
        { "egg-tart",     "Egg Tart",                               "蛋撻",     Md3DimSumArt_Tart      },
        { "cheung-fun",   "Cheung Fun (Rice Noodle Roll)",          "腸粉",     Md3DimSumArt_Roll      },
        { "lo-mai-gai",   "Lo Mai Gai (Sticky Rice in Lotus Leaf)", "糯米雞",   Md3DimSumArt_LotusWrap },
        { "turnip-cake",  "Turnip Cake",                            "蘿蔔糕",   Md3DimSumArt_Cake      },
        { "ma-lai-go",    "Ma Lai Go (Steamed Sponge Cake)",        "馬拉糕",   Md3DimSumArt_SpongeCake }
    };
    const int g_cDishes = sizeof(g_aDishes) / sizeof(g_aDishes[0]);

    const int g_iStorageSchemaVersion = 1;
    const int g_iMaxStoragePayload = 4096;
    const int g_iToastAutoDismissMs = 9000;
    const int g_iToastFirstAttemptDelayMs = 1200;
    const int g_iToastRetryDelayMs = 1000;
    const int g_iToastMaxAttempts = 3;
    const int g_iToastMaxWidth = 320;
    const int g_iToastMargin = 20;
    const int g_iArtSize = 56;

    Md3DimSumArt artForDishId(const QString &strId)
    {
        for (int i = 0; i < g_cDishes; ++i)
            if (strId == QString::fromUtf8(g_aDishes[i].pszId))
                return g_aDishes[i].enmArt;
        return Md3DimSumArt_Dumpling;
    }

    /** Paints one small, original, code-drawn dish illustration into @a rect. */
    void md3PaintDishArt(QPainter &painter, const QRectF &rect, Md3DimSumArt enmArt)
    {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor base = md3(UIMd3ColorRole_TertiaryContainer);
        const QColor accent = md3(UIMd3ColorRole_OnTertiaryContainer);
        const QRectF r = rect.adjusted(4, 4, -4, -4);
        painter.setPen(QPen(accent, 2));
        painter.setBrush(base);

        switch (enmArt)
        {
            case Md3DimSumArt_Dumpling:
            {
                QPainterPath path;
                path.moveTo(r.left(), r.bottom());
                path.lineTo(r.center().x(), r.top());
                path.lineTo(r.right(), r.bottom());
                path.closeSubpath();
                painter.drawPath(path);
                for (int i = 1; i < 4; ++i)
                {
                    const qreal x = r.left() + (r.width() * i) / 4.0;
                    painter.drawLine(QPointF(x, r.bottom()),
                                     QPointF((x + r.center().x()) / 2.0, r.center().y()));
                }
                break;
            }
            case Md3DimSumArt_Siumai:
            {
                painter.drawRoundedRect(QRectF(r.left(), r.top() + r.height() * 0.15,
                                               r.width(), r.height() * 0.85), 4, 4);
                painter.setBrush(QColor(255, 200, 60));
                painter.drawEllipse(QPointF(r.center().x(), r.top() + r.height() * 0.15),
                                    r.width() * 0.09, r.width() * 0.09);
                break;
            }
            case Md3DimSumArt_Bun:
            {
                painter.drawEllipse(r);
                painter.drawLine(r.center() - QPointF(r.width() * 0.18, 0),
                                 r.center() + QPointF(r.width() * 0.18, 0));
                painter.drawLine(r.center() - QPointF(0, r.height() * 0.18),
                                 r.center() + QPointF(0, r.height() * 0.18));
                break;
            }
            case Md3DimSumArt_Tart:
            {
                QPainterPath path;
                path.addRoundedRect(r, r.width() * 0.5, 6);
                painter.drawPath(path);
                painter.setBrush(QColor(255, 210, 90));
                painter.drawEllipse(r.adjusted(r.width() * 0.18, r.height() * 0.22,
                                               -r.width() * 0.18, -r.height() * 0.1));
                break;
            }
            case Md3DimSumArt_Roll:
            {
                painter.drawRoundedRect(r, r.height() * 0.4, r.height() * 0.4);
                painter.setPen(QPen(QColor(120, 70, 20), 2));
                painter.drawLine(QPointF(r.left() + 6, r.center().y()),
                                 QPointF(r.right() - 6, r.center().y()));
                break;
            }
            case Md3DimSumArt_LotusWrap:
            {
                painter.setBrush(QColor(90, 130, 70));
                painter.drawRoundedRect(r, 6, 6);
                painter.setPen(QPen(QColor(210, 190, 140), 2));
                painter.drawLine(r.topLeft(), r.bottomRight());
                painter.drawLine(r.topRight(), r.bottomLeft());
                break;
            }
            case Md3DimSumArt_Cake:
            {
                painter.setBrush(QColor(245, 225, 190));
                painter.drawRoundedRect(r, 4, 4);
                painter.setPen(QPen(accent, 1));
                for (int i = 1; i < 3; ++i)
                {
                    const qreal x = r.left() + r.width() * i / 3.0;
                    painter.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
                }
                break;
            }
            case Md3DimSumArt_SpongeCake:
            default:
            {
                painter.setBrush(QColor(255, 224, 130));
                QPainterPath path;
                path.moveTo(r.left(), r.bottom());
                path.lineTo(r.right(), r.bottom());
                path.lineTo(r.right(), r.top());
                path.closeSubpath();
                painter.drawPath(path);
                break;
            }
        }
        painter.restore();
    }

    /** Tiny fixed-size widget that only ever paints one dish's illustration. */
    class Md3DimSumArtWidget : public QWidget
    {
    public:

        explicit Md3DimSumArtWidget(Md3DimSumArt enmArt, QWidget *pParent)
            : QWidget(pParent)
            , m_enmArt(enmArt)
        {
            setFixedSize(g_iArtSize, g_iArtSize);
            setFocusPolicy(Qt::NoFocus);
        }

    protected:

        virtual void paintEvent(QPaintEvent *) RT_OVERRIDE
        {
            QPainter painter(this);
            md3PaintDishArt(painter, QRectF(rect()), m_enmArt);
        }

    private:

        Md3DimSumArt m_enmArt;
    };

    /**
     * The actual non-blocking toast: a frameless, always-on-top, corner-
     * anchored Material 3 card that never activates and never steals focus.
     *
     * Deliberately has no Q_OBJECT of its own -- it introduces no new
     * signals or slots, connects only to inherited QWidget::close() and
     * UIMd3Button::sigClicked() (both already moc-generated by their own
     * classes), and overrides only ordinary virtual functions.  Kept as an
     * implementation detail of this .cpp rather than a second header/source
     * pair, so this feature's kBuild footprint stays exactly the two files
     * the build wiring below actually registers.
     */
    class Md3DimSumToast : public UIMd3Widget
    {
    public:

        explicit Md3DimSumToast(Md3DimSumArt enmArt)
            : UIMd3Widget(0, QStringLiteral("dimsum/toast"))
            , m_pTitle(0)
            , m_pDishName(0)
            , m_pDescription(0)
            , m_pDismiss(0)
            , m_pArt(0)
        {
            setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            setAttribute(Qt::WA_ShowWithoutActivating, true);
            setAttribute(Qt::WA_DeleteOnClose, true);
            setAttribute(Qt::WA_TranslucentBackground, true);
            setAttribute(Qt::WA_NoSystemBackground, true);
            setFocusPolicy(Qt::NoFocus);

            QVBoxLayout *pOuter = new QVBoxLayout(this);
            pOuter->setContentsMargins(16, 14, 16, 14);
            pOuter->setSpacing(8);

            QHBoxLayout *pTop = new QHBoxLayout;
            pTop->setSpacing(12);
            m_pArt = new Md3DimSumArtWidget(enmArt, this);
            pTop->addWidget(m_pArt, 0, Qt::AlignTop);

            QVBoxLayout *pTextColumn = new QVBoxLayout;
            pTextColumn->setSpacing(2);
            m_pTitle = new QLabel(this);
            m_pTitle->setFont(effectiveFont(UIMd3TypeRole_TitleSmall));
            m_pTitle->setStyleSheet(QStringLiteral("color: %1;").arg(md3(UIMd3ColorRole_OnSurface).name()));
            m_pDishName = new QLabel(this);
            m_pDishName->setFont(effectiveFont(UIMd3TypeRole_BodyMedium));
            m_pDishName->setStyleSheet(QStringLiteral("color: %1;").arg(md3(UIMd3ColorRole_OnSurfaceVariant).name()));
            m_pDishName->setWordWrap(true);
            pTextColumn->addWidget(m_pTitle);
            pTextColumn->addWidget(m_pDishName);
            pTop->addLayout(pTextColumn, 1);
            pOuter->addLayout(pTop);

            m_pDescription = new QLabel(this);
            m_pDescription->setFont(effectiveFont(UIMd3TypeRole_BodySmall));
            m_pDescription->setStyleSheet(QStringLiteral("color: %1;").arg(md3(UIMd3ColorRole_OnSurfaceVariant).name()));
            m_pDescription->setWordWrap(true);
            pOuter->addWidget(m_pDescription);

            QHBoxLayout *pActions = new QHBoxLayout;
            pActions->addStretch(1);
            m_pDismiss = new UIMd3Button(QString(), UIMd3ButtonVariant_Text, this);
            connect(m_pDismiss, &UIMd3Button::sigClicked, this, &QWidget::close);
            pActions->addWidget(m_pDismiss);
            pOuter->addLayout(pActions);

            setMaximumWidth(g_iToastMaxWidth);
            m_pDishName->setMaximumWidth(g_iToastMaxWidth - g_iArtSize - 12 - 32);
            m_pDescription->setMaximumWidth(g_iToastMaxWidth - 32);

            if (qApp)
                qApp->installEventFilter(this);
        }

        virtual ~Md3DimSumToast() RT_OVERRIDE
        {
            if (qApp)
                qApp->removeEventFilter(this);
        }

        QLabel *titleLabel() const { return m_pTitle; }
        QLabel *dishNameLabel() const { return m_pDishName; }
        QLabel *descriptionLabel() const { return m_pDescription; }
        UIMd3Button *dismissButton() const { return m_pDismiss; }
        QWidget *artWidget() const { return m_pArt; }

    protected:

        virtual void paintEvent(QPaintEvent *) RT_OVERRIDE
        {
            QPainter painter(this);
            paintContainer(painter, rect(), UIMd3ColorRole_SurfaceContainerHigh, UIMd3Shape::Large);
        }

        /** Application-wide filter so Escape dismisses the toast even though
          * it deliberately never takes keyboard focus. This is the toast's
          * whole keyboard story: it never steals focus on appearance, but a
          * keyboard user can still close it early from wherever their focus
          * already is, and it auto-dismisses on its own regardless. */
        virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE
        {
            if (pEvent->type() == QEvent::KeyPress && isVisible())
            {
                QKeyEvent *pKeyEvent = static_cast<QKeyEvent *>(pEvent);
                if (pKeyEvent->key() == Qt::Key_Escape)
                {
                    close();
                    return true;
                }
            }
            return UIMd3Widget::eventFilter(pObject, pEvent);
        }

    private:

        QLabel *m_pTitle;
        QLabel *m_pDishName;
        QLabel *m_pDescription;
        UIMd3Button *m_pDismiss;
        QWidget *m_pArt;
    };
} // anonymous namespace

UIMd3DimSum *UIMd3DimSum::s_pInstance = 0;

UIMd3DimSum *UIMd3DimSum::instance() { return s_pInstance; }

void UIMd3DimSum::create()
{
    if (!s_pInstance)
        s_pInstance = new UIMd3DimSum;
}

void UIMd3DimSum::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3DimSum::UIMd3DimSum()
    : QObject(0)
    , m_fAlreadyInvited(false)
    , m_fFirstLaunchEver(false)
    , m_fWon(false)
{
    registerText();

    /* This is the only "first run" signal this codebase has for this
     * feature: nothing else here tracks whether the application has ever
     * been launched before under this profile, so this service tracks it
     * itself in its own tiny persisted file.  A fresh profile therefore
     * always skips the surprise on launch 1 and becomes eligible from
     * launch 2 onward. */
    m_fFirstLaunchEver = !loadHasLaunchedBefore();
    saveHasLaunchedBefore();

    /* One fresh draw per process, taken right now rather than at display
     * time, so "never fire twice in one launch" and "a fresh random draw
     * per launch" both hold by construction rather than by discipline at
     * every call site. */
    m_fWon = QRandomGenerator::global()->bounded(100) < 10;
}

UIMd3DimSum::~UIMd3DimSum()
{
}

void UIMd3DimSum::registerText()
{
    if (!UIMd3Language::instance())
        return;
    UIMd3Language::instance()->registerText(
        QStringLiteral("md3.dimsum.title"),
        QStringLiteral("Dim sum surprise"),
        QStringLiteral("飲茶驚喜"));
    UIMd3Language::instance()->registerText(
        QStringLiteral("md3.dimsum.description"),
        QStringLiteral("A small dim sum surprise: %1. It closes on its own -- press Escape anytime to dismiss it now."),
        QStringLiteral("細細個飲茶驚喜：%1。佢會自己閂返，隨時撳 Escape 即刻收埋。"));
    UIMd3Language::instance()->registerText(
        QStringLiteral("md3.dimsum.dismiss"),
        QStringLiteral("Dismiss"),
        QStringLiteral("收埋"));
}

QList<UIMd3DimSumDish> UIMd3DimSum::catalogue()
{
    QList<UIMd3DimSumDish> result;
    for (int i = 0; i < g_cDishes; ++i)
    {
        UIMd3DimSumDish dish;
        dish.strId = QString::fromUtf8(g_aDishes[i].pszId);
        dish.strNameEnglish = QString::fromUtf8(g_aDishes[i].pszEnglish);
        dish.strNameCantonese = QString::fromUtf8(g_aDishes[i].pszCantonese);
        result << dish;
    }
    return result;
}

QString UIMd3DimSum::dishDisplayName(const UIMd3DimSumDish &dish)
{
    UIMd3Language *pLanguage = UIMd3Language::instance();
    const UIMd3LanguageMode enmMode = pLanguage ? pLanguage->mode() : UIMd3LanguageMode_English;
    if (enmMode == UIMd3LanguageMode_Cantonese)
        return dish.strNameCantonese;
    if (enmMode == UIMd3LanguageMode_Bilingual)
        return QStringLiteral("%1 · %2").arg(dish.strNameEnglish, dish.strNameCantonese);
    return dish.strNameEnglish;
}

void UIMd3DimSum::maybeShowAtStartup(QWidget *pParent)
{
    if (!pParent || m_fAlreadyInvited)
        return;
    /* Only the first invitation this process ever receives can matter --
     * this also protects against the same surface calling twice, or two
     * different top-level windows both inviting the surprise. */
    m_fAlreadyInvited = true;

    /* Never during a first run; never when the one-shot roll did not win. */
    if (m_fFirstLaunchEver || !m_fWon)
        return;

    QPointer<QWidget> pSafeParent(pParent);
    QTimer::singleShot(g_iToastFirstAttemptDelayMs, this, [this, pSafeParent]()
    {
        attemptShow(pSafeParent, g_iToastMaxAttempts);
    });
}

void UIMd3DimSum::attemptShow(QPointer<QWidget> pParent, int iAttemptsLeft)
{
    if (!pParent || iAttemptsLeft <= 0)
        return;

    /* Never fight an error path, a medium-enumeration warning, or any other
     * blocking flow for the screen: if something modal is up, wait a beat
     * and try again a bounded number of times, then quietly give up. */
    if (QApplication::activeModalWidget() != 0)
    {
        QTimer::singleShot(g_iToastRetryDelayMs, this, [this, pParent, iAttemptsLeft]()
        {
            attemptShow(pParent, iAttemptsLeft - 1);
        });
        return;
    }

    const QList<UIMd3DimSumDish> dishes = catalogue();
    if (dishes.isEmpty())
        return;
    const int iIndex = QRandomGenerator::global()->bounded(static_cast<int>(dishes.size()));
    showToast(pParent, dishes.at(iIndex));
}

void UIMd3DimSum::showToast(QWidget *pParent, const UIMd3DimSumDish &dish)
{
    if (!pParent)
        return;

    Md3DimSumToast *pToast = new Md3DimSumToast(artForDishId(dish.strId));

    const QString strAltText = dishDisplayName(dish);
    const QString strTitle = md3Text(QStringLiteral("md3.dimsum.title"));
    const QString strDescription = md3Text(QStringLiteral("md3.dimsum.description")).arg(strAltText);

    pToast->titleLabel()->setText(strTitle);
    pToast->dishNameLabel()->setText(strAltText);
    pToast->descriptionLabel()->setText(strDescription);
    pToast->dismissButton()->setText(md3Text(QStringLiteral("md3.dimsum.dismiss")));

    /* Alt text: the picture itself carries the dish's factual bilingual
     * name, independent of whatever the funny-level sliders did to the
     * surrounding chrome text. */
    pToast->artWidget()->setAccessibleName(strAltText);
    pToast->setAccessibleName(strTitle);
    pToast->setAccessibleDescription(strDescription);

    pToast->adjustSize();

    QScreen *pScreen = pParent->screen();
    if (!pScreen)
        pScreen = QGuiApplication::primaryScreen();
    if (pScreen)
    {
        const QRect avail = pScreen->availableGeometry();
        int iX = avail.right() - pToast->width() - g_iToastMargin;
        int iY = avail.bottom() - pToast->height() - g_iToastMargin;
        iX = qMax(avail.left() + g_iToastMargin, iX);
        iY = qMax(avail.top() + g_iToastMargin, iY);
        pToast->move(iX, iY);
    }

    /* WA_ShowWithoutActivating plus a plain show() -- never activateWindow(),
     * never setFocus() -- is what keeps this from stealing focus or gating
     * startup: the caller's own event loop is already running and this
     * returns immediately either way. */
    pToast->show();

    QAccessibleEvent accessibleEvent(pToast, QAccessible::Alert);
    QAccessible::updateAccessibility(&accessibleEvent);

    QTimer::singleShot(g_iToastAutoDismissMs, pToast, &QWidget::close);

    emit sigDishShown(dish.strId);
}

QString UIMd3DimSum::storagePath()
{
    const QString strLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strLocation.isEmpty())
        return QString();
    return QDir(strLocation).filePath(QStringLiteral("md3-dimsum.json"));
}

bool UIMd3DimSum::loadHasLaunchedBefore()
{
    const QString strPath = storagePath();
    if (strPath.isEmpty())
        return false;

    QFile file(strPath);
    if (!file.exists() || file.size() <= 0 || file.size() > g_iMaxStoragePayload)
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray data = file.read(g_iMaxStoragePayload + 1);
    if (data.isEmpty() || data.size() > g_iMaxStoragePayload)
        return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt(-1) != g_iStorageSchemaVersion)
        return false;

    return root.value(QStringLiteral("hasLaunchedBefore")).toBool(false);
}

void UIMd3DimSum::saveHasLaunchedBefore()
{
    const QString strPath = storagePath();
    if (strPath.isEmpty())
        return;

    const QFileInfo fileInfo(strPath);
    if (!QDir().mkpath(fileInfo.absolutePath()))
        return;

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), g_iStorageSchemaVersion);
    root.insert(QStringLiteral("hasLaunchedBefore"), true);

    QSaveFile file(strPath);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();
}
