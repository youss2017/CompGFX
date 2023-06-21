#pragma once
#include <core/egx.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include <pipeline/RenderTarget.hpp>

namespace egx {

	namespace DataRegistryTable {
		constexpr uint32_t RENDER_TARGET = 0;
	}

	/// <summary>
	/// IDataRegistry managers inter-object data communication
	/// between IRenderStage(s) and IScene
	/// </summary>
	class IDataRegistry {
		using addr_type = uint64_t;
	public:
		IDataRegistry() {
			m_Data = std::make_shared<std::unordered_map<addr_type, std::vector<uint8_t>>>();
		}

		template<typename T>
		void Store(addr_type address, T value) {
			std::vector<uint8_t> data(sizeof(T));
			memcpy(data.data(), &value, sizeof(T));
			m_Data->operator[](address) = std::move(data);
		}

		template<typename T>
		T Read(addr_type address) {
			return *(T*)m_Data->at(address).data();
		}

		template<typename T>
		void StoreRef(addr_type address, T& value) {
			Store<size_t>(address, (size_t)&value);
		}

		template<typename T>
		void StorePtr(addr_type address, T* value) {
			Store<size_t>(address, (size_t)value);
		}

		template<typename T>
		T* ReadPtr(addr_type address) {
			return (T*)m_Data->at(address).data();
		}

		template<typename T>
		T& ReadRef(addr_type address) {
			return *ReadPtr<T>(address);
		}

	protected:
		std::shared_ptr<std::unordered_map<addr_type, std::vector<uint8_t>>> m_Data;
	};

	/**********************************************
	* The purpose of the IRenderStage interface is
	* to manage pipeline objects and issue commands.
	* The interface will manage the following
	* resources:
	*		1) Pipeline Stage
	*		2) Issue Render/Compute commands
	*********************************************/

	class IRenderStage {
	public:
		IRenderStage(const DeviceCtx& ctx, const IDataRegistry& registry) : m_Ctx(ctx), m_Registry(registry) {}

		virtual void Initialize(bool first_load) = 0;
		virtual void Process(vk::CommandBuffer cmd) = 0;

	protected:
		DeviceCtx m_Ctx;
		IDataRegistry m_Registry;
	};


	/**********************************************
	* The purpose of the IScene interface is to
	* simplifying the rendering process.
	* The interface will manage the following
	* resources:
	*		1) IRenderStage(s)
	*		2) Synchronization between stages/frames
	*		3) Data sharing between IRenderStage(s)
	*********************************************/

	class IScene {
	public:
		IScene(const DeviceCtx& ctx);

		virtual void Initialize(bool first_load) {
			for (auto& stage : m_Stages)
				stage->Initialize(first_load);
		}

		virtual void SetRenderTarget(const IRenderTarget& rt) {
			m_RT = rt;
			m_Registry.Store(DataRegistryTable::RENDER_TARGET, m_RT.value());
		}

		virtual void Process();

	protected:

		void AddRenderStage(const std::shared_ptr<IRenderStage>& stage) {
			m_Stages.push_back(stage);
		}

	protected:
		IDataRegistry m_Registry;
		DeviceCtx m_Ctx;
		uint32_t* m_CurrentFrame;

		std::vector<vk::CommandPool> m_CommandPools;
		std::vector<vk::CommandBuffer> m_CommandBuffers;
		std::vector<vk::Fence> m_FrameFence;
		std::vector<std::shared_ptr<IRenderStage>> m_Stages;
		std::optional<IRenderTarget> m_RT;
	};


}

