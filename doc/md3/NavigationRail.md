# Manager navigation rail

The manager's global tool selection is presented through a Material Design 3
navigation rail in `UIGlobalToolsWidget`. The rail is a native Qt widget; it is
not a second tool model and it does not bypass `UITools`, `UIToolsModel`,
`UIToolPane`, or `UIActionPoolManager`.

## Behaviour

- Home, Machines, Media, Network, Cloud, Resources, and Extensions are exposed
  as keyboard-focusable, checkable rail buttons.
- Activating a button calls the existing `setMenuToolType` path, so selection,
  tool-pane creation, cloud refresh, and action-pool behavior stay unchanged.
- Existing expert-mode and empty-chooser restrictions are mirrored into the
  rail as disabled states. The visible label remains present so a user can tell
  which capability is unavailable.
- The active `UIToolType` is synchronized from the existing model selection,
  including changes caused by COM events or persisted startup state.

## Appearance and accessibility

The rail consumes the shared Material 3 surface, outline, primary, and state
layer roles from `UIMd3Theme`. It uses a 40 px comfortable control height (36 px
in compact density), visible focus borders, accessible names, and a 176--224 px
bounded width. Theme updates refresh the live colors and density without
replacing the underlying tool widgets.

## Failure modes and security

The rail never creates or destroys virtual machines and never performs COM
operations itself. If a tool is restricted, its button is disabled and the
existing tool pane remains the authority. A failure to construct the global
tool pane is handled by the existing VirtualBox error path; the rail is only
an alternate selector surface.

## Verification

The MD3 validation workflow checks that the rail source and MOC header are
wired into the `VirtualBox` target. Runtime verification still requires a
working `VirtualBoxClient` COM registration and a real native capture of the
rail in Home, Machines, and at least one restricted expert-mode state. Design
thumbnails and static previews are not acceptable substitutes.

Suggested articles: [Material 3 GUI overview](README.md),
[Design coverage ledger](DesignCoverage.md), and the repository
[build instructions](../README.md#build-and-prerequisites).
