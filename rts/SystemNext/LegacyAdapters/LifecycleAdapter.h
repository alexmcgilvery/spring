/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "../Application/Lifecycle/ApplicationLifecycle.h"

class SpringApp;

namespace runtime {

/**
 * Concrete ApplicationLifecycle adapter wrapping the existing SpringApp
 * init/shutdown/reload infrastructure.
 *
 * Initialize calls SpringApp::Init() which creates the window, GL context,
 * fonts, filesystem, global structures and the initial controller.
 * CreateInitialMode returns a placeholder GameMode (the actual controller
 * is already set by Init/Startup). CreateMode handles reload by delegating
 * to SpringApp::Reload() with the correct reload script. Shutdown calls
 * SpringApp::Kill(true).
 */
class LifecycleAdapter final : public ApplicationLifecycle {
public:
	explicit LifecycleAdapter(SpringApp& app);
	~LifecycleAdapter() override;

	void Initialize() override;
	std::shared_ptr<IMode> CreateInitialMode() override;
	std::shared_ptr<IMode> CreateMode(const LifecycleRequest& request) override;
	void Shutdown() override;

private:
	SpringApp& app;
};

} // namespace runtime
