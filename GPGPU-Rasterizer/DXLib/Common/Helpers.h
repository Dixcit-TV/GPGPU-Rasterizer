#pragma once
namespace Helpers
{
	template<typename RESOURCE_TYPE, typename = std::enable_if_t<std::is_convertible_v<RESOURCE_TYPE, IUnknown*>>>
	void SafeRelease(RESOURCE_TYPE& pResource)
	{
		if (pResource)
		{
			pResource->Release();
			pResource = nullptr;
		}
	}

	template<typename OBJECT_TYPE, typename = std::enable_if_t<!std::is_convertible_v<OBJECT_TYPE, IUnknown*>&& std::is_pointer_v<OBJECT_TYPE>>>
	void SafeDelete(OBJECT_TYPE& pObject)
	{
		if (pObject)
		{
			delete pObject;
			pObject = nullptr;
		}
	}
};

