#include "RenderScene.h"

void FusionReactor::RenderScene::SubmitFrameCamera(const Camera const* camera) {
	m_activeCameraIndex = 1 - m_activeCameraIndex;
	camera->Clone(m_camera[m_activeCameraIndex]);
}
