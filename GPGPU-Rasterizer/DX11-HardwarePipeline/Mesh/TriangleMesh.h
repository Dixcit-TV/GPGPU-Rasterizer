#pragma once
#include <DirectXMath.h>
#include <vector>

class Camera;
class Material;

class TriangleMesh
{
public:
	explicit TriangleMesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents = false);
	~TriangleMesh();

	TriangleMesh(const TriangleMesh&) = delete;
	TriangleMesh(TriangleMesh&&) noexcept = delete;
	TriangleMesh& operator=(const TriangleMesh&) = delete;
	TriangleMesh& operator=(TriangleMesh&&) noexcept = delete;

	void SetMaterial(ID3D11Device* pdevice, Material* pmaterial);
	void SetupDrawInfo(Camera* pcamera, ID3D11DeviceContext* pdeviceContext) const;

	UINT GetIndexCount() const { return static_cast<UINT>(std::size(m_Indices)); }

private:
	DirectX::XMFLOAT4X4 m_WorldMatrix;

	std::vector<DirectX::XMFLOAT3> m_VertexPositions;
	std::vector<DirectX::XMFLOAT3> m_VertexNorms;
	std::vector<DirectX::XMFLOAT3> m_VertexTangents;
	std::vector<DirectX::XMFLOAT2> m_VertexUvs;
	std::vector<uint32_t> m_Indices;

	ID3D11Buffer* m_VertexBuffer;
	ID3D11Buffer* m_IndexBuffer;
	Material* m_pMaterial;

	void BuildVertexBuffer(ID3D11Device* pdevice);
	void BuildIndexBuffer(ID3D11Device* pdevice);
};

