#include "pch.h"
#include "Camera.h"

Camera::Camera(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& forward, float aspectRatio, float fovDeg, float nearPlane, float farPlane)
	: m_ViewMatrix{}
	, m_ProjectionMatrix{}
	, m_ViewInvMatrix{}
	, m_ProjectionInvMatrix{}
	, m_AspectRatio{ aspectRatio }
	, m_FOV{ DirectX::XMConvertToRadians(fovDeg) }
	, m_NearPlane{ nearPlane }
	, m_FarPlane{ farPlane }
{
	using namespace DirectX;

	RecalculateProjection();

	const XMVECTOR xmPos{ XMLoadFloat3(&position) };
	const XMVECTOR xmFwd{ XMLoadFloat3(&forward) };

	const XMVECTOR xmRight{ XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), xmFwd) };
	const XMVECTOR xmUp{ XMVector3Cross(xmFwd, xmRight) };

	const XMMATRIX viewMatrix{ XMMatrixLookToLH(xmPos, xmFwd, xmUp) };
	const XMMATRIX viewInvMatrix{ XMMatrixInverse(nullptr, viewMatrix) };

	XMStoreFloat4x4(&m_ViewMatrix, viewMatrix);
	XMStoreFloat4x4(&m_ViewInvMatrix, viewInvMatrix);
}

void Camera::RecalculateProjection()
{
	using namespace DirectX;
	const XMMATRIX projectionMatrix{ XMMatrixPerspectiveFovLH(m_FOV, m_AspectRatio, m_NearPlane, m_FarPlane) };
	const XMMATRIX projectionInvMatrix{ XMMatrixInverse(nullptr, projectionMatrix) };

	XMStoreFloat4x4(&m_ProjectionMatrix, projectionMatrix);
	XMStoreFloat4x4(&m_ProjectionInvMatrix, projectionInvMatrix);
}

DirectX::XMFLOAT4X4 Camera::GetViewProjection() const
{
	using namespace DirectX;

	XMFLOAT4X4 res{};
	const XMMATRIX viewMatrix{ XMLoadFloat4x4(&m_ViewMatrix) };
	const XMMATRIX projectionMatrix{ XMLoadFloat4x4(&m_ProjectionMatrix) };
	XMStoreFloat4x4(&res, viewMatrix * projectionMatrix);

	return res;
}

DirectX::XMFLOAT4X4 Camera::GetViewProjectionInverse() const
{
	using namespace DirectX;

	XMFLOAT4X4 res{};
	const XMMATRIX viewInvMatrix{ XMLoadFloat4x4(&m_ViewInvMatrix) };
	const XMMATRIX projectionInvMatrix{ XMLoadFloat4x4(&m_ProjectionInvMatrix) };
	XMStoreFloat4x4(&res, projectionInvMatrix * viewInvMatrix);

	return res;
}