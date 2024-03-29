#pragma once
#include "../egx.hpp"
#include "egxref.hpp"
#include <memory>
#include <imgui.h>
#include "frameflight.hpp"
#include "../../cmd/cmd.hpp"
#include <optional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace egx {

	enum class memorylayout : uint32_t {
		local,
		dynamic,
		stream
	};

#pragma region GPU Buffer
	enum buffertype : uint32_t {
		BufferType_Vertex       = 0b00001,
		BufferType_Index        = 0b00010,
		BufferType_Storage      = 0b00100,
		BufferType_Uniform      = 0b01000,
		BufferType_Indirect     = 0b10000,
		BufferType_TransferOnly = 0b00000
	};

	class Bufferx : public FrameFlight {

	public:
		static ref<Buffer> EGX_API FactoryCreate(
			const DeviceCtx* pCtx,
			size_t size,
			memorylayout layout,
			uint32_t type,
			bool CpuWritePerFrameFlag = false,
			bool BufferReference = false,
			bool requireCoherent = false);

		EGX_API ~Buffer();
		Buffer(Buffer& cp) = delete;
		Buffer& operator=(Buffer& cp) = delete;
		EGX_API Buffer(Buffer&& move) noexcept;
		EGX_API Buffer& operator=(Buffer&& move) noexcept;

		EGX_API ref<Buffer> Clone(bool CopyContents = true);
		EGX_API void Copy(const ref<Buffer>& source, size_t srcOffset, size_t dstOffset, size_t size);
		// Copies including other frame data
		EGX_API void CopyAll(const ref<Buffer>& source, size_t srcOffset, size_t dstOffset, size_t size);

		EGX_API void Write(const void* data, size_t offset, size_t size, bool keepMapped = false);
		EGX_API void Write(const void* data, size_t size, bool keepMapped = false);
		EGX_API void Write(const void* data, bool keepMapped = false);

		// Write data(ptr) to all Frame 1 to Frame N buffers
		EGX_API void WriteAll(const void* data, bool keepMapped = false) {
			WriteAll(data, 0, Size, keepMapped);
		}

		// Write data(ptr) to all Frame 1 to Frame N buffers
		EGX_API void WriteAll(const void* data, size_t offset, size_t size, bool keepMapped) {
			if (CpuAccessPerFrame) {
				for (uint32_t i = 0; i < _coreinterface->MaxFramesInFlight; i++) {
					SetStaticFrameIndex(i);
					Write(data, offset, size, keepMapped);
				}
				SetStaticFrameIndex();
			}
			else {
				Write(data, offset, size, keepMapped);
			}
		}

		// [WARNING]! 
		// If you are using CPUAccessFlag then every frame you must call this function
		// to get mapped pointer otherwise you will be writing/reading from the wrong
		// buffer.
		EGX_API int8_t* Map();
		EGX_API void Unmap();

		EGX_API void Read(void* pOutput, size_t offset, size_t size);
		EGX_API void Read(void* pOutput, size_t size);
		EGX_API void Read(void* pOutput);

		EGX_API void Flush();
		EGX_API void Flush(size_t offset, size_t size);
		EGX_API void Invalidate();
		EGX_API void Invalidate(size_t offset, size_t size);
		EGX_API void SetDebugName(const std::string& Name);

		EGX_API const VkBuffer& GetBuffer();
		EGX_API std::vector<size_t> GetBufferBasePointers();
		EGX_API size_t GetBufferBasePointer();

		/// <summary>
		/// Allocates a new size if the size is different,
		/// returns true if the buffer was reallocated.
		/// </summary>
		EGX_API bool Resize(size_t newSize, bool ShrinkBuffer, bool CopyOldContents);

	protected:
		EGX_API Buffer(
			size_t size,
			memorylayout layout,
			uint32_t type,
			bool cpuaccessflag,
			bool bufferreference,
			bool coherent,
			VkAlloc::CONTEXT context,
			const DeviceCtx* pCtx) :
			Size(size), Layout(layout),
			Type(type), CoherentFlag(coherent),
			_context(context), _coreinterface(coreinterface),
			CpuAccessPerFrame(cpuaccessflag), BufferReference(bufferreference)
		{
			DelayInitalizeFF(coreinterface, !CpuAccessPerFrame);
			_mapped_ptr.resize(coreinterface->MaxFramesInFlight, nullptr);
		}

	public:
		size_t Size;
		const memorylayout Layout;
		const uint32_t Type;
		const bool CoherentFlag;
		const bool CpuAccessPerFrame;
		const bool BufferReference;
	protected:
		ref<VulkanCoreInterface> _coreinterface;
		const VkAlloc::CONTEXT _context;
		std::vector<VkAlloc::BUFFER> _buffers;
		std::vector<int8_t*> _mapped_ptr;
		bool _mapped_flag = false;
		uint32_t _ResizeFlag = 0;
		std::vector<bool> _ResizeFrameFlag;
		bool _ResizeCopyOldContents = false;
		size_t _ResizeBytes = 0;
		std::vector<size_t> _BufferPointers;
	};
#pragma endregion GPU Buffer

	class Image {

	public:
		/// <summary>
		/// Creates a image or texture
		/// </summary>
		/// <param name="CoreInterface">CoreInterfance from EngineCore</param>
		/// <param name="width">Width (or size for 1D Image)</param>
		/// <param name="height">Height (If Image is 1D Set to 1)</param>
		/// <param name="depth">Depth for 3D Images (for 2D/1D set to 1)</param>
		/// <param name="format">Format of image</param>
		/// <param name="mipcount">Set to 0 for max mipmap count</param>
		/// <param name="arraylevel">For arrayed images</param>
		/// <returns></returns>
		EGX_API static ref<Image> FactoryCreateEx(
			const DeviceCtx* pCtx,
			VkImageAspectFlags aspect,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			VkFormat format,
			uint32_t mipCount,
			uint32_t arrayLevel,
			VkImageUsageFlags usage,
			VkImageLayout initialLayout,
			bool createDefaultView,
			bool streamingMode = false);

		static inline ref<Image> FactoryCreate(
			const DeviceCtx* pCtx,
			VkImageAspectFlags aspect,
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageUsageFlags usage,
			VkImageLayout initialLayout,
			bool createDefaultView,
			uint32_t mipCount = 0,
			bool streamingMode = false
		) {
			return FactoryCreateEx(CoreInterface, aspect, width, height, 1, format, mipCount, 1, usage, initialLayout, createDefaultView, streamingMode);
		}

		/// <summary>
		/// Creates an image from VkImage handle, these properties are used to clone the image,
		/// You are still responsible for destroying image handle.
		/// </summary>
		EGX_API static ref<Image> FactorCreateFromHandle(
			const DeviceCtx* pCtx,
			VkImageAspectFlags aspect,
			VkImage image,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			VkFormat format,
			VkImageUsageFlags usage,
			VkImageLayout initialLayout,
			uint32_t mipCount
		);

		EGX_API ~Image() noexcept;

		EGX_API Image(Image& cp) = delete;
		EGX_API Image(Image&& move) noexcept :
			Width(move.Width), Height(move.Height),
			Depth(move.Depth), Mipcount(move.Mipcount),
			Arraylevels(move.Arraylevels), Image_(move.Image_),
			ImageUsage(move.ImageUsage), _context(move._context),
			_image(move._image), _coreinterface(move._coreinterface),
			ImageAspect(move.ImageAspect), Format(move.Format), 
			StreamingMode(move.StreamingMode) {
			memcpy(this, &move, sizeof(Image));
			memset(&move, 0, sizeof(Image));
		}
		EGX_API Image& operator=(Image&& move) noexcept {
			if (this == &move) return *this;
			this->~Image();
			memcpy(this, &move, sizeof(Image));
			memset(&move, 0, sizeof(Image));
			return *this;
		}

		EGX_API void SetLayout(VkImageLayout OldLayout, VkImageLayout NewLayout);

		EGX_API void Write(
			const void* Data,
			VkImageLayout CurrentLayout,
			uint32_t xOffset,
			uint32_t yOffset,
			uint32_t zOffset,
			uint32_t Width,
			uint32_t Height,
			uint32_t Depth,
			uint32_t ArrayLevel,
			uint32_t StrideSizeInBytes);

		EGX_API void Write(const void* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t ArrayLevel, uint32_t StrideInBytes);
		EGX_API void Write(const void* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t Height, uint32_t ArrayLevel, uint32_t StrideInBytes);
		EGX_API void Write(const void* Data, VkImageLayout CurrentLayout, uint32_t Width, uint32_t ArrayLevel, uint32_t StrideInBytes);

		EGX_API void GenerateMipmap(VkImageLayout CurrentLayout, uint32_t ArrayLevel = 0);

		EGX_API VkImageView CreateView(
			uint32_t ViewId,
			uint32_t Miplevel,
			uint32_t MipCount,
			uint32_t ArrayLevel,
			uint32_t ArrayCount,
			VkComponentMapping RGBASwizzle = {});

		EGX_API VkImageView CreateView(uint32_t ViewId, VkComponentMapping RGBASwizzle = {});

		inline const VkImageView View(uint32_t ViewId) noexcept {
			assert(_views.find(ViewId) != _views.end());
			return _views[ViewId];
		}

		static egx::ref<egx::Image> EGX_API CreateCubemap(const DeviceCtx* pCtx, std::string_view path, VkFormat format);
		static ref<Image> EGX_API LoadFromDisk(const DeviceCtx* pCtx, std::string_view path, VkImageUsageFlags usage, VkImageLayout InitalLayout,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		void EGX_API Barrier(VkCommandBuffer cmd, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			VkAccessFlags srcAccess, VkAccessFlags dstAccess,
			uint32_t miplevel = 0,
			uint32_t arraylevel = 0,
			uint32_t mipcount = VK_REMAINING_MIP_LEVELS,
			uint32_t arraycount = VK_REMAINING_ARRAY_LAYERS) const;

		VkImageMemoryBarrier EGX_API Barrier(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess,
			uint32_t miplevel = 0,
			uint32_t arraylevel = 0,
			uint32_t mipcount = VK_REMAINING_MIP_LEVELS,
			uint32_t arraycount = VK_REMAINING_ARRAY_LAYERS) const;

		inline static VkImageMemoryBarrier Barrier(
			VkImage Img, 
			VkImageAspectFlags ImageAspect, 
			VkImageLayout oldLayout, 
			VkImageLayout newLayout, 
			VkAccessFlags srcAccess, 
			VkAccessFlags dstAccess,
			uint32_t miplevel = 0,
			uint32_t arraylevel = 0,
			uint32_t mipcount = VK_REMAINING_MIP_LEVELS,
			uint32_t arraycount = VK_REMAINING_ARRAY_LAYERS) {
			VkImageMemoryBarrier barr{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barr.srcAccessMask = srcAccess;
			barr.dstAccessMask = dstAccess;
			barr.oldLayout = oldLayout;
			barr.newLayout = newLayout;
			barr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barr.image = Img;
			barr.subresourceRange.aspectMask = ImageAspect;
			barr.subresourceRange.baseMipLevel = miplevel;
			barr.subresourceRange.levelCount = mipcount;
			barr.subresourceRange.baseArrayLayer = arraylevel;
			barr.subresourceRange.layerCount = arraycount;
			return barr;
		}

		// currentImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR
		void EGX_API Read(void* buffer, VkImageLayout currentImageLayout, uint32_t mipLevel, VkOffset3D offset, VkExtent3D size);
		// currentImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR
		egx::ref<egx::Buffer> EGX_API Read(VkImageLayout currentImageLayout, uint32_t mipLevel, VkOffset3D offset, VkExtent3D size);

		/// <summary>
		/// Clones image with the same properties and same content.
		/// </summary>
		/// <param name="CurrentLayout">The current layout of the source image.</param>
		/// <param name="NewCopyFinalLayout">The final layout of new copied image, if undefinied it will be same as 'CurrentLayout'</param>
		/// <returns></returns>
		ref<Image> EGX_API copy(VkImageLayout CurrentLayout, VkImageLayout CopyFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED) const;
		// Same properties
		ref<Image> EGX_API clone() const;

		EGX_API ImTextureID GetImGuiTextureID(VkSampler sampler, uint32_t viewId = 0);
		
		// cmd can be null, if it is null then the clear will be blocking and immediate
		EGX_API void ClearImage(VkCommandBuffer cmd, const VkClearValue& clearValue);
		EGX_API void ClearImage(const VkClearValue& clearValue);

		EGX_API void SetDebugName(const std::string& Name);

		inline std::tuple<uint32_t, uint32_t, uint32_t> GetSize3D() const { return { Width, Height, Depth }; };
		inline std::pair<uint32_t, uint32_t> GetSize2D() const { return { Width, Height }; };
		inline glm::ivec2 GetSize2DGlm() const { return { Width, Height }; };
		inline glm::ivec3 GetSize3DGlm() const { return { Width, Height, Depth }; };

	protected:
		EGX_API Image(uint32_t width, uint32_t height,
			uint32_t depth, uint32_t mipcount, uint32_t arraylevels,
			VkImage image, VkImageUsageFlags usage,
			VkAlloc::CONTEXT _context, VkAlloc::IMAGE _image,
			const ref<VulkanCoreInterface>& _interface, VkFormat format,
			VkImageAspectFlags aspect, VkImageLayout initalLayout,
			bool streamingMode)
			: Width(width), Height(height),
			Depth(depth), Mipcount(mipcount),
			Arraylevels(arraylevels), Image_(image),
			ImageUsage(usage), _context(_context),
			_image(_image), _coreinterface(_interface),
			Format(format), ImageAspect(aspect),
			_imageusage(usage),
			_initial_layout(initalLayout),
			StreamingMode(streamingMode)
		{}

	public:
		const uint32_t Width;
		const uint32_t Height;
		const uint32_t Depth;
		const uint32_t Mipcount;
		const uint32_t Arraylevels;
		const VkImageUsageFlags ImageUsage;
		const VkFormat Format;
		const VkImageAspectFlags ImageAspect;
		const VkImage Image_;
		const bool StreamingMode;

	protected:
		const VkAlloc::CONTEXT _context;
		const VkAlloc::IMAGE _image;
		ref<VulkanCoreInterface> _coreinterface;
		std::map<uint32_t, VkImageView> _views;
		ImTextureID _imgui_textureid{};
		VkImageUsageFlags _imageusage{};
		VkImageLayout _initial_layout{};
		bool _call_destruction = true;
		ref<Buffer> _StreamingBuffer;
		CommandBuffer _StreamingCmd;
		ref<Fence> _StreamingFence;
	};

}