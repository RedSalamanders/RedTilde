# Diagnostics with RedMonitor

RedTilde sends bounded diagnostics labelled RedTilde through the shared RedMonitor client header.
Install the production Release helper independently from its standalone package:
run Install-RedMonitor.ps1 in the extracted RedMonitor ZIP (CurrentUser is the default).
It installs under LocalAppData/Programs/RedMonitor; LocalMachine selects ProgramFiles/RedMonitor.

Start RedMonitor separately, or use RedTilde.exe --redmonitor. A bare Debug launch may also start the
installed Release helper asynchronously. Release apps, help, tests and probes never auto-start it.
The application keeps working when no viewer is installed or ETW permissions prevent collection.
An explicit --redmonitor request returns a failure code when the helper is absent.

Ordinary RedTilde builds compile only the pinned Client.h and verify local hashes; they never rebuild or
package the viewer. Developers update the client with Update-RedMonitorClient.ps1 -Commit FULL_SHA,
then review and commit the header, MIT notice and lock. No sibling checkout is needed.
Viewer settings belong to RedMonitor. Existing application logs, crash recovery and preferences retain
their ownership. ETW access may require Performance Log Users membership or elevation.

Unexpected SDK/application callback failures produce a local minidump and a fixed `.record` sidecar under the
profile's `Crashes` folder. These retain the original failing thread and context; the reporting worker does not
replace them. Reports may contain document text and remain local. The next ordinary launch identifies a pending
report. Expected input, capacity and native I/O refusals keep the editable document and show their ordinary status.
Healthy recovery checkpoints remain separate from fatal reporting. Keep the exact application EXE/PDB and its
pinned SDK identity with diagnostic evidence. ASan builds retain the sanitizer's independent detection evidence.
