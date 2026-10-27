# Hosted MSVC setup followup

Status: DONE for implementation and local tooling qualification
Date: 2026-10-27

- [x] Confirm the retained MSVC 14.52 work is already integrated; preserve its original worktree.
- [x] Port explicit GUI process waiting and diagnostic retention without changing compiler or target requirements.
- [x] Add delayed-GUI success/failure and required-tool/refusal controls to the ordinary tooling entrypoint.
- [x] Pass affected tooling, source validation and changelog folding.
- [x] Verify available hosted setup evidence and record the required merge disposition; preserve external qualification limits explicitly.

This change owns CI provisioning only. Existing native, performance, physical-input and release gates retain their owners. RedTilde's separate Release keyboard-helper repair and historical qualification evidence remain in its rendering completion plan.
Local validation: all repository validators passed, including 12 delayed-GUI/refusal controls, evaluated application/archive profiles and interactive helper Release-policy counterfactuals. Format Check, PowerShell syntax and changelog fold/check passed. The existing source-bound six builds and three x64 Full receipts remain historical; this provisioning-only followup does not rebind them or close the recorded memory findings. Hosted run 40501470681 did not start because GitHub reported failed account payments or spending limits.

Delivery disposition: on 2026-10-27 the user explicitly authorized merge using the passing local validation despite GitHub refusing hosted jobs for account payment/spending restrictions. Those failures stay visible and are not passing CI evidence. Existing native-platform, performance, interactive and release holds remain open with their owning plans.
