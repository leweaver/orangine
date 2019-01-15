#pragma once

#include "Manager_base.h"

namespace oe {
    class IUser_interface_manager : 
        public Manager_base,
        public Manager_windowDependent,
        public Manager_windowsMessageProcessor
    {
    public:
        explicit IUser_interface_manager(Scene& scene) : Manager_base(scene) {}

        virtual void render() = 0;
        virtual bool keyboardCaptured() = 0;
        virtual bool mouseCaptured() = 0;
    };

    class User_interface_manager : public IUser_interface_manager {
    public:
        explicit User_interface_manager(Scene& scene);

        // Manager_base implementation
        void initialize() override;
        void shutdown() override;

        // Manager_windowDependent implementation
        void createWindowSizeDependentResources(DX::DeviceResources& deviceResources, HWND window, int width, int height) override;
        void destroyWindowSizeDependentResources() override;

        // Manager_windowsMessageProcessor implementation
        bool processMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

        // IUser_interface_manager implementation
        void render() override;
        bool keyboardCaptured() override;
        bool mouseCaptured() override;

    private:
        HWND _window = nullptr;
    };
}