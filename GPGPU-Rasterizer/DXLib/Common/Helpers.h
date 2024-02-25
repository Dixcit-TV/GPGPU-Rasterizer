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

namespace StringHelpers
{
	template<typename ... Args>
	std::string StringFormat(const std::string& format, Args ... args)
	{
		int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(size_s);
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), args ...);
		return std::string(buf.get(), buf.get() + size - 1);
	}

	template<typename ... Args>
	std::string StringFormat(const std::wstring& format, Args ... args)
	{
		int size_s = _snwprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
		if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
		auto size = static_cast<size_t>(size_s);
		char* buf(new char[size]);
		_snwprintf(buf.get(), size, format.c_str(), args ...);
		std::wstring ret = std::wstring(buf.get(), buf.get() + size - 1);
		delete[] buf;

		return ret;
	}
}