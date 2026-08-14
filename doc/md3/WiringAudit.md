# Wiring audit: is the Material 3 manager a real VirtualBox or a shell?

Commit audited: `0d9eda43cd0` on `claude/reality-audit-20260814`, from `main`. This document is
read-only investigation — no source changes were made as part of this audit. Every claim below
cites a repository-relative file and line number checked at that commit.

## Summary answer

The Material 3 rewrite is layered **on top of** the real VirtualBox manager, not beside it.
`UIVirtualBoxManager` is still a `QIMainWindow`
(`src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.h:59`), `UIVirtualBoxWidget` is
still the real widget
(`src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxWidget.h:54`), and the Material header,
navigation rail, tab strip, and command palette are additive chrome wrapped around that real
window, its real `UIActionPool`, its real `UIChooser`, and its real `UIToolPane`/`UIGlobalToolsWidget`.
Two of the four reported failures (Machines/Media/Network disabled, and the general shape of
"tools disabled") are **not defects** — they are genuine upstream VirtualBox behavior that the
rewrite reproduced faithfully, including the upstream mechanism for escaping the restriction. One
failure (Global Preferences not opening via `Ctrl+G`) is a **real, provable regression**
introduced by the rewrite, and it is systemic: the same code pattern is repeated in three places
in the VM Runtime window, so it very likely also breaks in-VM keyboard shortcuts (Host-key
combinations) on every running machine. Two failures (New VM producing no effect, and
`Ctrl+Shift+F` producing no palette) could not be explained by source-level tracing — every hop
in both call chains is intact and matches unmodified upstream wiring, so these need interactive
re-verification with real mouse/keyboard input rather than a source patch.

---

## 1. Why are Machines, Media, and Network disabled?

**Not a defect.** This is condition (a) from the task brief: a legitimate, correctly-reproduced
upstream VirtualBox condition, not the Material rewrite failing to wire a destination.

### The gating logic is genuine upstream logic, faithfully mirrored in the rail

`UIGlobalToolsWidget::isMenuToolEnabled()` is the authority the manager's command-palette entries
consult:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIGlobalToolsWidget.cpp:115-125
bool UIGlobalToolsWidget::isMenuToolEnabled(UIToolType enmType) const
{
    if (!chooser() || !gEDataManager)
        return enmType == UIToolType_Home;
    if (enmType == UIToolType_Machines && chooser()->isNavigationListEmpty())
        return false;
    if (!gEDataManager->isSettingsInExpertMode()
        && (enmType == UIToolType_Media || enmType == UIToolType_Network))
        return false;
    return true;
}
```

The navigation rail's own enable/disable pass, `sltHandleToolMenuUpdate()`, computes the identical
`restrictedTypes` set and applies it directly to `UIMd3NavigationRail::setToolEnabled()`:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIGlobalToolsWidget.cpp:229-258
QSet<UIToolType> restrictedTypes;
if (chooser()->isNavigationListEmpty())
    restrictedTypes << UIToolType_Machines;
const bool fExpertMode = gEDataManager->isSettingsInExpertMode();
if (!fExpertMode)
    restrictedTypes << UIToolType_Media << UIToolType_Network;
...
if (m_pNavigationRail)
{
    m_pNavigationRail->setToolEnabled(UIToolType_Home, true);
    m_pNavigationRail->setToolEnabled(UIToolType_Machines, !restrictedTypes.contains(UIToolType_Machines));
    ...
    m_pNavigationRail->setToolEnabled(UIToolType_Media, !restrictedTypes.contains(UIToolType_Media));
    m_pNavigationRail->setToolEnabled(UIToolType_Network, !restrictedTypes.contains(UIToolType_Network));
```

This is the code the pixel-luminance test observed. Both rules pre-date the Material rewrite —
`git log` on this file shows them added by the upstream `bugref:10814` change series
(`58d37ef0450` "we are not going to show it if there are no VMs", `b901cdb8cde` "Experience mode
should be taken into account"), long before any `md3` commit. The rewrite did not invent this
restriction; it reproduced it correctly in the new rail.

### Why the test machine hits both rules at once

- **Machines**: `chooser()->isNavigationListEmpty()` is true because the established evidence
  confirms a working-but-empty machine registry — zero registered VMs.
- **Media / Network**: gated behind Basic/Expert mode. The default is Basic (`false`) unless a
  debug build or an explicit choice:

```cpp
// src/VBox/Frontends/VirtualBox/src/extradata/UIExtraDataManager.cpp:739-749
bool UIExtraDataManager::isSettingsInExpertMode()
{
#ifdef DEBUG
    if (extraDataString(GUI_Settings_ExpertMode).isEmpty())
        return true;
#endif
    return isFeatureAllowed(GUI_Settings_ExpertMode);
}
```

  and the one-time auto-decision at startup only flips to Expert mode when VMs **already exist**:

```cpp
// src/VBox/Frontends/VirtualBox/src/extradata/UIExtraDataManager.cpp:3314-3324
void UIExtraDataManager::prepareExtraDataSettings()
{
    if (extraDataString(UIExtraDataDefs::GUI_Settings_ExpertMode).isNull())
    {
        if (!gpGlobalSession->virtualBox().GetMachines().isEmpty())
            setSettingsInExpertMode(true);
    }
}
```

  With zero machines at first launch, `GUI_Settings_ExpertMode` stays unset, so Basic mode is in
  effect, so Media and Network stay disabled — again, exactly the upstream design, not a rewrite bug.

### The disablement is honestly communicated, exactly per the audit's hard rule

`UIMd3NavigationRail` does not merely gray the buttons out; it explains *why*, per-tool, through a
real accessible/tooltip string:

```cpp
// src/VBox/Frontends/VirtualBox/src/md3/UIMd3NavigationRail.cpp:418-433
QString UIMd3NavigationRail::disabledReason(UIToolType enmType) const
{
    switch (enmType)
    {
        case UIToolType_Machines:
            return md3RailText("md3.manager.disabled.machines", tr("No virtual machine is available."));
        case UIToolType_Media:
        case UIToolType_Network:
            return md3RailText("md3.manager.disabled.expert", tr("Expert mode is required."));
        ...
```

This text is applied to `toolTip()`, `accessibleDescription()`, and `statusTip()` at
`UIMd3NavigationRail.cpp:439-442`.

### The escape hatch is real and does not require Global Preferences

The Home screen (`UIHomePane`, completely unmodified upstream code — see §2) shows an explicit
"Please choose Experience Mode!" panel with **Basic Mode** / **Expert Mode** buttons whenever
`GUI_Settings_ExpertMode` is undecided:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIHomePane.cpp:186-190
if (gEDataManager->extraDataString(UIExtraDataDefs::GUI_Settings_ExpertMode).isNull())
{
    m_pLabelMode = new QIRichTextLabel(this);
    ...
```

Clicking either button calls `gEDataManager->setSettingsInExpertMode(fExpertMode)` directly
(`UIHomePane.cpp:157`), with no dependency on Global Preferences opening at all. So even with the
`Ctrl+G` regression below, a user is not actually trapped — the Home screen offers the same choice.

**Conclusion:** no implementation lane should "fix" the Machines/Media/Network disabling; doing so
would violate the hard rule against faking a success path. The one real gap here is indirect —
see §4, since a broken `Ctrl+G` makes the *other* place this toggle lives (Global Preferences,
`GlobalSettingsPageType_General` → the Basic/Expert switch persisted at
`UIAdvancedSettingsDialog.cpp:1528`) harder to reach by keyboard.

---

## 2. Does the Material shell drive the real VirtualBox models, or a parallel UI?

**It drives the real models.** Confirmed by class inheritance and by tracing every level of the
stack back to genuine upstream classes:

| Class | Declaration | Base class |
| --- | --- | --- |
| `UIVirtualBoxManager` | `src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.h:59` | `QIMainWindow` (real upstream manager window) |
| `UIVirtualBoxWidget` | `src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxWidget.h:54` | `QWidget`, owns the real `UIChooser`, `UIGlobalToolsWidget`, `UIMachineToolsWidget` |
| `UIGlobalToolsWidget` | `src/VBox/Frontends/VirtualBox/src/manager/UIGlobalToolsWidget.h:56` | `QWidget`, owns the real `UIToolPane`/`UITools` |

`UIVirtualBoxManager::prepareWidgets()` wraps the *existing* `UIVirtualBoxWidget` in a new header,
it does not replace it:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp:2653-2676
QWidget *pCentral = new QWidget(this);
QVBoxLayout *pCentralLayout = new QVBoxLayout(pCentral);
...
pCentralLayout->addWidget(new UIMd3ManagerHeader(this, pCentral));
m_pWidget = new UIVirtualBoxWidget(this);   // the real, unmodified manager widget
...
pCentralLayout->addWidget(m_pWidget, 1);
setCentralWidget(pCentral);
```

The action pool is the real `UIActionPool`, created once
(`UIVirtualBoxManager.cpp:2610`: `m_pActionPool = UIActionPool::create(UIType_ManagerUI);`), and
every Material control that performs a VM action ultimately calls into it. The navigation rail's
own destination commands go through `UIGlobalToolsWidget::isMenuToolEnabled()`/`setMenuToolType()`
(`UIVirtualBoxWidget.cpp:325-329`, `283-287`), which are the real tool-pane switch primitives, not
new state. The Home screen (`UIHomePane`) is instantiated unmodified by the real `UIToolPane`:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIToolPane.cpp:140-152
case UIToolType_Home:
{
    m_pPaneHome = new UIHomePane;
    ...
    connect(m_pPaneHome, &UIHomePane::sigHomeTask, this, &UIToolPane::sigHomeTask);
    m_pLayout->addWidget(m_pPaneHome);
```

The command palette does not reimplement action policy; it registers passthrough entries that
call the real `UIAction::trigger()` and report the real `isEnabled()` state:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp:2790-2804
const auto registerActionCommand = [this, ...]
    (const QString &strTitle, const QString &strId, const QString &strDisabledReason,
     const int enmActionIndex)
{
    UIAction *pAction = actionPool()->action(enmActionIndex);
    ...
    UIMd3CommandPalette::registerCommand(UIMd3Command(
        strTitle, strManagerSource,
        [pAction]() { if (pAction->isEnabled()) pAction->trigger(); }, 0, strManagerCategory, strId,
        [pAction]() { return pAction->isEnabled(); }, strDisabledReason));
};
```

No `QProcess`, no shell-out, no second in-memory model of the machine list was found anywhere in
`src/VBox/Frontends/VirtualBox/src/md3/`, matching the established evidence. This audit additionally
confirms the *positive* claim: not only does the rewrite avoid a fake backend, it actively routes
every destination through the same `UIActionPool`/`UIToolPane`/`UIChooser` objects upstream
VirtualBox has always used.

---

## 3. Why does New VM not open a wizard?

**Traced end-to-end; the wiring is intact and matches unmodified upstream code.** No source-level
defect was found. This needs interactive re-verification against the real built app, not a blind
source patch — inventing a fix here without a reproduced root cause would risk exactly the kind of
fake success path the audit brief forbids.

The chain, fully cited:

1. Home screen link `"Create a new virtual machine (VM)"` → `#create#` anchor
   (`UIHomePane.cpp:88`) → `UIHomePane::sltHandleLinkActivated()` maps it to `HomeTask_Create`
   (`UIHomePane.cpp:123,131`) → emits `sigHomeTask` (`UIHomePane.h:56`).
2. Toolbar "New" action is also present: on the Home tool, `UIVirtualBoxWidget::updateToolbar()`
   adds `UIActionIndexMN_M_Home_S_New` to the visible toolbar
   (`src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxWidget.cpp:724`).
3. Both routes reach `UIVirtualBoxManager`: the Home-pane signal chain is
   `UIHomePane::sigHomeTask` → `UIToolPane::sigHomeTask` (`UIToolPane.cpp:148`) →
   `UIVirtualBoxWidget::sigHomeTask` (`UIVirtualBoxWidget.cpp:646-647`) →
   `UIVirtualBoxManager::sltExecuteHomeTask` (`UIVirtualBoxManager.cpp:2870-2871`); the toolbar
   action is a direct `connect(...UIActionIndexMN_M_Home_S_New..., triggered, sltOpenNewMachineWizard)`
   at `UIVirtualBoxManager.cpp:2925`.
4. `sltExecuteHomeTask(HomeTask_Create)` calls `sltOpenNewMachineWizard()`
   (`UIVirtualBoxManager.cpp:1074-1075`), which calls `openNewMachineWizard()`
   (`UIVirtualBoxManager.cpp:1453-1462`), which calls `sltOpenWizard(WizardType_NewVM)`
   (`UIVirtualBoxManager.cpp:3313-3322`).
5. `sltOpenWizard()` constructs a real `UIWizardNewVM` (`UIVirtualBoxManager.cpp:1357-1359`), then
   unconditionally shows it: `.show()`, `.setWindowState(...)`, `.activateWindow()`, `.raise()`
   (`UIVirtualBoxManager.cpp:1422-1425`).
6. `UINativeWizard::show()` is a thin, unmodified override — `init()` then `QDialog::show()`
   (`src/VBox/Frontends/VirtualBox/src/wizards/UINativeWizard.cpp:198-205`); the wizard is a normal
   `QDialog(pParent, Qt::Window)` (`UINativeWizard.cpp:151`).
7. The enable-state that gates the action is also unconditionally true regardless of the (empty)
   VM list: `case UIActionIndexMN_M_Home_S_New: ... return !isGroupSavingInProgress();`
   (`UIVirtualBoxManager.cpp:4172-4184`).

Every hop above is either pre-existing upstream code (`UIHomePane`, `UINativeWizard::show()`,
`UIWizardNewVM`) or a straightforward, unremarkable `connect()` added by the rewrite. The
Material-specific piece — `UIMd3Wizard`, the shell wrapped around the wizard's real page stack —
is documented (`doc/md3/WizardShell.md`) as presentation-only and was never interactively tested by
the prior effort (its own verification notes say "Native wizard captures remain blocked... design
previews are not substituted for application evidence" — see `WizardShell.md` "Verification"
section), so it is the least-verified link in this chain and the most likely place a real,
runtime-only defect (a crash inside `UIMd3Wizard` construction, a layout exception, etc.) could hide
without showing up in a source read. Recommend: build once, click New VM with a real mouse click
(not a synthetic key/coordinate injector), and if it still fails, attach a debugger or add one
temporary log line inside `UIWizardNewVM`'s constructor and `UINativeWizard::prepare()`
(`UINativeWizard.cpp:538`) to see how far construction gets before diagnosing further.

---

## 4. Why does Ctrl+Shift+F not show the palette, and why doesn't Global Preferences open?

These have two different answers.

### 4a. Global Preferences via Ctrl+G — real, proven regression

`Ctrl+G` is the genuine upstream default shortcut for Preferences:

```cpp
// src/VBox/Frontends/VirtualBox/src/globals/UIActionPool.cpp:1144-1152 (UIActionSimplePreferences)
virtual QKeySequence defaultShortcut(UIType enmActionPoolType) const RT_OVERRIDE
{
    switch (enmActionPoolType)
    {
        case UIType_ManagerUI: return QKeySequence("Ctrl+G");
        ...
```

This shortcut is a property of the `QAction` returned by `actionPool()->action(...)`, and that
`QAction`'s only home is the `QMenuBar`'s "File" menu
(`src/VBox/Frontends/VirtualBox/src/globals/UIActionPoolManager.cpp:4289`:
`pMenu->addAction(action(UIActionIndex_M_Application_S_Preferences));` under `#else /* !VBOX_WS_MAC */`).
The Material rewrite hides that entire menu bar on non-macOS platforms:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp:2638-2644
menuBar()->setContextMenuPolicy(Qt::CustomContextMenu);
#ifndef VBOX_WS_MAC
    /* Keep the action-backed menu model, but do not stack legacy chrome above
     * the frameless Material header.  The header exposes these menus on demand. */
    menuBar()->hide();
#endif
```

Qt resolves an action's default `Qt::WindowShortcut` context by walking up from the widget the
action was added to, looking for a visible top-level ancestor; an action whose *only* registration
lives on a hidden `QMenuBar` has no visible owner for the shortcut grab, so it stops firing. This
is a well-known Qt gotcha with hidden menu bars/toolbars, and the rewrite's own commit shows the
author was aware some workaround was needed — but only applied it to the *two new* shortcuts it
introduced itself:

```cpp
// src/VBox/Frontends/VirtualBox/src/manager/UIVirtualBoxManager.cpp:2831-2839
QShortcut *pCommandPaletteShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), this);
pCommandPaletteShortcut->setContext(Qt::ApplicationShortcut);
...
QShortcut *pHistoryShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
pHistoryShortcut->setContext(Qt::ApplicationShortcut);
```

Both are standalone `QShortcut` objects parented directly to `this` (the always-visible
`UIVirtualBoxManager` window) with the most permissive `Qt::ApplicationShortcut` context — neither
depends on the hidden menu bar, which is exactly why they were expected to keep working. Every
*other* keyboard accelerator whose only registration point is a menu-bar `QAction` — `Ctrl+G`
Preferences included — has no equivalent workaround and is unreachable by keyboard. The
`menuBar()->hide()` line was introduced in commit `74004bd7025` ("Repair Material 3 manager shell
composition"); `git log -p -S "menuBar()->hide()"` confirms this is a rewrite-introduced line, not
inherited from upstream.

**This is systemic, not confined to the Manager.** The identical pattern — hide the menu bar,
keep it only as an off-screen action model — was applied three more times in the VM Runtime
window, all on Windows:

```cpp
// src/VBox/Frontends/VirtualBox/src/runtime/normal/UIMachineWindowNormal.cpp:128
#ifdef VBOX_WS_WIN
    menuBar()->hide();
#else
// ...:250 (inside prepareRuntimeHeader(), constructing UIMd3RuntimeHeader)
// ...:357 (inside loadSettings())
```

`UIMd3RuntimeHeader` (the runtime window's equivalent header) does copy `.shortcut()` and
`.shortcutContext()` onto proxy `QAction`s for its own popup menus
(`src/VBox/Frontends/VirtualBox/src/md3/UIMd3RuntimeHeader.cpp:277-278,333-334`), but those proxies
exist only transiently while a popup menu is open — they are not a persistent, always-registered
shortcut owner, so they do not fix the underlying registration problem. Every Host-key VM
shortcut (fullscreen toggle, close, pause, etc.) whose only QAction lives in the runtime window's
hidden menu bar is likely unreachable by keyboard on Windows, for the same reason `Ctrl+G` is
unreachable in the Manager. This was not directly observed in the established evidence (which only
tested the Manager), but it is the same code pattern with the same consequence, so it is called out
here rather than left for a separate audit to rediscover.

**The hamburger-menu mouse-click route to Preferences, by contrast, is not obviously broken.**
`UIMd3ManagerHeader::showApplicationMenu()` builds a live popup directly from
`m_pWindow->menuBar()->actions()` (`UIMd3ManagerHeader.cpp:252-270`) — these are the same bound
`QAction`s the real menu bar owns, added to a genuinely visible `QMenu` that is `exec()`'d on
screen. Hovering "File" opens the real File submenu (the *same* `QMenu` object, not a copy), whose
population is driven by `connect(m_pMenu, &UIMenu::aboutToShow, actionPool(), &UIActionPool::sltHandleMenuPrepare)`
(`src/VBox/Frontends/VirtualBox/src/globals/UIActionPool.cpp:327-328`) — a signal tied to the menu
being displayed, independent of the menu bar's own visibility. `sltOpenPreferencesDialog()`
(`UIVirtualBoxManager.cpp:1288-1310`) unconditionally creates the dialog and calls `load()`, and
`UIAdvancedSettingsDialogGlobal::load()` (`src/VBox/Frontends/VirtualBox/src/settings/UIAdvancedSettingsDialogSpecific.cpp:135-150`)
always returns `true` — there is no silent-failure path here. If the hamburger route also failed
in live testing, the Ctrl+G defect above is not sufficient to explain it, and it needs the same
interactive re-verification called out in §3.

### 4b. Ctrl+Shift+F — wiring is intact; likely a test-input artifact, not a code defect

As shown in the snippet above, the palette shortcut is a standalone `QShortcut` with
`Qt::ApplicationShortcut` context, owned by the always-visible main window — it does **not** share
the hidden-menu-bar failure mode. There is also a second, fully independent activation route: the
header's own always-visible "Search everything" pill button, wired directly to the same handler:

```cpp
// src/VBox/Frontends/VirtualBox/src/md3/UIMd3ManagerHeader.cpp:165-173
m_pPalette = new UIMd3Button(QString(), UIMd3ButtonVariant_Tonal, this);
...
connect(m_pPalette, &UIMd3Button::sigClicked, this, [pWindow]()
{
    UIMd3CommandPalette::showPalette(pWindow);
});
```

`UIMd3CommandPalette::showPalette()` itself (`src/VBox/Frontends/VirtualBox/src/md3/UIMd3CommandPalette.cpp:135-168`)
is straightforward — it positions, `show()`s, `raise()`s and `activateWindow()`s a real `QDialog`.
Given that both the keyboard route and the always-visible button route are independently, plainly
wired to working code, and given this project's own recorded operating history notes that
synthetic keyboard chords (`Ctrl+Shift+F` specifically involves a 3-key chord) delivered through
generic input-injection tooling are documented as unreliable against headless Qt windows, the most
likely explanation is a test-input delivery miss, not a broken shortcut. Recommend re-testing with
a direct click on the header's palette button before treating this as a code defect.

---

## 5. Sweep for controls that look interactive but are not wired

Audited in full: `UIMd3ManagerHeader` (every one of its 6 buttons — menu, palette, notifications,
minimize, maximize, close — connects to a real handler at `UIMd3ManagerHeader.cpp:139-214`),
`UIMd3NavigationRail` (tool buttons drive real `sigToolTypeSelected`/`sigPreferencesRequested`,
consumed at `UIGlobalToolsWidget.cpp:649,654`), the Home-tool toolbar
(`UIVirtualBoxWidget.cpp:722-730`, all real `UIActionPool` actions), and the New VM / Preferences
wizard chains in §3–4. No decorative or dead control was found in any of these. `UIMd3Button`
itself (`src/VBox/Frontends/VirtualBox/src/md3/UIMd3Button.cpp`) is a real custom-painted
`mouseReleaseEvent`-driven control, not a static label — clicks are gated correctly on
`rect().contains(pos)`, `isEnabled()`, and `m_fActivationEnabled` before emitting `sigClicked()`
(`UIMd3Button.cpp:244-250`).

**Not audited in this pass**, for lack of remaining time budget, and worth a dedicated follow-up
sweep before anyone assumes they are clean: `UIMd3TabStrip.cpp`/`UIMd3TabManager.cpp` (tab
reordering, pinning, grouping — the largest and most complex md3 files by `connect()` count: 30
and 24 respectively), `UIMd3AppearanceEditor.cpp`, `UIMd3RegexBuilder.cpp`,
`UIMd3NotificationCentre.cpp`, `UIMd3History.cpp`, `UIMd3RuntimeHeader.cpp` beyond the proxy-action
shortcut point already covered above, and `UIMd3ManagerToolSearch.cpp`. None of these showed any
`TODO`/`FIXME`/"not implemented" marker text (a repository-wide grep for those terms across
`src/VBox/Frontends/VirtualBox/src/md3/*.cpp` returned no matches outside placeholder-text UI
strings), which is a mild positive signal but not proof of correctness.

---

## Implementation clusters

See the structured output accompanying this document for the disjoint file-ownership breakdown.
In summary:

1. **Manager keyboard-shortcut recovery** (`UIVirtualBoxManager.cpp`/`.h`) — register every
   action-pool shortcut against the always-visible main window (or another always-visible owner)
   so hiding the legacy menu bar stops silently disabling `Ctrl+G` and any other menu-bound
   accelerator. Use this same lane's build to interactively re-verify New VM and the
   hamburger-menu Preferences route once shortcuts are restored, since both are believed correct
   from source but need a real click to confirm.
2. **Runtime window keyboard-shortcut recovery** (`UIMachineWindowNormal.cpp`/`.h`) — the
   identical defect, reproduced independently three times in the VM Runtime window's Windows-only
   code path; almost certainly breaks Host-key shortcuts for every running VM the same way `Ctrl+G`
   is broken in the Manager.

No cluster is proposed to "fix" Machines/Media/Network disabling, because it is not broken.
