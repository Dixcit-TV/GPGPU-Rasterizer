#pragma once
#include <DirectXMath.h>
#include <vector>

class Camera;

namespace CompuRaster
{
	class Material;

	class CompuMesh
	{
	public:
		explicit CompuMesh(std::vector<DirectX::XMFLOAT3>&& positions, std::vector<DirectX::XMFLOAT3>&& normals, std::vector<DirectX::XMFLOAT2>&& uvs, std::vector<uint32_t>&& indices, bool calculateTangents = false);
		~CompuMesh();

		CompuMesh(const CompuMesh&) = delete;
		CompuMesh(CompuMesh&&) noexcept = delete;
		CompuMesh& operator=(const CompuMesh&) = delete;
		CompuMesh& operator=(CompuMesh&&) noexcept = delete;

		void SetMaterial(ID3D11Device* pdevice, Material* pmaterial);
		void SetupDrawInfo(Camera* pcamera, ID3D11DeviceContext* pdeviceContext) const;
		ID3D11ShaderResourceView* GetVertexBufferView() const { return m_VertexBufferView; }
		ID3D11ShaderResourceView* GetIndexBufferView() const { return m_IndexBufferView; }
		ID3D11ShaderResourceView* GetVertexOutBufferView() const { return m_VertexOutBufferView; }
		ID3D11UnorderedAccessView* GetVertexOutBufferUAV() const { return m_VertexOutBufferUAV; }

		UINT GetIndexCount() const { return static_cast<UINT>(std::size(m_Indices)); }
		UINT GetVertexCount() const { return static_cast<UINT>(std::size(m_VertexPositions)); }

	private:
		DirectX::XMFLOAT4X4 m_WorldMatrix;

		std::vector<DirectX::XMFLOAT3> m_VertexPositions;
		std::vector<DirectX::XMFLOAT3> m_VertexNorms;
		std::vector<DirectX::XMFLOAT3> m_VertexTangents;
		std::vector<DirectX::XMFLOAT2> m_VertexUvs;
		std::vector<uint32_t> m_Indices;

		ID3D11ShaderResourceView* m_VertexBufferView;
		ID3D11ShaderResourceView* m_VertexOutBufferView;
		ID3D11ShaderResourceView* m_IndexBufferView;
		ID3D11UnorderedAccessView* m_VertexOutBufferUAV;
		ID3D11Buffer* m_VertexBuffer;
		ID3D11Buffer* m_VertexOutBuffer;
		ID3D11Buffer* m_IndexBuffer;
		Material* m_pMaterial;

		void BuildVertexBuffer(ID3D11Device* pdevice);
		void BuildIndexBuffer(ID3D11Device* pdevice);
	};
}

