#pragma once
#include <DxUi/DxUi.h>
#include <functional>
#include <vector>

namespace RedTilde
{
// Application-owned rows adapt document headings and local paths to the shared
// Tree. Input, scrolling, rendering and UI Automation remain DxUi responsibilities.
class NavigationModel final : public DxUi::ITreeModel, public DxUi::ITreeDelegate
{
  public:
    std::vector<DxUi::TreeItemData> rows;
    std::function<void(std::uint64_t)> invoked;
    std::function<void(std::uint64_t, bool)> expanded;
    [[nodiscard]] std::size_t GetVisibleItemCount() const noexcept override
    {
        return rows.size();
    }
    void GetVisibleItem(std::size_t index, DxUi::TreeItemData &item) const override
    {
        item = rows.at(index);
    }
    void OnTreeItemInvoked(std::uint64_t id) override
    {
        if (invoked)
            invoked(id);
    }
    void OnTreeToggleExpanded(std::uint64_t id, bool value) override
    {
        if (expanded)
            expanded(id, value);
    }
};
} // namespace RedTilde
