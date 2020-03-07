#include "stdafx.h"
#include "D3D12GBufferRenderer.h"
#include "..\D3D12Camera.hpp"
#include "..\D3D12Window.hpp"
#include "..\D3D12Technique.hpp"
#include "..\D3D12Mesh.hpp"
#include "..\D3D12Texture.hpp"
#include "..\D3D12VertexBuffer.hpp"

bool D3D12GBufferRenderer::Initialize() {
	return false;
}

void D3D12GBufferRenderer::Submit(SubmissionItem item, Camera* c = nullptr, unsigned char layer = 0) {
	D3D12Camera* camera = static_cast<D3D12Camera*>(c);

	Sphere sphere;
	sphere.center = item.transform.pos;
	sphere.radius = max(item.transform.scale.x, max(item.transform.scale.y, item.transform.scale.z));

	if (!camera->GetFrustum().CheckAgainstFrustum(sphere))
		return;

	unsigned short techIndex = static_cast<D3D12Technique*>(item.blueprint->techniques[0])->GetID();
	unsigned short meshIndex = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetID();

	SortingItem s;
	s.item = item;
	s.sortingIndex = 0U;

	//int i2 = sizeof(INT64);
	//int i = sizeof(s.sortingIndex);
	int meshTechindex = (techIndex - 1) * m_d3d12->GetNrMeshesCreated() + (meshIndex - 1);

	if (c != nullptr) {
		Float3 f = item.transform.pos - c->GetPosition();

		unsigned int dist = f.length() * 65;
		s.distance = UINT_MAX - min(dist, UINT_MAX);

		if (dist < m_closestMeshType_float[meshTechindex]) {
			m_closestMeshType_float[meshTechindex] = dist;
			m_closestMeshType[meshTechindex] = USHRT_MAX - (unsigned short)(dist);
		}

		if (dist < m_closestTechniqueType_float[techIndex - 1]) {
			m_closestTechniqueType_float[techIndex - 1] = dist;
			m_closestTechniqueType[techIndex - 1] = USHRT_MAX - (unsigned short)(dist);
		}
	}

	std::vector<Texture*>& textureList = s.item.blueprint->textures;

	s.distance = 0;
	if (textureList.size() > 0)
		s.textureIndex = s.item.blueprint->textures[0]->GetIndex();
	else
		s.textureIndex = 0;
	s.meshIndex = meshIndex;
	s.meshTypeDistance = m_closestMeshType_lastFrame[meshTechindex];
	s.techniqueIndex = techIndex;
	s.techniqueTypeDistance = m_closestTechniqueType_lastFrame[techIndex - 1];
	s.layer = UCHAR_MAX - layer;

	m_items.push_back(s);
}

void D3D12GBufferRenderer::ClearSubmissions() {
	m_items.clear();
}

void D3D12GBufferRenderer::Frame(Window* window, Camera* camera) {
}

void D3D12GBufferRenderer::Present(Window* window, GUI* gui = nullptr) {
}

void D3D12GBufferRenderer::ClearFrame() {
}

void D3D12GBufferRenderer::SetLightSources(const std::vector<LightSource>& lights) {
}
