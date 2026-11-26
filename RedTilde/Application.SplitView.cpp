#include "Application.h"
#include "res/Resource.h"
#include <algorithm>
#include <utility>

namespace RedTilde
{
namespace
{
void ClearEditorCallbacks(Editor* target)
{
    if (! target)
        return;
    target->SetOnTextChanged({});
    target->SetOnSelectionChanged({});
    target->SetBitmapPasteHandler({});
    target->SetOnContextMenu({});
    target->SetOnLinkActivated({});
}

bool ExchangeEditorsWithoutCallbacks(Editor& from, Editor& to)
{
    ClearEditorCallbacks(&from);
    ClearEditorCallbacks(&to);
    DxUi::RichText::Document staging;
    to.SetReadOnly(false);
    if (! from.ExchangeDocumentState(staging))
    {
        to.SetReadOnly(true);
        return false;
    }
    if (! to.ExchangeDocumentState(staging))
    {
        (void)from.ExchangeDocumentState(staging);
        to.SetReadOnly(true);
        return false;
    }
    from.SetReadOnly(true);
    return true;
}
} // namespace

Editor* Application::ActiveEditor() const noexcept
{
    return _settings.view == EditorView::Split && _splitActivePane == SplitActivePane::Source && _mirror ? _mirror : _editor;
}

Editor* Application::MirrorEditor() const noexcept
{
    return _settings.view == EditorView::Split && _splitActivePane == SplitActivePane::Source && _editor ? _editor : _mirror;
}

bool Application::SynchronizeMirror()
{
    if (IsClosing())
        return false;
    if (_settings.view != EditorView::Split)
        return true;
    if (! _root || _host.GetRoot() != _root)
        return false;
    auto* const primary = ActiveEditor();
    auto* const target  = MirrorEditor();
    auto* const owner   = _root;
    const auto source   = primary->Snapshot();
    if (_derivedCaches.mirroredSource && _derivedCaches.mirroredSource->IsSameRevision(source))
        return true;
    const auto selection       = primary->GetSelection();
    const auto revision        = primary->Revision();
    const auto primaryLifetime = GetControlLifetimeToken(*primary);
    const auto targetLifetime  = GetControlLifetimeToken(*target);
    const auto ownerLifetime   = GetControlLifetimeToken(*owner);
    const auto current         = [&]
    {
        if (IsClosing() || primaryLifetime.expired() || targetLifetime.expired() || ownerLifetime.expired() || ActiveEditor() != primary ||
            MirrorEditor() != target || _root != owner || _host.GetRoot() != owner || _settings.view != EditorView::Split)
            return false;
        // These app-owned controls are never reparented independently of the
        // root; its host ownership and all three weak identities were checked.
        return primary->Snapshot().IsSameRevision(source) && primary->Revision() == revision && primary->GetSelection() == selection;
    };
    if (target->SynchronizeReadOnlySourceSnapshot(source, selection))
    {
        if (! current() || ! target->Snapshot().IsSameRevision(source) || target->GetSelection() != selection)
            return false;
    }
    else
    {
        if (! current())
            return false;
        const auto savedTargetScroll = target->GetScrollInfo();
        // Retain exact SetText fallback for an incompatible mirror profile or Rich-preview mirror.
        // Own this spelling across callbacks instead of borrowing a mutable app cache.
        const auto text = source.Text();
        if (! AcceptSourceResult(target->TrySetDocumentFilePath(_session.path.native())) || ! current())
            return false;
        target->SetRemoteImagesEnabled(_settings.allowRemoteImages || _session.remoteImagesAllowed);
        if (! AcceptSourceResult(target->TrySetText(text)))
            return false;
        if (! current() || target->GetText() != text)
            return false;
        if (target->GetSelection() != selection)
        {
            if (! AcceptSourceResult(target->TrySetSelection(selection)))
                return false;
            if (! current())
                return false;
        }
        if (! _settings.splitScrollSync && savedTargetScroll)
        {
            if (target == _mirror)
                _pendingSourceScrollRestore = savedTargetScroll->offsetDip;
            else
                _pendingComposeScrollRestore = savedTargetScroll->offsetDip;
            if (_window && ! _postPreparePosted)
                _postPreparePosted = PostMessageW(_window.get(), PostPreparePlacement, 0, 0) != FALSE;
        }
        else if (_settings.splitScrollSync)
        {
            if (const auto driverScroll = primary->GetScrollInfo())
            {
                _pendingSplitScroll = PendingSplitScroll{primary, driverScroll->offsetDip};
                if (_window && ! _postPreparePosted)
                    _postPreparePosted = PostMessageW(_window.get(), PostPreparePlacement, 0, 0) != FALSE;
            }
        }
    }
    // A nested synchronization may already have published this same source.
    if (! _derivedCaches.mirroredSource || ! _derivedCaches.mirroredSource->IsSameRevision(source))
    {
        _derivedCaches.mirroredSource = source;
        ++_activity.mirrorResets;
    }
    return true;
}

void Application::BindEditorCallbacks()
{
    auto* const active  = ActiveEditor();
    auto* const passive = MirrorEditor();
    ClearEditorCallbacks(passive);
    if (active)
    {
        active->SetOnTextChanged([this] { Changed(); });
        active->SetOnSelectionChanged([this] { SelectionChanged(); });
        active->SetBitmapPasteHandler([this](std::span<const std::byte> dib, bool v5) { return BeginBitmapPaste(dib, v5); });
        active->SetOnContextMenu([this](POINT point, bool keyboard) { Menu(point, false, false, keyboard); });
        active->SetOnLinkActivated([this](const DxUi::RichTextLinkActivation& activation) { RunUiAction([&] { RequestOpenLink(activation); }); });
    }
}

bool Application::EnsurePrimaryEditorAuthoritative()
{
    if (_splitActivePane == SplitActivePane::Compose)
        return true;
    _splitScrollSyncInProgress = true;
    const auto resetSync       = wil::scope_exit([&] { _splitScrollSyncInProgress = false; });
    if (_editor && _mirror)
    {
        if (! ExchangeEditorsWithoutCallbacks(*_mirror, *_editor))
        {
            BindEditorCallbacks();
            return false;
        }
        _editor->SetReadOnly(false);
        _mirror->SetReadOnly(true);
    }
    _splitActivePane = SplitActivePane::Compose;
    BindEditorCallbacks();
    _derivedCaches.mirroredSource.reset();
    _derivedCaches.dirtyFact.reset();
    _derivedCaches.selectionFactsSource.reset();
    _pendingComposeScrollRestore.reset();
    _pendingSourceScrollRestore.reset();
    _pendingSplitScroll.reset();
    UpdateSplitHeaders();
    return true;
}

void Application::SwitchSplitActivePane(SplitActivePane target, bool moveFocus, std::optional<DxUi::RichText::Selection> overrideSelection)
{
    if (IsClosing() || ! IsUiAttached() || _settings.view != EditorView::Split || target == _splitActivePane)
        return;
    if (! CanAcceptCommand() || HasActiveModalPrompt() || _files.Busy() || _switchingDocument)
    {
        Status(IDS_BUSY);
        return;
    }
    if (HasBlockDraft())
    {
        Status(IDS_BLOCKPENDING);
        return;
    }
    DxUi::NativeTextInputState native;
    if (_host.TryReadNativeTextInputState(_host.GetFocusControl(), native) && native.compositionStartIndex)
        return;
    auto* const oldActive = ActiveEditor();
    auto* const newActive = MirrorEditor();
    if (! oldActive || ! newActive || oldActive->GetSemanticSourceRegion() || newActive->GetSemanticSourceRegion())
    {
        Status(IDS_BUSY);
        return;
    }
    const auto oldScroll       = oldActive->GetScrollInfo();
    const auto newScroll       = newActive->GetScrollInfo();
    _splitScrollSyncInProgress = true;
    const auto resetSync       = wil::scope_exit([&] { _splitScrollSyncInProgress = false; });
    if (! ExchangeEditorsWithoutCallbacks(*oldActive, *newActive))
    {
        BindEditorCallbacks();
        Status(IDS_BUSY);
        return;
    }
    _splitActivePane = target;
    ++_activity.splitPaneSwitches;
    (void)AcceptSourceResult(newActive->TrySetDocumentFilePath(_session.path.native()));
    newActive->SetRemoteImagesEnabled(_settings.allowRemoteImages || _session.remoteImagesAllowed);
    BindEditorCallbacks();
    if (overrideSelection)
    {
        const auto snap = newActive->Snapshot();
        const auto& sel = overrideSelection.value();
        if (sel.anchor <= snap.Length() && sel.caret <= snap.Length() && snap.IsBoundary(sel.anchor) && snap.IsBoundary(sel.caret))
            (void)AcceptSourceResult(newActive->TrySetSelection(sel));
    }
    _derivedCaches.mirroredSource.reset();
    _derivedCaches.dirtyFact.reset();
    _derivedCaches.selectionFactsSource.reset();
    (void)SynchronizeMirror();
    _pendingSplitScroll.reset();
    if (oldScroll)
    {
        if (oldActive == _editor)
            _pendingComposeScrollRestore = oldScroll->offsetDip;
        else
            _pendingSourceScrollRestore = oldScroll->offsetDip;
    }
    if (newScroll)
    {
        if (newActive == _editor)
            _pendingComposeScrollRestore = newScroll->offsetDip;
        else
            _pendingSourceScrollRestore = newScroll->offsetDip;
    }
    if ((_pendingComposeScrollRestore || _pendingSourceScrollRestore) && _window && ! _postPreparePosted)
        _postPreparePosted = PostMessageW(_window.get(), PostPreparePlacement, 0, 0) != FALSE;
    if (_activeOverlay == OverlayKind::FindBar)
    {
        oldActive->ClearSourceHighlights();
        _find.projections.highlightsNeedRefresh = true;
        RefreshFind();
    }
    if (_activeOverlay == OverlayKind::FloatingSelectionTools && _splitActivePane == SplitActivePane::Source)
        SetActiveOverlay(OverlayKind::None);
    (void)oldActive->TrySetDecorations(oldActive->Snapshot(), {});
    _spell.appliedKey.reset();
    _spell.issues.clear();
    _spell.suppressedTypingWord.reset();
    _spell.typingSuppressionActive = false;
    _spell.pendingMenu.reset();
    _spell.pendingLeftClick.reset();
    _spell.leftDownArmed = false;
    Layout();
    SelectionChanged();
    ScheduleSpellCheck(true);
    if (moveFocus)
        _host.SetFocusControl(newActive, MayRequestNativeFocus());
}

void Application::OnSplitPaneScrolled(Editor* sourcePane, float scrollDip)
{
    if (_splitScrollSyncInProgress || ! _settings.splitScrollSync || _settings.view != EditorView::Split || IsClosing() || ! sourcePane)
        return;
    if (! sourcePane->IsVisible())
        return;
    ApplySplitScrollSync(sourcePane, scrollDip, true);
}

void Application::ApplySplitScrollSync(Editor* sourcePane, float scrollDip, bool allowDefer)
{
    if (_splitScrollSyncInProgress || ! _settings.splitScrollSync || _settings.view != EditorView::Split || IsClosing() || ! sourcePane)
        return;
    Editor* const follower = sourcePane == _editor ? _mirror : sourcePane == _mirror ? _editor : nullptr;
    if (! follower || ! follower->IsVisible())
        return;
    const auto driverInfo   = sourcePane->GetScrollInfo();
    const auto followerInfo = follower->GetScrollInfo();
    if (! driverInfo || ! followerInfo)
    {
        if (allowDefer)
        {
            _pendingSplitScroll = PendingSplitScroll{sourcePane, scrollDip};
            if (_window && ! _postPreparePosted)
                _postPreparePosted = PostMessageW(_window.get(), PostPreparePlacement, 0, 0) != FALSE;
        }
        return;
    }
    _splitScrollSyncInProgress = true;
    const auto resetSync       = wil::scope_exit([&] { _splitScrollSyncInProgress = false; });
    const float driverMax      = std::max(0.f, driverInfo->contentHeightDip - driverInfo->viewportHeightDip);
    const float followerMax    = std::max(0.f, followerInfo->contentHeightDip - followerInfo->viewportHeightDip);
    if (scrollDip <= 1.0f || driverMax <= 1.0f)
    {
        (void)follower->SetScrollOffsetDip(0.0f);
        ++_activity.splitScrollSyncs;
        return;
    }
    if (scrollDip >= driverMax - 1.0f)
    {
        (void)follower->SetScrollOffsetDip(followerMax);
        ++_activity.splitScrollSyncs;
        return;
    }
    const auto visibleRange = sourcePane->GetVisibleSourceRange(_host);
    if (visibleRange && follower->ScrollSourceRangeIntoView(_host, visibleRange->first, visibleRange->first, true))
    {
        ++_activity.splitScrollSyncs;
        return;
    }
    if (allowDefer)
    {
        _pendingSplitScroll = PendingSplitScroll{sourcePane, scrollDip};
        if (_window && ! _postPreparePosted)
            _postPreparePosted = PostMessageW(_window.get(), PostPreparePlacement, 0, 0) != FALSE;
        return;
    }
    const float ratio = std::clamp(scrollDip / driverMax, 0.f, 1.f);
    (void)follower->SetScrollOffsetDip(ratio * followerMax);
    ++_activity.splitScrollSyncs;
}
} // namespace RedTilde
