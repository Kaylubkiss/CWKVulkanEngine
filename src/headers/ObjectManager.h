#pragma once
#include "ThreadPool.h"
#include "Object.h"
#include "TextureManager.h"

struct str_cmp
{
	bool operator()(const char* a, char const* b) const
	{
		return std::strcmp(a, b) < 0;
	}
};

namespace vk 
{
	typedef const char* ObjectName;
	typedef std::string TextureFileName;

	class ObjectManager
	{
	public:

		ObjectManager() = default;
		ObjectManager( GraphicsContextInfo& contextInfo );
		~ObjectManager() = default;

		//Accessors
		std::map<const char*, std::unique_ptr<Object>, str_cmp>& Objects();

		//Modifiers
		void SyncIO();
		void LoadObject( const ObjectCreateInfo& objectCI );
		void DrawObjects( VkCommandBuffer cmdBuffer,
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE );
		void Update( float dt );

	private:
		std::mutex map_mutex;
		ThreadPool m_threadWorkers;
		
		vk::Device* c_devicePtr = nullptr;

		std::map<const char*, std::unique_ptr<Object>, str_cmp> m_objects;
		std::unique_ptr<TextureManager> m_textureManager;
	};
}
