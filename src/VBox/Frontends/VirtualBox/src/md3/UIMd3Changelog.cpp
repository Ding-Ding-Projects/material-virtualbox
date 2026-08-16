/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 in-app changelog viewer.
 */

/*
 * Copyright (C) 2026 Material Virtual Machine contributors.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3 of the
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
#include <QClipboard>
#include <QComboBox>
#include <QDateEdit>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPalette>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSize>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3Changelog.h"
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

namespace
{
    QString md3ChangelogText(const char *pszKey, const QString &strFallback)
    {
        UIMd3Language *pLanguage = UIMd3Language::instance();
        return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
    }

    /** Adds one compiled-in entry.  Every field is transcribed from the
      * repository's own CHANGELOG.md; every commit hash was verified with
      * <tt>git cat-file -e</tt> and its date read back with <tt>git log</tt>
      * against this exact checkout before this table was written -- see
      * doc/md3/Changelog.md for the verification record. Nothing here is a
      * placeholder or an invented example. */
    void addEntry(QList<UIMd3ChangelogEntry> &entries,
                  const QString &strVersion, const QDate &date,
                  const QString &strCategory, const QString &strSection,
                  const QString &strTitle, const QString &strDetail,
                  const QString &strCommitSha, bool fOnDefaultBranch = true)
    {
        UIMd3ChangelogEntry entry;
        entry.strVersion = strVersion;
        entry.date = date;
        entry.strCategory = strCategory;
        entry.strSection = strSection;
        entry.strTitle = strTitle;
        entry.strDetail = strDetail;
        entry.strCommitSha = strCommitSha;
        entry.strCommitUrl = QStringLiteral("https://github.com/Ding-Ding-Projects/material-virtualbox/commit/%1")
                             .arg(strCommitSha);
        entry.fOnDefaultBranch = fOnDefaultBranch;
        entries << entry;
    }
}

UIMd3Changelog *UIMd3Changelog::s_pInstance = 0;

UIMd3Changelog *UIMd3Changelog::instance()
{
    return s_pInstance;
}

void UIMd3Changelog::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3Changelog;
    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.title"),
                                                 QStringLiteral("Changelog"),
                                                 QStringLiteral("更新日誌"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.search"),
                                                 QStringLiteral("Search changelog text, titles, or commit hashes"),
                                                 QStringLiteral("搜尋更新日誌內文、標題或提交雜湊"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.version"),
                                                 QStringLiteral("Version"),
                                                 QStringLiteral("版本"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.allVersions"),
                                                 QStringLiteral("All versions"),
                                                 QStringLiteral("所有版本"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.category"),
                                                 QStringLiteral("Category"),
                                                 QStringLiteral("分類"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.allCategories"),
                                                 QStringLiteral("All categories"),
                                                 QStringLiteral("所有分類"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.from"),
                                                 QStringLiteral("From"),
                                                 QStringLiteral("由"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.to"),
                                                 QStringLiteral("To"),
                                                 QStringLiteral("到"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.anyDate"),
                                                 QStringLiteral("Any date"),
                                                 QStringLiteral("任何日期"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.export"),
                                                 QStringLiteral("Export Markdown"),
                                                 QStringLiteral("匯出 Markdown"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.copy"),
                                                 QStringLiteral("Copy to clipboard"),
                                                 QStringLiteral("複製到剪貼簿"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.noMatch"),
                                                 QStringLiteral("No changelog entries match this filter"),
                                                 QStringLiteral("冇更新日誌符合呢個篩選"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.summary"),
                                                 QStringLiteral("%1 of %2 entries"),
                                                 QStringLiteral("%1 / %2 項紀錄"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.exported"),
                                                 QStringLiteral("Exported %1 entries."),
                                                 QStringLiteral("已匯出 %1 項紀錄。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.copied"),
                                                 QStringLiteral("Copied %1 entries to the clipboard."),
                                                 QStringLiteral("已將 %1 項紀錄複製到剪貼簿。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.unreleasedNotice"),
                                                 QStringLiteral("Every entry below is listed under the commit that made it. "
                                                                 "Published releases exist for this repository; open any "
                                                                 "entry's commit link to see exactly where it landed."),
                                                 QStringLiteral("以下每一項都係按住做出嗰個改動嘅提交嚟列。"
                                                                 "呢個倉庫已經有發佈咗嘅版本；打開任何一項嘅提交連結，"
                                                                 "就可以睇到佢實際去咗邊。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.openCommit"),
                                                 QStringLiteral("Open commit %1 in your browser"),
                                                 QStringLiteral("喺瀏覽器開啟提交 %1"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.commitTooltip"),
                                                 QStringLiteral("Full commit hash: %1\nOpens in your default browser."),
                                                 QStringLiteral("完整提交雜湊：%1\n會用預設瀏覽器開啟。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.changelog.offBranch"),
                                                 QStringLiteral("Not yet on the default branch"),
                                                 QStringLiteral("仲未併入預設分支"));
    }
}

void UIMd3Changelog::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3Changelog::UIMd3Changelog()
    : QObject(0)
    , m_pHeading(0)
    , m_pUnreleasedNotice(0)
    , m_pSearch(0)
    , m_pVersion(0)
    , m_pCategory(0)
    , m_pFrom(0)
    , m_pTo(0)
    , m_pVersionLabel(0)
    , m_pCategoryLabel(0)
    , m_pFromLabel(0)
    , m_pToLabel(0)
    , m_pScrollArea(0)
    , m_pRowsLayout(0)
    , m_pStatus(0)
    , m_pExport(0)
    , m_pCopy(0)
{
    prepareEntries();
}

UIMd3Changelog::~UIMd3Changelog()
{
}

void UIMd3Changelog::prepareEntries()
{
    m_entries.clear();
    const QString strUnreleased = QStringLiteral("Unreleased");
    const QDate pipelineDate(2026, 8, 13);
    const QString strPipelineSection = QStringLiteral("Windows build and packaging pipeline");
    const QDate featuresDate(2026, 8, 14);
    const QString strFeaturesSection = QStringLiteral("Three canonical features that had no implementation at all");

    /* Release versions.  CHANGELOG.md's own rule is that an entry leaves
     * [Unreleased] once it appears in a published release; these constants are
     * how that rule is expressed here.  Which release first carried which
     * commit was determined with git merge-base --is-ancestor against each
     * release's target commitish, read from the GitHub releases API -- not
     * guessed from dates. */
    const QString strCi96  = QStringLiteral("v7.2.97-ci.96");
    const QString strCi101 = QStringLiteral("v7.2.97-ci.101");
    const QString strCi106 = QStringLiteral("v7.2.97-ci.106");
    const QString strCi107 = QStringLiteral("v7.2.97-ci.107");
    const QString strCi108 = QStringLiteral("v7.2.97-ci.108");
    const QDate releaseDate15(2026, 8, 15);
    const QString strReleaseSection = QStringLiteral("Releases");

    /* ---- Unreleased: written, not yet in any published release. ---- */

    addEntry(m_entries, strUnreleased, QDate(2026, 8, 16), QStringLiteral("Fixed"), QString(),
             QStringLiteral("Repair the stale README contract and add a localization gate"),
             QStringLiteral(
                 "The \"Compact README contract\" step in the Material 3 validation workflow "
                 "was red: it required the literal 'no verified installer is published yet', "
                 "and README.md now states the opposite and true thing -- verified unsigned "
                 "NSIS installers are published as GitHub releases. The assertion is replaced "
                 "rather than deleted, and two negative guards were added so the claim can "
                 "never be quietly upgraded to a signed installer. The same commit adds "
                 "tools/md3/check-language-registry.ps1, an executable localization gate over "
                 "the UIMd3Language text registry, wired into CI with -MaxUnparseable 2. Its "
                 "first run failed on a real defect: md3.wizard.current-page-description was "
                 "registered twice with different text, and registerText ends in "
                 "QHash::insert, which overwrites -- so one of the two readers was always "
                 "wrong. Limits: nothing was compiled; the gate is literal-only and reports 2 "
                 "unparseable call sites and the 41 registrations behind them as NOT CHECKED; "
                 "Material 3 validation is still red on main until this is pushed."),
             QStringLiteral("ca97ec93ccc06559d1035dd039ef56ca7b000a93"));

    addEntry(m_entries, strUnreleased, QDate(2026, 8, 16), QStringLiteral("Added"), QString(),
             QStringLiteral("A language switcher on the documentation site"),
             QStringLiteral(
                 "docs/index.html gains an inline English / Cantonese / bilingual switcher "
                 "mirroring UIMd3LanguageMode: same three modes in the same order, the same "
                 "middle-dot bilingual join as UIMd3Language::text(), the same "
                 "empty-Cantonese-falls-back-to-English rule, the same English default, and "
                 "persistence with the same clamp as GUI/Md3/LanguageMode. English remains the "
                 "literal document text, so the page without JavaScript is exactly the page "
                 "that shipped before. No subresources, no innerHTML. Limits: nothing was "
                 "compiled; only 14.9% of the page's visible characters get Cantonese; all 30 "
                 "published doc/md3/*.md articles stay English; the behavioural harness that "
                 "drove it in a real browser is a scratch file outside the repository and is "
                 "not a repeatable gate; the deployed site was not fetched."),
             QStringLiteral("2048ed0a7b651373a2707d30b4446a888861f33f"));

    /* The 2026-08-16 documentation-correction pass that added the release
     * entries below is deliberately NOT an entry here yet.  It is the commit
     * that contains this very edit, and no viewer can cite its own
     * not-yet-created hash without inventing one.  CHANGELOG.md records the
     * pass; a later task adds the entry once the hash exists, exactly as was
     * done for the viewer's own first entry.  The rule is unchanged: wait for
     * the hash, never invent it. */

    /* ---- Published releases. ---- */

    addEntry(m_entries, strCi108, releaseDate15, QStringLiteral("Released"), strReleaseSection,
             QStringLiteral("v7.2.97-ci.108 -- Dried Scallop Shrimp Dumpling"),
             QStringLiteral(
                 "Latest release, non-draft, published 2026-08-15T01:21:51Z by run "
                 "31851996367 (success, 1h26m24s). Installer VirtualBox-7.2.97-Setup.exe, "
                 "106,963,266 bytes, SHA-256 "
                 "8a6e1e94bdd73c3a9c526b7b7e074521f06a2562b9f9020ea1a806f15fcda627, plus "
                 "SHA256SUMS.txt and a dim sum photograph. No new frontend feature; it carries "
                 "everything in ci.107, ci.106 and ci.101. Material 3 validation is RED at "
                 "this commit (run 31856303192). No virtual machine can start from this "
                 "installer: VBoxSup.sys ships unsigned, 64-bit Windows will not load an "
                 "unsigned kernel driver, and code signing is permanently prohibited for this "
                 "project."),
             QStringLiteral("fe321a4fd6f6bfc7c2fbd7e0704e3f72d0eb2eee"));

    addEntry(m_entries, strCi107, releaseDate15, QStringLiteral("Released"), strReleaseSection,
             QStringLiteral("v7.2.97-ci.107 -- Lobster Dumpling"),
             QStringLiteral(
                 "Non-draft, published 2026-08-15T01:20:54Z by run 31851883237 (success, "
                 "1h27m26s). Installer 106,981,357 bytes, SHA-256 "
                 "a7540294d102bf31b4f009302dcee827d80549d678f77a5f1c7676597efe9237. Build "
                 "tooling only: bootstraps libxslt for native Windows builds."),
             QStringLiteral("753602ced18e50649ce0b0a3038cf7efd1ba97fe"));

    addEntry(m_entries, strCi106, releaseDate15, QStringLiteral("Released"), strReleaseSection,
             QStringLiteral("v7.2.97-ci.106 -- Pea Shoot Shrimp Dumpling"),
             QStringLiteral(
                 "Non-draft, published 2026-08-15T00:21:28Z by run 31848342345 (success, "
                 "1h28m12s). Installer 106,902,297 bytes, SHA-256 "
                 "ef62c3fec34e3b387ae7c0be06c6b1b895d6be921bd6059682e69c91f24ef7c3. This is "
                 "the release in which the emoji toggle, the dim sum surprise and the "
                 "personal-vocabulary upload first compiled and shipped. Their own "
                 "integration commit d548859c25785ec20d61e56f018ba25da5d2bb70 had failed its "
                 "build (run 31843086131, at step \"Build and package the Windows "
                 "installer\"); this is the first commit carrying all three whose build went "
                 "green. None of the three has been tested or captured."),
             QStringLiteral("8762579681594cf8ba6beb8ccec77576baa5199a"));

    addEntry(m_entries, strCi101, QDate(2026, 8, 14), QStringLiteral("Released"), strReleaseSection,
             QStringLiteral("v7.2.97-ci.101 -- Spinach Shrimp Dumpling"),
             QStringLiteral(
                 "Non-draft, published 2026-08-14T22:30:40Z by run 31840815630 (success, "
                 "1h26m14s). Installer 106,884,648 bytes, SHA-256 "
                 "bc9f86cbd21885e17837b438414ab4d61f51918e1ef59501be4bc58feb94f7db -- 11,910 "
                 "bytes larger than ci.100, consistent with a roughly 1,010-line class "
                 "landing. This is the release in which this changelog viewer itself was "
                 "compiled for the first time. It has still never been launched, screenshotted "
                 "or tested. Note that tags ci.102 through ci.105 were never created; the tag "
                 "counter is the workflow run number, not a release count."),
             QStringLiteral("bb63f016a5da05a9880ea957609695770004763c"));

    addEntry(m_entries, strCi96, QDate(2026, 8, 14), QStringLiteral("Released"), strReleaseSection,
             QStringLiteral("v7.2.97-ci.96 -- Classic Har Gow, the first release ever published"),
             QStringLiteral(
                 "Non-draft, published 2026-08-14T03:35:09Z, target "
                 "0d9eda43cd0f8ba69cc32ba5fc865b90dca64218. Run 31762849415 is the first "
                 "Windows packaging run in this repository's history that ever completed "
                 "success; every run before it failed or was cancelled. ci.97 through ci.100 "
                 "followed the same day, ci.100 with an installer of 106,872,738 bytes. These "
                 "carry the build and packaging fix chain recorded below."),
             QStringLiteral("0d9eda43cd0f8ba69cc32ba5fc865b90dca64218"));

    addEntry(m_entries, strCi101, featuresDate, QStringLiteral("Added"), QString(),
             QStringLiteral("Add an in-app changelog viewer"),
             QStringLiteral(
                 "Added the changelog viewer itself (UIMd3Changelog, reachable from the "
                 "Manager with Ctrl+Shift+L or the command palette's \"Open changelog\") "
                 "that compiles in every entry this file records, with plain-text/regex "
                 "search, version/category/date filters that compose, clickable per-entry "
                 "commit references, and filtered Markdown export/copy. See "
                 "doc/md3/Changelog.md for the full contract, including why the entry set "
                 "is a verified compiled-in transcription of CHANGELOG.md rather than a "
                 "runtime parser, and the maintenance duty that follows: every future edit "
                 "to CHANGELOG.md's entries must be mirrored into "
                 "UIMd3Changelog::prepareEntries() in the same task -- this entry is the "
                 "first one added under that duty."),
             QStringLiteral("aa52c21c9f4d3b1ac1fa961ed9fee70c75f91cb5"));

    addEntry(m_entries, strCi106, featuresDate, QStringLiteral("Added"), strFeaturesSection,
             QStringLiteral("Emoji-in-dialogs toggle"),
             QStringLiteral(
                 "UIMd3EmojiSetting adds a persisted toggle, default off, mapping eight "
                 "dialog tones to one decorative emoji each and returning text untouched "
                 "when disabled. Reachable from the command palette. Limit: no dialog or "
                 "message box in the frontend calls the decoration helper yet, so enabling "
                 "it changes nothing visible except the palette row's own label."),
             QStringLiteral("72b74ebcaa2e686aca23c19d820bda795fc5a55b"));

    addEntry(m_entries, strCi106, featuresDate, QStringLiteral("Added"), strFeaturesSection,
             QStringLiteral("Dim sum startup surprise"),
             QStringLiteral(
                 "UIMd3DimSumSurprise draws once per launch with a 10% chance and shows a "
                 "non-blocking, auto-dismissing toast naming a dish in English and "
                 "Cantonese. It cannot gate startup, cannot steal focus, and cannot be "
                 "turned off. By design it ships no photograph: those images belong to a "
                 "separate public catalog and are never vendored, generated, or downloaded "
                 "into this repository, so the toast renders an explicit placeholder where "
                 "a picture would go."),
             QStringLiteral("2792e4038262470bfc3db592e88ec4437bbca636"));

    addEntry(m_entries, strCi106, featuresDate, QStringLiteral("Added"), strFeaturesSection,
             QStringLiteral("Local personal-vocabulary JSON upload"),
             QStringLiteral(
                 "UIMd3PersonalVocabulary adds an always-visible file picker and a "
                 "bounded, versioned, all-or-nothing validator: file size, schema "
                 "version, nesting depth, entry count, key and value lengths and value "
                 "types are all bounded, and nesting is checked by scanning raw bytes "
                 "before any parser builds a tree. Nothing ships preloaded -- no samples, "
                 "no templates, no defaults. Limit: no other surface routes its rendered "
                 "text through the service yet."),
             QStringLiteral("804587eb558ca774ef04454f691ad89c7a8f25f9"));

    addEntry(m_entries, strCi106, featuresDate, QStringLiteral("Changed"), QString(),
             QStringLiteral("The completeness inventory counts itself now"),
             QStringLiteral(
                 "The inventory's summary table had drifted from the rows it summarises "
                 "-- three sections changed status while the Implemented count stayed "
                 "put, producing a table that added up correctly and described the tree "
                 "incorrectly. The rows stay hand-written, because a generated checklist "
                 "cannot look for a feature that has no implementation anywhere; only the "
                 "arithmetic became mechanical, through tools/md3/count-inventory-rows.py, "
                 "which fails closed when its own buckets disagree with its own total. "
                 "The same commit corrects section 16, which had claimed the "
                 "external-editor handoff was unimplemented long after it shipped."),
             QStringLiteral("8034b1cacb17c610f1afaec8fc99459dcfdc8f20"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Build the host binaries before packing them"),
             QStringLiteral(
                 "The workflow ran \"kmk ... packing\" on its own. That target only stages "
                 "already-built binaries into the installer payload; it does not build them. "
                 "It passed locally only because roughly eighteen prior local builds had left "
                 "VirtualBox.exe and friends sitting in out/. A clean CI checkout has no such "
                 "attic, and failed with \"The packaged Windows payload is missing "
                 "VirtualBox.exe\" after successfully building everything else. Split into a "
                 "full kmk build pass followed by the kmk ... packing pass, matching "
                 "VirtualBox's own documented build sequence."),
             QStringLiteral("e68fd88b9ae3d9f4b0f9c1d23d2c4249a1eb6f7a"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Install the Qt modules the frontend actually links"),
             QStringLiteral(
                 "aqtinstall was invoked with no --modules argument at all, which installs "
                 "qtbase only. UICommon_QT_MODULES needs Help and StateMachine in addition to "
                 "what qtbase supplies, so the build reached the link step roughly 33 minutes "
                 "in before failing on a missing Qt6StateMachine.lib. Also hardened the \"is "
                 "Qt already installed?\" guards, which previously checked only for qmake.exe "
                 "-- a cached module-less Qt install has a perfectly good qmake.exe and would "
                 "have made this fix silently do nothing on a warm cache."),
             QStringLiteral("04201aa85441341616cc90a84e4a9fdaac433b9d"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Ask for the Qt module that exists"),
             QStringLiteral(
                 "The previous fix requested --modules qtscxml qttools. aqtinstall rejected "
                 "qttools outright (\"package not found\"), and installed nothing for that "
                 "step -- Qt ships qttools inside the base desktop package, not as a "
                 "selectable add-on module; only qtscxml (which supplies StateMachine) needed "
                 "to be requested explicitly. The post-install assertion added in the "
                 "previous commit is what caught this in four minutes instead of another 33."),
             QStringLiteral("7d797e64e5f864244fe3113d1c5349db7eddf7ce"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Choose the v143 toolset in CI instead of hoping"),
             QStringLiteral(
                 "configure.py's MSVC auto-probe selected 14.29.30133 -- the v142 "
                 "compatibility compiler VS 2022 Enterprise ships alongside v143 -- on the "
                 "hosted runner image. It compiles the entire tree without complaint, so the "
                 "build ran a further 39 minutes before failing on a missing "
                 "vcruntime140.dll, because VCC143.kmk looks for its redistributables under a "
                 "14.[34]* path that a 14.29 toolset does not match. The workflow now selects "
                 "the toolset explicitly, mirroring the local tools/build-windows.ps1 logic, "
                 "and requires a 14.3x/14.4x version or throws immediately naming what was "
                 "rejected and why."),
             QStringLiteral("5cf840a2abd4d13eb12083e134aac072239f3505"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Select the newest MSVC toolset, not the oldest"),
             QStringLiteral(
                 "configure.py sorted the installed MSVC toolsets newest-first, then looped "
                 "over the whole list assigning into the same variable with os.path.join. "
                 "Because glob.glob returns absolute paths and os.path.join discards "
                 "everything before the final absolute argument, every loop iteration "
                 "overwrote the last, so the loop finished holding the last (oldest) entry "
                 "from a descending sort -- the same v142 toolset the previous fix had just "
                 "taught the workflow to reject. Fixed to take index 0 and stop. Also "
                 "corrected --with-vc to pass the installation root configure.py expects, "
                 "rather than a pre-resolved toolset directory."),
             QStringLiteral("d3ec5d6a11a8a17ca24802e26d96745991afe119"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Hand kBuild forward slashes so it can find the redistributables"),
             QStringLiteral(
                 "VCC143.kmk only takes its correct redistributable path when "
                 "PATH_TOOL_VCC143 ends in /tools/msvc/ with forward slashes. configure.py "
                 "was emitting a mixed-separator path (forward slashes from short-path "
                 "normalization, backslashes from os.path.join), so the check never matched. "
                 "kBuild fell through to an ancestor search that found a directory literally "
                 "named VC/Redist -- correctly named, genuinely present, and one directory "
                 "short of where the actual DLLs live under MSVC/<version>. The build ran a "
                 "further 39 minutes before failing on the same missing vcruntime140.dll "
                 "symptom as before, for an unrelated reason. Normalized separators once, at "
                 "the point the toolset path is chosen."),
             QStringLiteral("c48dbebe6a0c1eb93eb41ee33a23fde5987f0435"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Fixed"), strPipelineSection,
             QStringLiteral("Let Qt ask about a macro MSVC will never define"),
             QStringLiteral(
                 "Qt's qnumeric.h guards a branch with \"#if (Q_CC_GNU >= 500 || ...)\", and "
                 "Q_CC_GNU is a GCC-only macro that is simply undefined under MSVC -- which is "
                 "exactly the case the code is handling correctly. MSVC's C4668 warning "
                 "exists to flag that substitution, and with -Wall -WX in effect it became a "
                 "fatal error the moment any translation unit that includes QtCore was "
                 "compiled, 49 minutes into the build. Added -wd4668 to VBOX_VCC_WARN_ALL, "
                 "unconditionally rather than only inside the VCC143 guard, because Qt does "
                 "this under every MSVC version. Left the other nine warnings the same build "
                 "run surfaced (C4223, C4295, C4133, C4113, C4142, C5274, C4474, C4476, "
                 "C5267) untouched and visible."),
             QStringLiteral("9efa7f522c69f3ce1b99940d13e96d534ee2a0c3"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Resolved"), strPipelineSection,
             QStringLiteral("Packaging step used to fail later, on something new (resolved)"),
             QStringLiteral(
                 "Recorded at the time as: the Windows package and release workflow is still "
                 "red; it gets substantially further than any run before it -- past Qt, past "
                 "the toolset selection, past the redistributable path, through the full build "
                 "and into the packaging step's own self-check -- before failing because "
                 "tstVMStructSize.exe and tstAsmStructs.exe, two build-time self-check "
                 "testcases, run and crash with STATUS_STACK_BUFFER_OVERRUN (0xC0000409) "
                 "rather than reporting a structure-layout mismatch; no release has been "
                 "published and no verified Windows installer exists. RESOLVED, recorded "
                 "2026-08-16: both closing claims are now false. The blocker was worked around "
                 "with VBOX_WITHOUT_VMM_RUN_STRUCT_TESTS=1. That fix was NOT proved by the "
                 "next run -- run 31740515001 at cb9f573030e completed failure, and "
                 "doc/md3/LocalGates.md left it recorded as \"in_progress, do not infer a "
                 "result\" for days. The first Windows packaging run that ever completed "
                 "success was run 31762849415, which published v7.2.97-ci.96. Nine releases "
                 "exist now."),
             QStringLiteral("9efa7f522c69f3ce1b99940d13e96d534ee2a0c3"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Documentation"), QString(),
             QStringLiteral("Add changelog, capture matrix, and bring docs current"),
             QStringLiteral(
                 "Added CHANGELOG.md itself, a capture-matrix tracking table "
                 "(doc/md3/CaptureMatrix.md), and brought HANDOFF.md, ROADMAP.md, and "
                 "README.md current with the build/packaging state recorded above."),
             QStringLiteral("51ed3bc2ae78940357e56f5043a18ddbc9108123"));

    addEntry(m_entries, strCi96, pipelineDate, QStringLiteral("Related work"), QString(),
             QStringLiteral("Material 3 runtime chrome branch reconciled with main (has since landed)"),
             QStringLiteral(
                 "Merges 17 commits of native Material Design 3 runtime window chrome with "
                 "the Windows build-tooling chain above, but only as of where main stood "
                 "before the pipeline fixes in this list. Recorded at the time as: it lives on "
                 "branch claude/full-ui-rewrite-reconciled-20260813 and is not an ancestor of "
                 "main. CORRECTED 2026-08-16: both claims are now false. git merge-base "
                 "--is-ancestor confirms it IS an ancestor of main, so it is compiled into "
                 "every release from v7.2.97-ci.96 onward; and git ls-remote --heads shows "
                 "that branch no longer exists on the remote. What has not changed is that "
                 "none of this runtime chrome has ever been exercised: the runtime window "
                 "cannot be opened at all, because no virtual machine can start while "
                 "VBoxSup.sys is unsigned, 64-bit Windows refuses unsigned kernel drivers, and "
                 "signing is permanently prohibited for this project."),
             QStringLiteral("1e59aca274c171b85a5e293e6fe8b70a04ccce27"));
}

QList<UIMd3ChangelogEntry> UIMd3Changelog::filtered(const QString &strQuery,
                                                    const QString &strCategory,
                                                    const QString &strVersion,
                                                    const QDate &from,
                                                    const QDate &to) const
{
    QList<UIMd3ChangelogEntry> result;
    const QString strNeedle = strQuery.trimmed().left(512);
    for (const UIMd3ChangelogEntry &entry : m_entries)
    {
        if (!strCategory.isEmpty() && entry.strCategory != strCategory)
            continue;
        if (!strVersion.isEmpty() && entry.strVersion != strVersion)
            continue;
        if (from.isValid() && entry.date < from)
            continue;
        if (to.isValid() && entry.date > to)
            continue;
        const QString strHaystack = entry.strVersion + QLatin1Char(' ')
                                   + entry.strCategory + QLatin1Char(' ')
                                   + entry.strSection + QLatin1Char(' ')
                                   + entry.strTitle + QLatin1Char(' ')
                                   + entry.strDetail + QLatin1Char(' ')
                                   + entry.strCommitSha;
        if (!strNeedle.isEmpty() && !strHaystack.contains(strNeedle, Qt::CaseInsensitive))
            continue;
        result << entry;
    }
    return result;
}

QStringList UIMd3Changelog::knownVersions() const
{
    QStringList result;
    for (const UIMd3ChangelogEntry &entry : m_entries)
        if (!result.contains(entry.strVersion))
            result << entry.strVersion;
    return result;
}

QStringList UIMd3Changelog::knownCategories() const
{
    QStringList result;
    for (const UIMd3ChangelogEntry &entry : m_entries)
        if (!result.contains(entry.strCategory))
            result << entry.strCategory;
    return result;
}

QString UIMd3Changelog::shortSha(const QString &strSha)
{
    return strSha.left(7);
}

QList<UIMd3ChangelogEntry> UIMd3Changelog::visibleCentreRows() const
{
    if (!m_pSearch || !m_pVersion || !m_pCategory || !m_pFrom || !m_pTo)
        return QList<UIMd3ChangelogEntry>();

    const QString strVersion = m_pVersion->currentData().toString();
    const QString strCategory = m_pCategory->currentData().toString();
    const QDate dateMinimum(1900, 1, 1);
    const QDate from = m_pFrom->date() == dateMinimum ? QDate() : m_pFrom->date();
    const QDate to = m_pTo->date() == dateMinimum ? QDate() : m_pTo->date();

    QList<UIMd3ChangelogEntry> result;
    for (const UIMd3ChangelogEntry &entry : m_entries)
    {
        if (!strVersion.isEmpty() && entry.strVersion != strVersion)
            continue;
        if (!strCategory.isEmpty() && entry.strCategory != strCategory)
            continue;
        if (from.isValid() && entry.date < from)
            continue;
        if (to.isValid() && entry.date > to)
            continue;
        const QString strHaystack = entry.strVersion + QLatin1Char(' ')
                                   + entry.strCategory + QLatin1Char(' ')
                                   + entry.strSection + QLatin1Char(' ')
                                   + entry.strTitle + QLatin1Char(' ')
                                   + entry.strDetail + QLatin1Char(' ')
                                   + entry.strCommitSha;
        if (!m_pSearch->matches(strHaystack))
            continue;
        result << entry;
    }
    return result;
}

QString UIMd3Changelog::formatEntries(const QList<UIMd3ChangelogEntry> &rows, bool fFullDetail)
{
    QString strText;
    QString strLastVersion;
    QString strLastCategory;
    for (const UIMd3ChangelogEntry &entry : rows)
    {
        if (entry.strVersion != strLastVersion)
        {
            strText += QStringLiteral("## [%1]\n\n").arg(entry.strVersion);
            strLastVersion = entry.strVersion;
            strLastCategory.clear();
        }
        const QString strCategoryHeading = entry.strSection.isEmpty()
                                          ? entry.strCategory
                                          : QStringLiteral("%1 -- %2").arg(entry.strCategory, entry.strSection);
        if (strCategoryHeading != strLastCategory)
        {
            strText += QStringLiteral("### %1\n\n").arg(strCategoryHeading);
            strLastCategory = strCategoryHeading;
        }
        strText += QStringLiteral("- **%1** ([`%2`](%3))%4\n")
                  .arg(entry.strTitle, shortSha(entry.strCommitSha), entry.strCommitUrl,
                       entry.fOnDefaultBranch ? QString() : QStringLiteral(" _not yet on the default branch_"));
        if (fFullDetail && !entry.strDetail.isEmpty())
            strText += QStringLiteral("  %1\n").arg(entry.strDetail);
        strText += QLatin1Char('\n');
    }
    return strText;
}

void UIMd3Changelog::showCentre(QWidget *pParent)
{
    if (m_pDialog)
    {
        m_pDialog->show();
        m_pDialog->raise();
        m_pDialog->activateWindow();
        sltRefreshCentre();
        return;
    }
    if (!UIMd3Theme::instance())
        return;

    m_pDialog = new QDialog(pParent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    m_pDialog->setObjectName(QStringLiteral("md3Changelog"));
    m_pDialog->setMinimumSize(QSize(560, 460));
    QPalette palette = m_pDialog->palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    m_pDialog->setAutoFillBackground(true);
    m_pDialog->setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(m_pDialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(md3Theme().gutter() / 2);

    m_pHeading = new QLabel(m_pDialog);
    m_pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pRootLayout->addWidget(m_pHeading);

    m_pUnreleasedNotice = new QLabel(m_pDialog);
    m_pUnreleasedNotice->setWordWrap(true);
    m_pUnreleasedNotice->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    QPalette noticePalette = m_pUnreleasedNotice->palette();
    noticePalette.setColor(QPalette::WindowText, md3(UIMd3ColorRole_OnSurfaceVariant));
    m_pUnreleasedNotice->setPalette(noticePalette);
    pRootLayout->addWidget(m_pUnreleasedNotice);

    m_pSearch = new UIMd3SearchField(QStringLiteral("changelog"),
                                     md3ChangelogText("md3.changelog.search",
                                                      tr("Search changelog text, titles, or commit hashes")),
                                     m_pDialog);
    pRootLayout->addWidget(m_pSearch);

    QHBoxLayout *pFilterLayout = new QHBoxLayout;
    pFilterLayout->setContentsMargins(0, 0, 0, 0);
    pFilterLayout->setSpacing(md3Theme().gutter() / 2);
    m_pVersionLabel = new QLabel(m_pDialog);
    m_pVersion = new QComboBox(m_pDialog);
    m_pVersion->setMinimumHeight(md3Theme().controlHeight());
    m_pVersion->addItem(QString(), QString());
    for (const QString &strVersion : knownVersions())
        m_pVersion->addItem(strVersion, strVersion);
    pFilterLayout->addWidget(m_pVersionLabel);
    pFilterLayout->addWidget(m_pVersion, 1);
    m_pCategoryLabel = new QLabel(m_pDialog);
    m_pCategory = new QComboBox(m_pDialog);
    m_pCategory->setMinimumHeight(md3Theme().controlHeight());
    m_pCategory->addItem(QString(), QString());
    for (const QString &strCategory : knownCategories())
        m_pCategory->addItem(strCategory, strCategory);
    pFilterLayout->addWidget(m_pCategoryLabel);
    pFilterLayout->addWidget(m_pCategory, 1);
    pRootLayout->addLayout(pFilterLayout);

    QHBoxLayout *pDateLayout = new QHBoxLayout;
    pDateLayout->setContentsMargins(0, 0, 0, 0);
    pDateLayout->setSpacing(md3Theme().gutter() / 2);
    const QDate dateMinimum(1900, 1, 1);
    m_pFromLabel = new QLabel(m_pDialog);
    m_pFrom = new QDateEdit(dateMinimum, m_pDialog);
    m_pFrom->setCalendarPopup(true);
    m_pFrom->setMinimumDate(dateMinimum);
    m_pFrom->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    m_pToLabel = new QLabel(m_pDialog);
    m_pTo = new QDateEdit(dateMinimum, m_pDialog);
    m_pTo->setCalendarPopup(true);
    m_pTo->setMinimumDate(dateMinimum);
    m_pTo->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    pDateLayout->addWidget(m_pFromLabel);
    pDateLayout->addWidget(m_pFrom);
    pDateLayout->addWidget(m_pToLabel);
    pDateLayout->addWidget(m_pTo);
    pDateLayout->addStretch(1);
    pRootLayout->addLayout(pDateLayout);

    m_pScrollArea = new QScrollArea(m_pDialog);
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *pRowsWidget = new QWidget(m_pScrollArea);
    m_pRowsLayout = new QVBoxLayout(pRowsWidget);
    m_pRowsLayout->setContentsMargins(0, 0, 0, 0);
    m_pRowsLayout->setSpacing(md3Theme().gutter() / 2);
    m_pScrollArea->setWidget(pRowsWidget);
    pRootLayout->addWidget(m_pScrollArea, 1);

    m_pStatus = new QLabel(m_pDialog);
    m_pStatus->setAccessibleName(tr("Changelog status"));
    pRootLayout->addWidget(m_pStatus);

    QHBoxLayout *pButtonLayout = new QHBoxLayout;
    pButtonLayout->setContentsMargins(0, 0, 0, 0);
    m_pExport = new QPushButton(m_pDialog);
    m_pExport->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    m_pCopy = new QPushButton(m_pDialog);
    m_pCopy->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pButtonLayout->addWidget(m_pExport);
    pButtonLayout->addWidget(m_pCopy);
    pButtonLayout->addStretch(1);
    pRootLayout->addLayout(pButtonLayout);

    connect(m_pSearch, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3Changelog::sltRefreshCentre);
    connect(m_pVersion, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UIMd3Changelog::sltRefreshCentre);
    connect(m_pCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UIMd3Changelog::sltRefreshCentre);
    connect(m_pFrom, &QDateEdit::dateChanged,
            this, &UIMd3Changelog::sltRefreshCentre);
    connect(m_pTo, &QDateEdit::dateChanged,
            this, &UIMd3Changelog::sltRefreshCentre);
    connect(m_pExport, &QPushButton::clicked,
            this, &UIMd3Changelog::sltExportCentre);
    connect(m_pCopy, &QPushButton::clicked,
            this, &UIMd3Changelog::sltCopyCentre);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3Changelog::retranslateCentre, Qt::UniqueConnection);
    connect(m_pDialog, &QObject::destroyed, this, [this]()
    {
        m_pDialog = 0;
        m_pHeading = 0;
        m_pUnreleasedNotice = 0;
        m_pSearch = 0;
        m_pVersion = 0;
        m_pCategory = 0;
        m_pFrom = 0;
        m_pTo = 0;
        m_pVersionLabel = 0;
        m_pCategoryLabel = 0;
        m_pFromLabel = 0;
        m_pToLabel = 0;
        m_pScrollArea = 0;
        m_pRowsLayout = 0;
        m_pStatus = 0;
        m_pExport = 0;
        m_pCopy = 0;
    });

    retranslateCentre();
    m_pDialog->resize(760, 620);
    m_pDialog->show();
    m_pDialog->raise();
    m_pDialog->activateWindow();
    m_pSearch->setFocus(Qt::ShortcutFocusReason);
}

void UIMd3Changelog::sltRefreshCentre()
{
    if (!m_pRowsLayout || !m_pSearch || !m_pVersion || !m_pCategory || !m_pFrom || !m_pTo)
        return;

    while (QLayoutItem *pItem = m_pRowsLayout->takeAt(0))
    {
        if (QWidget *pWidget = pItem->widget())
            delete pWidget;
        delete pItem;
    }

    const QList<UIMd3ChangelogEntry> rows = visibleCentreRows();
    for (const UIMd3ChangelogEntry &entry : rows)
    {
        QFrame *pRow = new QFrame(m_pRowsLayout->parentWidget());
        pRow->setAccessibleName(entry.strTitle);
        pRow->setAccessibleDescription(entry.strDetail);
        pRow->setAutoFillBackground(true);
        QPalette rowPalette = pRow->palette();
        rowPalette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainerHighest));
        pRow->setPalette(rowPalette);

        QVBoxLayout *pRowLayout = new QVBoxLayout(pRow);
        pRowLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                       md3Theme().gutter(), md3Theme().gutter());
        pRowLayout->setSpacing(4);

        QLabel *pTitle = new QLabel(entry.strTitle, pRow);
        pTitle->setWordWrap(true);
        pTitle->setFont(md3Theme().font(UIMd3TypeRole_TitleMedium));
        pRowLayout->addWidget(pTitle);

        const QString strSectionSuffix = entry.strSection.isEmpty()
                                        ? QString() : QStringLiteral(" | %1").arg(entry.strSection);
        QLabel *pMetadata = new QLabel(QStringLiteral("[%1] %2%3 | %4")
                                       .arg(entry.strVersion, entry.strCategory, strSectionSuffix,
                                            QLocale().toString(entry.date, QLocale::ShortFormat)),
                                       pRow);
        pMetadata->setWordWrap(true);
        pMetadata->setFont(md3Theme().font(UIMd3TypeRole_LabelMedium));
        pMetadata->setStyleSheet(QStringLiteral("color: %1;")
                                 .arg(md3(UIMd3ColorRole_OnSurfaceVariant).name()));
        pRowLayout->addWidget(pMetadata);

        if (!entry.strDetail.isEmpty())
        {
            QLabel *pDetail = new QLabel(entry.strDetail, pRow);
            pDetail->setTextFormat(Qt::PlainText);
            pDetail->setWordWrap(true);
            pDetail->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
            pRowLayout->addWidget(pDetail);
        }

        QHBoxLayout *pFooterLayout = new QHBoxLayout;
        pFooterLayout->setContentsMargins(0, 0, 0, 0);
        pFooterLayout->setSpacing(md3Theme().gutter() / 2);
        QPushButton *pCommitButton = new QPushButton(
            tr("Commit %1").arg(shortSha(entry.strCommitSha)), pRow);
        pCommitButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
        pCommitButton->setFlat(true);
        pCommitButton->setAccessibleName(md3ChangelogText("md3.changelog.openCommit",
                                                           tr("Open commit %1 in your browser"))
                                         .arg(shortSha(entry.strCommitSha)));
        pCommitButton->setToolTip(md3ChangelogText("md3.changelog.commitTooltip",
                                                    tr("Full commit hash: %1\nOpens in your default browser."))
                                  .arg(entry.strCommitSha));
        const QString strCommitUrl = entry.strCommitUrl;
        connect(pCommitButton, &QPushButton::clicked, this, [strCommitUrl]()
        {
            QDesktopServices::openUrl(QUrl(strCommitUrl));
        });
        pFooterLayout->addWidget(pCommitButton);
        if (!entry.fOnDefaultBranch)
        {
            QLabel *pOffBranch = new QLabel(md3ChangelogText("md3.changelog.offBranch",
                                                              tr("Not yet on the default branch")), pRow);
            pOffBranch->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
            QPalette warnPalette = pOffBranch->palette();
            warnPalette.setColor(QPalette::WindowText, md3(UIMd3ColorRole_Error));
            pOffBranch->setPalette(warnPalette);
            pFooterLayout->addWidget(pOffBranch);
        }
        pFooterLayout->addStretch(1);
        pRowLayout->addLayout(pFooterLayout);

        m_pRowsLayout->addWidget(pRow);
    }

    if (rows.isEmpty())
    {
        QLabel *pEmpty = new QLabel(md3ChangelogText("md3.changelog.noMatch",
                                                      tr("No changelog entries match this filter")),
                                    m_pRowsLayout->parentWidget());
        pEmpty->setAlignment(Qt::AlignCenter);
        pEmpty->setWordWrap(true);
        pEmpty->setAccessibleName(pEmpty->text());
        pEmpty->setFont(md3Theme().font(UIMd3TypeRole_BodyLarge));
        m_pRowsLayout->addWidget(pEmpty);
    }
    m_pRowsLayout->addStretch(1);

    if (m_pStatus)
    {
        m_pStatus->setText(md3ChangelogText("md3.changelog.summary", tr("%1 of %2 entries"))
                           .arg(rows.size()).arg(m_entries.size()));
        m_pStatus->setAccessibleName(m_pStatus->text());
    }
}

void UIMd3Changelog::sltExportCentre()
{
    if (!m_pDialog)
        return;
    const QList<UIMd3ChangelogEntry> rows = visibleCentreRows();
    if (rows.isEmpty())
        return;

    const QString strDefaultPath = QDir(QStandardPaths::writableLocation(
        QStandardPaths::DocumentsLocation)).filePath(QStringLiteral("virtualbox-changelog.md"));
    const QString strPath = QFileDialog::getSaveFileName(m_pDialog,
                                                          tr("Export changelog"),
                                                          strDefaultPath,
                                                          tr("Markdown files (*.md);;Text files (*.txt)"));
    if (strPath.isEmpty())
        return;

    QSaveFile file(strPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    const QByteArray data = formatEntries(rows, true).toUtf8();
    if (file.write(data) != data.size())
        return;
    if (file.commit() && m_pStatus)
    {
        m_pStatus->setText(md3ChangelogText("md3.changelog.exported", tr("Exported %1 entries.")).arg(rows.size()));
        m_pStatus->setAccessibleName(m_pStatus->text());
    }
}

void UIMd3Changelog::sltCopyCentre()
{
    const QList<UIMd3ChangelogEntry> rows = visibleCentreRows();
    if (rows.isEmpty())
        return;
    QClipboard *pClipboard = QGuiApplication::clipboard();
    if (!pClipboard)
        return;
    pClipboard->setText(formatEntries(rows, true));
    if (m_pStatus)
    {
        m_pStatus->setText(md3ChangelogText("md3.changelog.copied", tr("Copied %1 entries to the clipboard.")).arg(rows.size()));
        m_pStatus->setAccessibleName(m_pStatus->text());
    }
}

void UIMd3Changelog::retranslateCentre()
{
    if (!m_pDialog)
        return;
    const QString strTitle = md3ChangelogText("md3.changelog.title", tr("Changelog"));
    m_pDialog->setWindowTitle(strTitle);
    m_pDialog->setAccessibleName(strTitle);
    if (m_pHeading)
    {
        m_pHeading->setText(strTitle);
        m_pHeading->setAccessibleName(strTitle);
    }
    if (m_pUnreleasedNotice)
    {
        const QString strNotice = md3ChangelogText("md3.changelog.unreleasedNotice",
                                                    tr("Every entry below is listed under the commit that made it. "
                                                       "Published releases exist for this repository; open any "
                                                       "entry's commit link to see exactly where it landed."));
        m_pUnreleasedNotice->setText(strNotice);
        m_pUnreleasedNotice->setAccessibleName(strNotice);
    }
    if (m_pSearch)
    {
        const QString strSearch = md3ChangelogText("md3.changelog.search",
                                                    tr("Search changelog text, titles, or commit hashes"));
        m_pSearch->setPlaceholderText(strSearch);
        m_pSearch->setAccessibleName(strSearch);
    }
    if (m_pVersionLabel)
        m_pVersionLabel->setText(md3ChangelogText("md3.changelog.version", tr("Version")));
    if (m_pCategoryLabel)
        m_pCategoryLabel->setText(md3ChangelogText("md3.changelog.category", tr("Category")));
    if (m_pFromLabel)
        m_pFromLabel->setText(md3ChangelogText("md3.changelog.from", tr("From")));
    if (m_pToLabel)
        m_pToLabel->setText(md3ChangelogText("md3.changelog.to", tr("To")));
    if (m_pVersion && m_pVersion->count())
        m_pVersion->setItemText(0, md3ChangelogText("md3.changelog.allVersions", tr("All versions")));
    if (m_pVersion)
        m_pVersion->setAccessibleName(md3ChangelogText("md3.changelog.version", tr("Version")));
    if (m_pCategory && m_pCategory->count())
        m_pCategory->setItemText(0, md3ChangelogText("md3.changelog.allCategories", tr("All categories")));
    if (m_pCategory)
        m_pCategory->setAccessibleName(md3ChangelogText("md3.changelog.category", tr("Category")));
    if (m_pFrom)
        m_pFrom->setSpecialValueText(md3ChangelogText("md3.changelog.anyDate", tr("Any date")));
    if (m_pFrom)
        m_pFrom->setAccessibleName(md3ChangelogText("md3.changelog.from", tr("From")));
    if (m_pTo)
        m_pTo->setSpecialValueText(md3ChangelogText("md3.changelog.anyDate", tr("Any date")));
    if (m_pTo)
        m_pTo->setAccessibleName(md3ChangelogText("md3.changelog.to", tr("To")));
    if (m_pExport)
    {
        const QString strExport = md3ChangelogText("md3.changelog.export", tr("Export Markdown"));
        m_pExport->setText(strExport);
        m_pExport->setAccessibleName(strExport);
    }
    if (m_pCopy)
    {
        const QString strCopy = md3ChangelogText("md3.changelog.copy", tr("Copy to clipboard"));
        m_pCopy->setText(strCopy);
        m_pCopy->setAccessibleName(strCopy);
    }
    sltRefreshCentre();
}
