# Material Virtual Machine — Material 3 GUI rewrite handoff

This package replaces the Qt frontend chrome of the VirtualBox platform with a
Material 3 shell, keeping the same language (Qt 6 + C++), the same build system
(kBuild), and the same engine contracts, so it can be dropped into an existing
checkout without a parallel frontend.

Scope agreed with the owner: whole Qt frontend; Qt Widgets with a new MD3 widget
layer; strict Material 3; seed `#6750A4`; dark default; comfortable density;
frameless title bar; Windows only; Qt6 only; preferences and per-element
appearance persisted in VirtualBox extradata; existing VirtualBox icon assets
kept and restyled; product name "Material Virtual Machine", user-renamable.

---

## 1. What is in this project

### Reviewable prototypes (open in a browser)

| File | Surface |
| --- | --- |
| `Manager.dc.html` | Manager shell: title bar, tab strip, navigation rail, chooser, Details / Snapshots / Logs / Resource Use / File Manager, and the whole global-memory layer |
| `VM Settings.dc.html` | All ten machine settings pages with live validation |
| `Runtime Window.dc.html` | Runtime window: menus, status bar, mini toolbar, visual states |
| `Wizards.dc.html` | New VM, Clone, Export, Import, Create Virtual Hard Disk |
| `Managers.dc.html` | Media, Network, Extension Pack, Cloud, Log Viewer, Activity, and eighteen preference pages |

The prototypes are the visual contract. Where the C++ and a prototype disagree
about spacing, wording or ordering, the prototype is right.

### Drop-in source (`cpp/md3/`)

Copy the whole directory to
`src/VBox/Frontends/VirtualBox/src/md3/` and add one line to
`src/VBox/Frontends/VirtualBox/Makefile.kmk`:

```
include $(PATH_SUB_CURRENT)/src/md3/Makefile.kmk
```

`cpp/md3/Makefile.kmk` appends to `VirtualBox_SOURCES`,
`VirtualBox_QT_MOCHDRS` and `VirtualBox_INCS` on the existing target, so there
is no second binary and no second moc pass.

---

## 2. Source inventory and what each file replaces

| New file | Replaces / wraps | Notes |
| --- | --- | --- |
| `UIMd3Tokens.h` | — | Colour, type, shape, elevation, state-layer and motion tokens |
| `UIMd3Theme.{h,cpp}` | `UIColorThemeEditor` behaviour | Generates the tonal palette from one seed; publishes a QPalette so unmigrated widgets stay readable |
| `UIMd3Widget.{h,cpp}` | `QIWithRetranslateUI` base usage | State layers, focus ring, appearance key, Shift+right-click into the appearance editor |
| `UIMd3Button.{h,cpp}` | `QPushButton`, `QToolButton`, `QIToolButton` | Filled / tonal / outlined / text / elevated / icon / danger |
| `UIMd3SearchField.{h,cpp}` | `UISearchLineEdit`, `UIChooserSearchWidget` input | The single search-field implementation; every search bar is one of these |
| `UIMd3RegexBuilder.{h,cpp}` | — (new global requirement) | Guided constructs, flags, sample, live preview, 300 ms budget |
| `UIMd3CommandPalette.{h,cpp}` | — | Ctrl+Shift+F, registered per surface, teleports and flashes the target element |
| `UIMd3TabStrip.h` | — | Tabs, groups, pinning, and the close-set resolver used by both close tools |
| `UIMd3TabManager.h` | — | The four independent searches and the preview-then-authorise close flow |
| `UIMd3AppearanceEditor.h` | — | Anchored per-element editor, persisted per element key |
| `UIMd3DestructiveGate.{h,cpp}` | `msgCenter().confirmRemoveMachine()` etc. | Two keys, then slider, then authorise; emergency exit is the default button |
| `UIMd3History.{h,cpp}` | — | Append-only Git-backed revisions; restore appends a new revision |
| `UIMd3NotificationCentre.h` | `UINotificationCenter` | Toast is presentation only; the centre keeps the reviewable record |
| `UIMd3Export.{h,cpp}` | — | Seventeen output formats from section/key/value rows |
| `UIMd3Language.h` | — | English / playful Cantonese / compact bilingual plus playfulness level |
| `UIMd3Style.h` | — | QProxyStyle so platform dialogs match |
| `UIMd3TitleBar.h` | `UIMiniToolBar` title handling | Frameless chrome, renamable wordmark, palette entry, bell |
| `UIMd3NavigationRail.h` | `UITools`, `UIToolsModel`, `UIToolsItem` | Same `UIToolType` destinations; rail above 1000px, drawer below |
| `UIMd3ManagerWindow.{h,cpp}` | `UIVirtualBoxManager`, `UIVirtualBoxWidget` | Keeps `UIActionPool`, `UIChooser`, `UIToolPane` and the manager signals |
| `UIMd3SettingsDialog.h` | `UIAdvancedSettingsDialog`, `UIAdvancedSettingsDialogSpecific` | Hosts existing `UISettingsPage` subclasses unchanged |
| `UIMd3RuntimeWindow.h` | `UIMachineWindowNormal` and friends | Chrome only; `UIMachineView` and capture logic untouched |
| `UIMd3Wizard.h` | `UINativeWizard` | Hosts existing `UINativeWizardPage` subclasses unchanged |
| `UIMd3ManagersWindow.h` | `UIMediumManager`, `UINetworkManager`, `UIExtensionPackManager`, `UICloudProfileManager`, `UIVMLogViewer`, `UIVMActivityOverview` | One window, one tab strip, existing models |

---

## 3. Integration points a developer must touch

1. **`src/main.cpp`** — after `UICommon` is up and before the first window:
   ```cpp
   UIMd3Theme::create();
   UIMd3History::create();
   UIMd3NotificationCentre::create();
   UIMd3Language::create();
   UIMd3Style::install();
   UIMd3ManagerWindow::create();     // instead of UIVirtualBoxManager::create()
   ```
   and mirror the four `destroy()` calls on shutdown, before `UICommon` goes down.

2. **`UIExtraDataManager`** — add the two accessors the theme uses:
   ```cpp
   QString md3String(const QString &strKey, const QString &strDefault = QString()) const;
   void    setMd3String(const QString &strKey, const QString &strValue);
   ```
   Both are thin wrappers over the existing global extradata get/set, scoped to
   the `GUI/Md3/` prefix. This is the only change required outside `src/md3/`.

3. **`UIToolPane`** — add `exportCurrentView(QWidget*)` and `exportRows()`,
   returning `QVector<UIMd3ExportRow>` from whatever the pane currently shows.
   `UIMd3Export` does the rest.

4. **Action pool** — no change. `UIActionPoolManager` and
   `UIActionPoolRuntime` remain the source of every menu, shortcut and
   checkable state; the Material 3 menus and rail read them.

5. **Icons** — no new artwork. The rail, details elements and status bar use the
   existing `.qrc` entries (`:/machine_details_manager_24px.png`,
   `:/snapshot_manager_24px.png`, `:/chipset_16px.png` …) exactly as
   `UIToolsItem::icon()` and `UIConverterBackendGlobal.cpp` map them today.

---

## 4. Global-memory features and where each one lives

| Feature | Implementation | Reachable from |
| --- | --- | --- |
| Browser-style tabs, groups, pinning | `UIMd3TabStrip` | Tab strip, tab manager, palette |
| Four independent tab searches | `UIMd3TabManager` | Ctrl+Shift+T |
| Close containing / not containing, with preview | `UIMd3TabStrip::resolveCloseSet()` | Tab manager |
| Command palette with element teleport | `UIMd3CommandPalette` | Ctrl+Shift+F, title bar |
| Regex builder on every search field | `UIMd3RegexBuilder` via `UIMd3SearchField` | The `.*` button on every field |
| Per-element appearance editor | `UIMd3AppearanceEditor` | Shift+right-click, ✎ affordances, palette |
| Named themes, export and import | `UIMd3Theme` | Preferences → Appearance |
| English / Cantonese / bilingual | `UIMd3Language` | Preferences → Interface |
| Notification centre | `UIMd3NotificationCentre` | Title-bar bell |
| Two-key destructive gate | `UIMd3DestructiveGate` | Every irreversible action |
| Append-only history with restore | `UIMd3History` | Ctrl+H, palette |
| Multi-format export | `UIMd3Export` | Ctrl+E, ⇩ on every pane |
| Searchable settings everywhere | `UIMd3SearchField` in every page | Every settings and manager page |

---

## 5. Known gaps, stated plainly

These are declared rather than hidden, so nobody discovers them during a build:

- `.cpp` implementations are provided for `UIMd3Theme`, `UIMd3Widget`,
  `UIMd3Button`, `UIMd3SearchField`, `UIMd3RegexBuilder`,
  `UIMd3CommandPalette`, `UIMd3DestructiveGate`, `UIMd3History`,
  `UIMd3Export` and `UIMd3ManagerWindow`. The remaining headers
  (`UIMd3TabStrip`, `UIMd3TabManager`, `UIMd3AppearanceEditor`,
  `UIMd3NotificationCentre`, `UIMd3Language`, `UIMd3Style`,
  `UIMd3TitleBar`, `UIMd3NavigationRail`, `UIMd3SettingsDialog`,
  `UIMd3RuntimeWindow`, `UIMd3Wizard`, `UIMd3ManagersWindow`) are complete,
  documented interfaces whose bodies still have to be written; the matching
  prototype defines the exact behaviour each one owes.
- The HCT tonal palette in `UIMd3Theme::tone()` is an HSL approximation. It is
  perceptually close and dependency-free; swap in the reference HCT solver if
  exact parity with Material's own tooling is required.
- `UIMd3History` shells out to `git`. If `git` is absent the revision file is
  still appended, so history keeps working without version control.
- Windows only, as agreed. Nothing in the layer is Windows-specific except the
  frameless title bar's system-move call, which is isolated in
  `UIMd3TitleBar::mousePressEvent()`.

---

## 6. Suggested order of work for the next developer

1. `UIExtraDataManager` accessors, then build `UIMd3Theme` alone and confirm
   the palette regenerates on a seed change.
2. `UIMd3TitleBar` + `UIMd3NavigationRail` + `UIMd3TabStrip`, then boot
   `UIMd3ManagerWindow` with the existing chooser and tool pane inside it.
3. The global-memory layer, in this order: search field → regex builder →
   palette → tab manager → appearance editor → history → notifications → export.
4. `UIMd3SettingsDialog`, hosting the unmodified settings pages.
5. `UIMd3Wizard`, then `UIMd3ManagersWindow`, then `UIMd3RuntimeWindow` last —
   the runtime path is the one place where a regression costs a user their VM
   session, so it moves only once everything else is proven.
