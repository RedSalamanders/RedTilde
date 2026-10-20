# Application and native UI

Read the root guidance, Editor contract and UI design system. The authoritative RichTextEditor remains stable
for the session; changing views must not call SetText on it. Return focus and preserve source selection after
formatting/menu actions. High contrast overrides custom themes. Use resource strings for chrome and the app
harness for actual captures. Do not add desktop automation, a web editor or a second presentation engine.
