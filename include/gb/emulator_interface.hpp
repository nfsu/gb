#include "system/viewport_interface.hpp"
#include "utils/hash.hpp"
#include "helpers/graphics_object_ref.hpp"
#include "gb/emulator.hpp"

namespace gb {

	using namespace igx;
	using namespace oic;

	class EmulatorInterface : public ViewportInterface {

		static constexpr Vec2u16 gbRes = { 160, 144 };

		Graphics &g;

		TextureRef emulationData;

		SwapchainRef swapchain;
		FramebufferRef intermediate;

		CommandListRef cl;
		DescriptorsRef descriptors;
		SamplerRef samp;

		PipelineRef pipeline;
		PipelineLayout pipelineLayout {
			RegisterLayout(NAME("Input"), 0, SamplerType::SAMPLER_2D, 0, ShaderAccess::FRAGMENT)
		};

		Emulator em;

	public:

		EmulatorInterface(Graphics &g, const Buffer&);

		void init(ViewportInfo *vp) final override;
		void release(const ViewportInfo*) final override;
		void resize(const ViewportInfo*, const Vec2u32 &size) final override;

		void render(const ViewportInfo*) final override;
		void update(const ViewportInfo*, f64) final override;

		void onInputUpdate(ViewportInfo*, const InputDevice*, InputHandle, bool) final override;
	};

}