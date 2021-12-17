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

void Camera::Update()
{
	DirectX::XMFLOAT3 offset{};
	const float linearVelocity{ 0.0001f };
	const float mouseSensitivity{ 0.000001f };

	float roll{ 0.f }, pitch{ 0.f };
	bool updated{ false };

	BYTE* keyboardState{new BYTE[256]};
	[[maybe_unused]] BOOL res{ GetKeyboardState(keyboardState) };

	DirectX::XMFLOAT2 rotateDir{ (keyboardState[VK_LEFT] & 0xF0) * -1.f + (keyboardState[VK_RIGHT] & 0xF0) * 1.f
						, (keyboardState[VK_DOWN] & 0xF0) * -1.f + (keyboardState[VK_UP] & 0xF0) * 1.f };

	if (abs(rotateDir.x) > FLT_EPSILON || abs(rotateDir.y) > FLT_EPSILON)
	{
		updated = true;
		roll += rotateDir.y;
		pitch -= rotateDir.x;
	}

	DirectX::XMFLOAT2 movDir{ (keyboardState['A'] & 0xF0) * 1.f + (keyboardState['D'] & 0xF0) * -1.f
							, (keyboardState['S'] & 0xF0) * -1.f + (keyboardState['W'] & 0xF0) * 1.f };

	if (abs(movDir.x) > FLT_EPSILON || abs(movDir.y) > FLT_EPSILON)
	{
		updated = true;
		offset.x += movDir.x * linearVelocity;
		offset.z += movDir.y * -linearVelocity;
	}

	if (updated)
	{
		DirectX::XMMATRIX xmRotation{ DirectX::XMMatrixRotationRollPitchYaw(roll * mouseSensitivity, pitch * mouseSensitivity, 0.f) };
		DirectX::XMMATRIX xmTranslation{ DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&offset)) };

		const DirectX::XMMATRIX viewMatrix{ XMLoadFloat4x4(&m_ViewMatrix) * xmTranslation * xmRotation };

		const DirectX::XMMATRIX viewInvMatrix{ XMMatrixInverse(nullptr, viewMatrix) };

		XMStoreFloat4x4(&m_ViewMatrix, viewMatrix);
		XMStoreFloat4x4(&m_ViewInvMatrix, viewInvMatrix);
	}
}