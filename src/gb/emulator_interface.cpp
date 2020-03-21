#include "system/system.hpp"
#include "system/log.hpp"
#include "system/local_file_system.hpp"
#include "gb/emulator_interface.hpp"
using namespace gb;

EmulatorInterface::EmulatorInterface(Graphics &g, const Buffer &buf): g(g) , em(buf) {

	emulationData = {
		g, NAME("Emulation data"),
		Texture::Info(gbRes, GPUFormat::RGBA8, GPUMemoryUsage::CPU_WRITE, 1, 1)
	};

	Buffer vertShader, fragShader;

	oicAssert("Couldn't find vertex shader", System::files()->read("./shaders/full_screen.vert.spv", vertShader));
	oicAssert("Couldn't find fragment shader", System::files()->read("./shaders/post_process.frag.spv", fragShader));

	pipeline = {
		g, NAME("Full screen pipeline"),
		Pipeline::Info(
			Pipeline::Flag::OPTIMIZE,
			{},
			{
				{ ShaderStage::VERTEX, vertShader },
				{ ShaderStage::FRAGMENT, fragShader }
			},
			pipelineLayout
		)
	};

	intermediate = {
		g, NAME("Intermediate render target"),
		Framebuffer::Info({ GPUFormat::RGBA8 }, DepthFormat::NONE, false)
	};

	samp = { g, NAME("Nearest sampler"), 
		Sampler::Info(SamplerMin::NEAREST, SamplerMag::NEAREST, SamplerMode::CLAMP_EDGE, 1) 
	};

	cl = { 
		g, NAME("Command list"),
		CommandList::Info(1_KiB)
	};

	auto descriptorsInfo = Descriptors::Info(
		pipelineLayout, 
		{ 
			{ 0, { samp, emulationData, TextureType::TEXTURE_2D } }
		}
	);

	descriptors = {
		g, NAME("Descriptors"),
		descriptorsInfo
	};

	cl->add(
		BeginFramebuffer(intermediate),
		BindPipeline(pipeline),
		BindDescriptors(descriptors),
		DrawInstanced(3),
		EndFramebuffer()
	);

	g.pause();
}

void EmulatorInterface::init(ViewportInfo *vp) {
	swapchain = {
		g, NAME("Swapchain"),
		Swapchain::Info{ vp, false }
	};
}

void EmulatorInterface::release(const ViewportInfo*) {
	swapchain.release();
}

void EmulatorInterface::resize(const ViewportInfo*, const Vec2u32 &size) {
	intermediate->onResize(size);
	swapchain->onResize(size);
}


void EmulatorInterface::render(const ViewportInfo*) {
	g.present(intermediate, swapchain, cl);
}

void EmulatorInterface::update(const ViewportInfo*, f64) {
	em.doFrame(emulationData->getTextureData2D<u32>());
	emulationData->flush({ Vec2u8(0, 1) });
}

void EmulatorInterface::onInputUpdate(ViewportInfo*, const InputDevice*, InputHandle, bool) {

}