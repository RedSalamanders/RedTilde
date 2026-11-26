#pragma once
#include <DxUi/DxUi.h>
#include <functional>
#include <optional>
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
    std::function<void(std::uint64_t, POINT)> contextMenu;
    [[nodiscard]] std::size_t GetVisibleItemCount() const noexcept override
    {
        return rows.size();
    }
    void GetVisibleItem(std::size_t index, DxUi::TreeItemData& item) const override
    {
        item = rows.at(index);
    }
    [[nodiscard]] std::optional<std::size_t> FindVisibleItemById(std::uint64_t itemId) const noexcept override
    {
        for (std::size_t i = 0u; i < rows.size(); ++i)
            if (rows[i].id == itemId)
                return i;
        return std::nullopt;
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
    void OnTreeContextMenu(std::uint64_t id, POINT screenPoint) override
    {
        if (contextMenu)
            contextMenu(id, screenPoint);
    }
};
} // namespace RedTilde
