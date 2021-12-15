#pragma once
#include <DirectXMath.h>

class Camera
{
public:
	explicit Camera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& forward = DirectX::XMFLOAT3{ 0.f, 0.f, 1.f }, float aspectRatio = 16.f/9.f, float fovDeg = 45.f, float nearPlane = 0.1f, float farPlane = FLT_MAX);

	const DirectX::XMFLOAT4X4& GetView() const { return m_ViewMatrix; }
	const DirectX::XMFLOAT4X4& GetViewInverse() const { return m_ViewInvMatrix; }
	const DirectX::XMFLOAT4X4& GetProjection() const { return m_ProjectionMatrix; }
	const DirectX::XMFLOAT4X4& GetProjectionInverse() const { return m_ProjectionInvMatrix; }

	DirectX::XMFLOAT4X4 GetViewProjection() const;
	DirectX::XMFLOAT4X4 GetViewProjectionInverse() const;

private:
	DirectX::XMFLOAT4X4 m_ViewMatrix;
	DirectX::XMFLOAT4X4 m_ProjectionMatrix;
	DirectX::XMFLOAT4X4 m_ViewInvMatrix;
	DirectX::XMFLOAT4X4 m_ProjectionInvMatrix;

	float m_AspectRatio;
	float m_FOV;
	float m_NearPlane;
	float m_FarPlane;

	void RecalculateProjection();
};

