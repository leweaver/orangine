//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "EntityManager.h"
#include "TestComponent.h"

extern void ExitGame();

using namespace DirectX;
using namespace OE;

using Microsoft::WRL::ComPtr;

Game::Game()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_scene = std::make_unique<Scene>();

    m_deviceResources->SetWindow(window, width, height);

	{
		// TODO: Change the timer settings if you want something other than the default variable timestep mode.
		// e.g. for 60 FPS fixed timestep update logic, call:
		/*
		m_timer.SetFixedTimeStep(true);
		m_timer.SetTargetElapsedSeconds(1.0 / 60);
		*/

		EntityManager& entityManager = m_scene->EntityManager();
		Entity& root1 = entityManager.Instantiate("Root 1");
		Entity& child1 = entityManager.Instantiate("Child 1", root1);
		Entity& child2 = entityManager.Instantiate("Child 2", root1);
		Entity& child3 = entityManager.Instantiate("Child 3", root1);

		// Test 2
		child1.AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.5f, 0.0f, 0.0f, 0.0f));
		child2.AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.5f, 0.0f, 0.0f));
		child3.AddComponent<TestComponent>().SetSpeed(XMVectorSet(0.0f, 0.0f, 0.5f, 0.0f));
		
		child1.SetLocalScale(DirectX::XMVectorSet(2, 1, 1, 1));
		child2.SetLocalScale(DirectX::XMVectorSet(1, 2, 1, 1));
		child3.SetLocalScale(DirectX::XMVectorSet(1, 1, 2, 1));

		child1.SetLocalPosition(DirectX::XMVectorSet(4, 0, 0, 1));
		child2.SetLocalPosition(DirectX::XMVectorSet(0, 0, 0, 1));
		child3.SetLocalPosition(DirectX::XMVectorSet(-4, 0, 0, 1));

		auto matl = std::make_shared<Material>();
		child1.AddComponent<RenderableComponent>().SetMaterial(matl);
		child2.AddComponent<RenderableComponent>().SetMaterial(matl);
		child3.AddComponent<RenderableComponent>().SetMaterial(matl);
	}

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    // TODO: Add your game logic here.
	m_scene->EntityManager().Tick(timer.GetElapsedSeconds());
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    // TODO: Add your rendering code here.
	m_scene->EntityRenderer().Render(*m_deviceResources);

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	m_scene->EntityRenderer().CreateDeviceDependentResources(*m_deviceResources);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	m_scene->EntityRenderer().CreateWindowSizeDependentResources(*m_deviceResources);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
