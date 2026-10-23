#pragma once

namespace RedTilde
{
class Application;
struct LaunchOptions;
// Explicit opt-in child of the canonical DxUi desktop lease. This is never
// registered in ordinary Core tests and never supplies its own warning/lease.
int RunDesignInteractiveProbe(Application &app, const LaunchOptions &options);
} // namespace RedTilde
