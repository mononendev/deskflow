---
id: des-wjpr
status: in_progress
deps: []
links: []
created: 2026-11-09T16:40:40Z
type: feature
priority: 2
assignee: Mononen
---
# Share clipboard images with PNG

Add compressed PNG clipboard transfer and native platform support.


## Notes

**2026-11-09T16:40:47Z**

Step 1 modified. Build verification is pending: cmake, ninja, and Qt 6 are absent from PATH and pkg-config.

**2026-11-09T17:01:34Z**

Linux warnings-as-errors build passes. Image conversion tests pass. Portal source compiles with upstream libportal headers. macOS and Windows builds and end-to-end tests require those platforms.

**2026-11-09T17:01:58Z**

Final Linux build passes with warnings treated as errors; all 33 tests pass. macOS and Windows source builds and the bidirectional desktop paste scenarios remain unverified because no machines or configured SSH hosts are available. Portal source passed a warnings-as-errors syntax check against upstream headers, but runtime requires a clipboard-capable portal.
