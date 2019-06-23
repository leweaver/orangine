#pragma once

#include "OeCore/IUser_interface_manager.h"

namespace oe::internal {
    class User_interface_manager : public IUser_interface_manager {
    public:
        explicit User_interface_manager(Scene& scene);

        // Manager_base implementation
        void initialize() override;
        void shutdown() override;
        const std::string& name() const override;

        // Manager_windowDependent implementation
        void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
        void destroyWindowSizeDependentResources() override;

        // Manager_windowsMessageProcessor implementation
        bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

        // IUser_interface_manager implementation
        void render() override;
        bool keyboardCaptured() override;
        bool mouseCaptured() override;
        void setUIScale(float uiScale) override { _uiScale = uiScale; }

    private:

        static std::string _name;

        HWND _window = nullptr;
        float _uiScale = 1.0f;
    };
}