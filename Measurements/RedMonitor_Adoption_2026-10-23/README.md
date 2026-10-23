# RedMonitor header-only adoption qualification

The client pins 9ebea6577fe9a5ced7647283c918f51f67d666fd. Normal builds verify two local files and never
restore, compile or package RedMonitor. Final-source six builds, all x64 Full noninteractive profiles,
actual Unicode/severity/PID/TID/time ETW probes, ASan detection, local offline integrity controls,
validators and formatting passed. Clean app-only ZIP extraction verifies the MIT notice and runs
Settings with system-only PATH. Earlier managed-viewer checkpoints do not qualify this boundary.

The machine-readable packet records exact source/dependency/artifact/receipt hashes and generated
evidence paths. Performance observations under concurrent build activity are diagnostic only; no paired
non-regression claim is made. Native ARM64, physical IME/AT, focus and release gates remain explicit.