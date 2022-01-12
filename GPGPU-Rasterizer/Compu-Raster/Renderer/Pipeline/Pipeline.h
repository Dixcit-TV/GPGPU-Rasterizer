#pragma once
#include "Render/Shader/Shader.h"

class Camera;

namespace CompuRaster
{
	constexpr char VERTEX_IN_VERBOSE[]{ "G_VERTEX_BUFFER" };
	constexpr char INDEX_IN_VERBOSE[]{ "G_INDEX_BUFFER" };
	constexpr char TRANS_VERTEX_VERBOSE[]{ "G_TRANS_VERTEX_BUFFER" };
	constexpr char PIXEL_OUT_VERBOSE[]{ "G_PIXEL_OUT_TEXTURE" };

	class CompuMesh;

	class Pipeline
	{
	public:
		explicit Pipeline();
		~Pipeline();

		Pipeline(const Pipeline&) = delete;
		Pipeline(Pipeline&&) noexcept = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline& operator=(Pipeline&&) noexcept = delete;

		void Init(ID3D11Device* pdevice, UINT vCount, UINT triangleCount, const wchar_t* geometrySetupPath, const wchar_t* binningPath, const wchar_t* tilePath, const wchar_t* finePath);

		void Dispatch(ID3D11DeviceContext* pdeviceContext, CompuMesh* pmesh, Camera* pcamera) const;

	private:
		ComputeShader* m_pGeometrySetupShader;
		ComputeShader* m_pBinningShader;
		ComputeShader* m_pCoarseShader;
		ComputeShader* m_pFineShader;

		ID3D11Buffer* m_pVOutoutBuffer = nullptr;
		ID3D11ShaderResourceView* m_pVOutoutSRV = nullptr;
		ID3D11UnorderedAccessView* m_pVOutoutUAV = nullptr;

		ID3D11Buffer* m_pRasterDataBuffer = nullptr;
		ID3D11ShaderResourceView* m_pRasterDataSRV = nullptr;
		ID3D11UnorderedAccessView* m_pRasterDataUAV = nullptr;

		ID3D11Buffer* m_pBinBuffer = nullptr;
		ID3D11ShaderResourceView* m_pBinSRV = nullptr;
		ID3D11UnorderedAccessView* m_pBinUAV = nullptr;

		ID3D11Buffer* m_pTileBuffer = nullptr;
		ID3D11ShaderResourceView* m_pTileSRV = nullptr;
		ID3D11UnorderedAccessView* m_pTileUAV = nullptr;

		ID3D11Buffer* m_pBinCounter = nullptr;
		ID3D11UnorderedAccessView* m_pBinCounterUAV = nullptr;

		ID3D11Buffer* m_pTileCounter = nullptr;
		ID3D11UnorderedAccessView* m_pTileCounterUAV = nullptr;
	};
}

