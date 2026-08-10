# Material Virtual Machine roadmap

The rewrite is delivered in native Qt slices that preserve VirtualBox behavior
and remain independently buildable. Status is evidence-based; an implementation
row remains open until its required source, interaction, runtime, accessibility,
CI, documentation, and capture evidence exists.

## In progress

1. **Manager tools** — compose Media, Network, Extensions, Cloud, Logs,
   Activities, and related existing manager models in responsive Material
   surfaces with local search, bulk actions, export, accessibility, and truthful
   empty/error states.
2. **Theme correctness** — replace the current HSL tone approximation with a
   reviewed HCT-compatible implementation, fixed reference vectors, contrast
   checks, and light/dark/high-contrast regeneration coverage.
3. **Runtime chrome** — compose native Material controls around
   `UIMachineWindow` and `UIMachineView` without changing guest rendering,
   input capture, session state, multi-monitor ownership, or fullscreen and
   seamless semantics.
4. **Surface completion** — close remaining settings, wizard, notification,
   history, command-palette, tabs, appearance, language, export, bulk-action,
   and accessibility gaps recorded in the 69-entry design ledger.
5. **Native evidence** — build and launch the real application, capture every
   required state at supported display scales and language modes, and keep the
   captures tied to exact commits and artifacts.

## Release readiness

- Local CI-equivalent checks and all required native targets pass on the exact
  candidate commit.
- GitHub Actions validation and Pages deployment complete successfully on that
  same commit.
- A real unsigned Windows installer is built through the supported repository
  scripts and verified by path, size, commit, and SHA-256.
- One unique non-draft release contains the verified installer, complete release
  evidence, workflow timing, and required metadata.
- The default branch contains every intended change, and no secondary checkout,
  branch, or stash retains unintegrated work.

Detailed implementation authority and evidence rules are in
[`doc/md3/CodexHandoff.md`](doc/md3/CodexHandoff.md); row-level status is in
[`doc/md3/DesignCoverage.md`](doc/md3/DesignCoverage.md).
