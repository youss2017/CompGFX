#pragma once
#include <core/egxfoundation.hpp>

namespace egx {

	class CoreEngine {

	public:

		/// <summary>
		/// Creates Vulkan ICD State and Chooses first capable Graphics Device.
		/// </summary>
		/// <param name="backBufferCount">This parameters how many frames to render in the future. This is only useful for classic graphics rendering.</param>
		/// <returns></returns>
		bool Startup(int backBufferCount);

		/// <summary>
		/// Creates PlatformWindow and SwapChain (w/ support for ImGui, VSync:on, auto-resize: on)
		/// </summary>
		/// <param name="title"></param>
		/// <param name="width"></param>
		/// <param name="height"></param>
		void LaunchWindow(const std::string& title, int width, int height);

		/// <summary>
		/// This function returns (or initializes) a Render Target from the Swap Chain back buffer.
		/// However, it does not invalidate the render target, so the user can add other buffers
		/// (such as depth buffer if needed). The user is required to Invalidate the RT (but only once.)
		/// </summary>
		/// <returns>IRenderTarget initalized from window SwapChain (but not invalidated)</returns>
		IRenderTarget& GetSwapChainRenderTarget(bool invalidate);

		/// <summary>
		/// Creates a new instance of ICommandBuffer (w/ Invalidation)
		/// </summary>
		/// <returns></returns>
		ICommandBuffer CreateCmdBuffer() const {
			return { Device };
		}

		IFramedSemaphore CreateSemaphore() const {
			return { Device };
		}

		IFramedFence CreateFence() const {
			return { Device };
		}

		uint32_t NextFrame() const {
			Device->NextFrame();
			return Device->CurrentFrame;
		}

		inline void WaitIdle() const {
			Device->Device.waitIdle();
		}

	public:
		egx::VulkanICD ICD;
		egx::DeviceCtx Device;
		egx::PhysicalDeviceAndQueueFamilyInfo PhysicalDevice;
		std::shared_ptr<egx::PlatformWindow> Window;
		egx::ISwapchainController SwapChain;
		int BackBufferCount = 0;

	private:
		int _backBufferCount = 0;
		std::optional<IRenderTarget> _swapChainRT;

	};

	class GraphicsPipelineBuilder {
	public:
		GraphicsPipelineBuilder() = default;

		GraphicsPipelineBuilder& SetVertexShader(const std::string& path);
		GraphicsPipelineBuilder& SetVertexShaderCode(const std::string& code);
		GraphicsPipelineBuilder& SetFragmentShader(const std::string& path);
		GraphicsPipelineBuilder& SetFragmentShaderCode(const std::string& code);

		void Invalidate(const CoreEngine& engine, const IRenderTarget& renderTarget, bool compileInDebugMode, PipelineSpecification pipelineSpec = {});

	public:
		Shader VertexShader;
		Shader FragmentShader;
		IGraphicsPipeline Pipeline;

	private:
		std::string _vertexShaderCode;
		std::string _fragmentShaderCode;

	};

}