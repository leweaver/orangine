#pragma once

#include "OeCore/IInput_manager.h"
#include "Mouse.h"

namespace oe::internal {
    class Input_manager : public IInput_manager {
    public:	    
	    explicit Input_manager(Scene &scene);

	    // Manager_base implementation
	    void initialize() override;
	    void shutdown() override;
        const std::string& name() const override;

	    // Manager_tickable implementation
	    void tick() override;

	    // Manager_windowDependent implementation
	    void createWindowSizeDependentResources(HWND window, int width, int height) override;
	    void destroyWindowSizeDependentResources() override;

	    // Manager_windowsMessageProcessor implementation
	    bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

	    // Mouse functions
	    std::weak_ptr<Mouse_state> mouseState() const override;
	    
    private:

        static std::string _name;

        DirectX::Mouse::ButtonStateTracker _buttonStateTracker;
        std::unique_ptr<DirectX::Mouse> _mouse;
        std::shared_ptr<Mouse_state> _mouseState;
    };
}
