#pragma once

#include "../Window.hpp"

class D3D12Window : public Window {
public:
	D3D12Window();

	// Inherited via Window
	virtual void SetDimensions(Int2 dimensions) override;

	virtual void SetDimensions(int w, int h) override;

	virtual void SetPosition(Int2 position) override;

	virtual void SetPosition(int x, int y) override;

	virtual void SetTitle(const char * title) override;

	virtual bool Create() override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void HandleWindowEvents() override;

};