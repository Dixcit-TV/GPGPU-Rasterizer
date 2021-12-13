#pragma once
#include <string>
#include <cstdint>

class Window
{
public:
	explicit Window(const wchar_t* name, uint16_t width, uint16_t height);
	~Window() = default;

	Window(const Window&) = delete;
	Window(Window&&) noexcept = delete;
	Window& operator=(const Window&) = delete;
	Window& operator=(Window&&) noexcept = delete;

	void Init(WNDPROC wndProcExt = nullptr);

	uint16_t GetWidth() const { return m_Width; }
	uint16_t GetHeight() const { return m_Height; }
	HWND GetHandle() const { return m_Handle; }
	bool IsInitialized() const { return m_bIsInitialized; }

private:
	std::wstring m_Name;
	uint16_t m_Width;
	uint16_t m_Height;
	HWND m_Handle;
	WNDPROC m_WndProcImpl;

	bool m_bIsInitialized;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

