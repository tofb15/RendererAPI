#pragma once
#include "D3D12TempNameRenderer.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		D3D12TempNameRenderer::D3D12TempNameRenderer(D3D12API* d3d12) : D3D12Renderer(d3d12) {
		}

		D3D12TempNameRenderer::~D3D12TempNameRenderer() {
		}

		bool D3D12TempNameRenderer::Initialize() {
			return false;
		}

		bool D3D12TempNameRenderer::InitializeCommandInterfaces() {
			return false;
		}

		bool D3D12TempNameRenderer::InitializeOutputTextures(D3D12Window* window) {
			return false;
		}

		void D3D12TempNameRenderer::ResetCommandListAndAllocator(int index) {
		}

		void D3D12TempNameRenderer::Submit(const SubmissionItem& item, Camera* camera, unsigned char layer) {
		}

		void D3D12TempNameRenderer::ClearSubmissions() {
		}

		void D3D12TempNameRenderer::Frame(Window* window, Camera* camera) {
		}

		void D3D12TempNameRenderer::Present(Window* window, GUI* gui) {
		}

		void D3D12TempNameRenderer::ClearFrame() {
		}

		void D3D12TempNameRenderer::SetLightSources(const std::vector<LightSource>& lights) {
		}

		void D3D12TempNameRenderer::SetSetting(const std::string& setting, float value) {
		}

		void D3D12TempNameRenderer::SetSetting(const std::string& setting, void* value) {
		}

		float D3D12TempNameRenderer::GetSetting(const std::string& setting) {
			return 0.0f;
		}

		bool D3D12TempNameRenderer::SaveLastFrame(const std::string& file) {
			return false;
		}
	}
}