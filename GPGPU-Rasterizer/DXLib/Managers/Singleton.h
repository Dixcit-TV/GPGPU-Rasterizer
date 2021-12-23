#pragma once

template<typename TYPE>
class Singleton
{
public:
	static TYPE& GetInstance()
	{
		static TYPE instance{};
		return instance;
	}

	virtual ~Singleton() = default;
	Singleton(const Singleton& other) = delete;
	Singleton(Singleton&& other) = delete;
	Singleton& operator=(const Singleton& other) = delete;
	Singleton& operator=(Singleton&& other) = delete;

protected:
	explicit Singleton() = default;
};