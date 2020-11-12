#include "Camera.hpp"

namespace FusionReactor {

	class RenderScene {
	public:
		RenderScene() {};
		~RenderScene() {};

		void Submit();
		void SubmitFrameCamera(const Camera const* camera);

	private:
		int m_activeCameraIndex = 0;
		Camera* m_camera[2] = { nullptr };
	};
}