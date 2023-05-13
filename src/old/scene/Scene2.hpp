#pragma once
#include <core/pipeline/egxpipeline.hpp>
#include <cmd/synchronization.hpp>
#include <core/memory/egxref.hpp>
#include <string>
#include <map>
#include <chrono>
#include <Utility/CppUtility.hpp>

namespace egx::scene
{
	class SceneData
	{
	private:
		std::map<std::string, std::vector<int8_t>> _Data;
		std::map<std::string, ref<int8_t>> _DataRefObjects;

	public:
		SceneData()
		{
			SetSceneData("StateHasChanged", false);
		}

		std::string PrintSceneData() {
			std::stringstream out;
			for (auto& [name, data] : _Data) {
				out << name << ":" << data.size() << " bytes.\n";
			}
			return out.str();
		}

		void ClearSceneData() {
			_Data.clear();
		}

		template <typename T>
		inline void SetSceneData(const std::string& identifier, const T& pData)
		{
			auto& item = _Data[identifier];
			item.resize(sizeof(T));
			memcpy(item.data(), &pData, sizeof(T));
		}

		template<>
		inline void SetSceneData<std::string_view>(const std::string& identifier, const std::string_view& pData)
		{
			auto& item = _Data[identifier];
			item.resize(pData.size() + 1);
			memcpy(item.data(), pData.data(), sizeof(char) * pData.size() + /* null termination */ 1);
		}

		template <typename T>
		inline T& GetSceneData(const std::string& identifier)
		{
			return *(T*)_Data.at(identifier).data();
		}

		template <typename T>
		inline T& GetSceneDataOrDefault(const std::string& identifier, const T& defaultValue)
		{
			if (HasSceneData(identifier)) {
				return *(T*)_Data.at(identifier).data();
			}
			else {
				SetSceneData(identifier, defaultValue);
				return *(T*)_Data.at(identifier).data();
			}
		}

		inline bool HasSceneData(const std::string& identifier)
		{
			return _Data.contains(identifier);
		}

		inline void SetSceneDataPointer(const std::string& identifier, const void* ObjPointer)
		{
			auto& item = _Data[identifier];
			item.resize(sizeof(void*));
			memcpy(item.data(), &ObjPointer, sizeof(void*));
		}

		template <typename T>
		inline T* GetSceneDataPointer(const std::string& identifier)
		{
			return *(T**)_Data.at(identifier).data();
		}

		template <typename T>
		inline T* GetSceneDataPointerOrDefault(const std::string& identifier, T* defaultValue) {
			if (_Data.contains(identifier)) {
				return _Data.at(identifier);
			}
			return defaultValue;
		}

		template <typename T>
		void SetSceneRefObject(const std::string& identifier, const ref<T>& obj) {
			_DataRefObjects[identifier] = egx::Cast<T, int8_t>(obj);
		}

		template <typename T>
		ref<T> GetSceneRefObject(const std::string& identifier) {
			auto& obj = _DataRefObjects.at(identifier);
			return egx::Cast<int8_t, T>(obj);
		}

	};

	class SceneDataInterface {
	public:
		SceneDataInterface(const std::shared_ptr<SceneData>& sceneData) : _SceneData(sceneData) {}
		SceneDataInterface() : _SceneData(std::make_shared<SceneData>()) {}

		template <typename T>
		inline void SetSceneData(const std::string& identifier, const T& pData)
		{
			_SceneData->SetSceneData<T>(identifier, pData);
		}

		template <>
		inline void SetSceneData<std::string>(const std::string& identifier, const std::string& pData)
		{
			_SceneData->SetSceneData<std::string_view>(identifier, pData);
		}

		inline void SetSceneData(const std::string& identifier, const char* pData)
		{
			_SceneData->SetSceneData<std::string_view>(identifier, pData);
		}

		template <typename T>
		inline T& GetSceneData(const std::string& identifier)
		{
			return _SceneData->GetSceneData<T>(identifier);
		}

		inline bool HasSceneData(const std::string& identifier)
		{
			return _SceneData->HasSceneData(identifier);
		}

		template <typename T>
		inline T& GetSceneDataOrDefault(const std::string& identifier, const T& defaultValue)
		{
			return _SceneData->GetSceneDataOrDefault(identifier, defaultValue);
		}

		inline void AssertHasSceneData(const std::string& identifier, const std::string& errorMsg)
		{
#ifdef _DEBUG
			if (!HasSceneData(identifier)) {
				LOG(ERR, "'{0}' not found. {1}", identifier, errorMsg);
				assert(false);
			}
#endif
		}

		inline void SetSceneDataPointer(const std::string& identifier, const void* ObjPointer)
		{
			_SceneData->SetSceneDataPointer(identifier, ObjPointer);
		}

		template <typename T>
		inline T* GetSceneDataPointer(const std::string& identifier)
		{
			return _SceneData->GetSceneDataPointer<T>(identifier);
		}

		template <typename T>
		inline T* GetSceneDataPointerOrDefault(const std::string& identifier, T* defaultValue)
		{
			return _SceneData->GetSceneDataPointerOrDefault<T>(identifier, defaultValue);
		}

		inline std::string PrintSceneData() {
			return _SceneData->PrintSceneData();
		}

		inline void ClearSceneData() {
			_SceneData->ClearSceneData();
		}

		template <typename T>
		void SetSceneRefObject(const std::string& identifier, const ref<T>& obj) {
			_SceneData->SetSceneRefObject(identifier, obj);
		}

		template <typename T>
		ref<T> GetSceneRefObject(const std::string& identifier) {
			return _SceneData->GetSceneRefObject<T>(identifier);
		}

	protected:
		std::shared_ptr<SceneData> _SceneData;
	};

	class ISceneComponent : public SceneDataInterface
	{
	protected:
		virtual void OnInitialize(bool firstLoad) = 0;
	public:
		ISceneComponent(
			const egx::ref<egx::VulkanCoreInterface>& CoreInterface,
			const egx::ref<egx::Framebuffer>& framebuffer,
			const std::shared_ptr<SceneData>& sceneData,
			uint32_t passId)
			: _CoreInterface(CoreInterface), _Framebuffer(framebuffer), _PassId(passId), SceneDataInterface(sceneData) {}

		virtual void Update(double dTime) = 0;
		virtual void Run(VkCommandBuffer cmd, double dTime) = 0;

	protected:
		virtual void EGX_API StateHasChanged();


	protected:
		egx::ref<egx::VulkanCoreInterface> _CoreInterface;
		egx::ref<egx::Pipeline> _Pipeline;
		egx::ref<egx::Framebuffer> _Framebuffer;
		uint32_t _PassId;
		friend class IScene;
	};

	class ISceneLogicComponent : public SceneDataInterface
	{
	public:
		ISceneLogicComponent(const std::shared_ptr<SceneData>& sceneData)
			: SceneDataInterface(sceneData) {}

		virtual void Update(double dTime) = 0;
	};

	class IScene : public egx::ISynchronization, public SceneDataInterface
	{
	protected:
		// On initialization when this function is called width and height is set 0
		virtual egx::ref<egx::Framebuffer> OnCreateFramebuffer(int32_t width, int32_t height) = 0;
		// Called before render pass has started
		virtual void Update() {}
		// called after render pass has started but before any components are called
		virtual void PreRender(VkCommandBuffer cmd) {}
		// called after all components are called but still inside render pass
		virtual void PostRender(VkCommandBuffer cmd) {}
		// called after post render, no longer in render pass
		virtual void AfterRender() {}
	public:
		IScene(const egx::ref<egx::VulkanCoreInterface>& CoreInterface, bool cmdStaticRecord, const std::string& sceneClassName)
			: ISynchronization(CoreInterface, sceneClassName), _CmdStaticRecord(cmdStaticRecord)
		{
			_Cmd = egx::CommandBuffer::FactoryCreate(CoreInterface);
			_CmdStaticInit.resize(CoreInterface->MaxFramesInFlight, false);
			GetOrCreateCompletionFence();
		}


		virtual void EGX_API ReInitialize();
		void CallResizeFramebuffer(int32_t width, int32_t height)
		{
			if (width == _Width && height == _Height) return;
			_Width = width, _Height = height, _ResizeFramebuffer = true;
		}

		template<class T, uint32_t ComponentPassId = 0, typename... Args>
		void AddComponent(Args&&...args)
		{
			std::shared_ptr<T> component;
			try {
				component = std::make_shared<T>(_CoreInterface, _Framebuffer, _SceneData, ComponentPassId, args...);
				if (_SceneComponentMapping.contains(typeid(T).name())) {
					LOG(ERR, "You cannot add the same component multiple times. (This is subject to change).");
					throw std::runtime_error("Duplicate components.");
				}
				_SceneComponents.push_back(component);
				_SceneComponentMapping.insert({ typeid(T).name(), (std::shared_ptr<egx::scene::ISceneComponent>)component });
			}
			catch (std::exception& e)
			{
				LOG(ERR, "Could not add component {0} because {1}", typeid(T).name(), e.what());
				return;
			}
			try {
				CallOnInitialize(component.get(), true);
				_ReadyComponents[size_t(component.get())] = true;
				_ComponentsFirstInitialized.push_back(size_t(component.get()));
			}
			catch (std::exception& e)
			{
				_ReadyComponents[size_t(component.get())] = false;
				LOG(WARNING, "Added component {0} but it will not render because OnInitialize(firstLoad: true) failed.", typeid(T).name(), e.what());
			}
		}

		template<class T, typename... Args>
		void AddLogicComponent(Args&&... args) {
			std::shared_ptr<T> component;
			try {
				component = std::make_shared<T>(_SceneData, args...);
				_SceneLogicComponents.push_back(component);
				_SceneLogicComponentMapping.insert({ typeid(T).name(), (std::shared_ptr<egx::scene::ISceneLogicComponent>)component });
			}
			catch (std::exception& e) {
				LOG(ERR, "Could not add Logic Component {} because {}", typeid(T).name(), e.what());
			}
		}

		virtual const egx::ref<egx::Framebuffer>& GetFramebuffer() { return _Framebuffer; }

		template<class T>
		std::shared_ptr<T> GetComponent()
		{
			return std::static_pointer_cast<T>(_SceneComponentMapping.at(typeid(T).name()));
		}

		template<class T>
		std::shared_ptr<T> GetLogicComponent()
		{
			return std::static_pointer_cast<T>(_SceneLogicComponentMapping.at(typeid(T).name()));
		}

	protected:
		template <class T>
		void RootInitialize()
		{
			_Framebuffer = ((T*)this)->OnCreateFramebuffer(0, 0);
		}

		static void CallOnInitialize(ISceneComponent* component, bool firstLoadFlag) {
			component->OnInitialize(firstLoadFlag);
		}

	protected:
		egx::ref<egx::Framebuffer> _Framebuffer;
		egx::ref<egx::CommandBuffer> _Cmd;
		bool _CmdStaticRecord;
		std::vector<bool> _CmdStaticInit;
		uint64_t _LastRunCount{};
		double _DTime{};
		std::vector<std::shared_ptr<ISceneComponent>> _SceneComponents;
		std::map<std::string, std::shared_ptr<ISceneComponent>> _SceneComponentMapping;
		std::vector<std::shared_ptr<ISceneLogicComponent>> _SceneLogicComponents;
		std::map<std::string, std::shared_ptr<ISceneLogicComponent>> _SceneLogicComponentMapping;
		// Declares whether the component was able to load correctly
		// if it fails to load then it is skipped in rendering.
		std::map<size_t, bool> _ReadyComponents;
		// Contains a list of all components that have been firstInitialized
		std::vector<size_t> _ComponentsFirstInitialized;

	private:
		// resize parameters
		int32_t _Width = 0;
		int32_t _Height = 0;
		bool _ResizeFramebuffer = false;
		void EGX_API InternalResizeFramebuffer();

		friend void EGX_API Run(const std::initializer_list<IScene*>& scenes);
		friend class ISceneSharedFBObject;
	};

	void EGX_API Run(const std::initializer_list<IScene*>& scenes);
}
