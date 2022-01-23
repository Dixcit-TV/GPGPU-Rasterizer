#include "pch.h"
#include "ObjReader.h"

void ObjReader::LoadModel(const std::wstring& objPath, std::vector<DirectX::XMFLOAT3>& positions, std::vector<DirectX::XMFLOAT3>& normals, std::vector<DirectX::XMFLOAT2>&, std::vector<uint32_t>& indexBuffer)
{
	using namespace DirectX;

	std::ifstream objStream{ objPath, std::ios::in | std::ios::ate };

	if (!objStream.is_open())
	{
		std::wcout << L"Error: Could not open obj file \"" << objPath << "\".";
		return;
	}

	constexpr size_t default_start_size{ 10000 };

	std::string line{};
	std::vector<XMFLOAT3> tmpVertices;
	tmpVertices.reserve(default_start_size);
	std::vector<XMFLOAT3> tmpVNormals;
	tmpVNormals.reserve(default_start_size);
	std::vector<XMFLOAT2> tmpUVs;
	tmpUVs.reserve(default_start_size);
	std::vector<std::string> tmpFaces;
	tmpFaces.reserve(default_start_size);

	//Parse all data in temporary arrays
	objStream.seekg(0, std::ios::beg);
	while (!objStream.eof())
	{
		std::getline(objStream, line, '\n');

		if (line._Starts_with("f "))
		{
			tmpFaces.push_back(line);
		}
		else
		{
			float f0, f1, f2;
			[[maybe_unused]] int ret = sscanf_s(line.c_str(), "%*s %f %f %f", &f0, &f1, &f2);

			if (line._Starts_with("v "))
			{
				tmpVertices.push_back({ f0, f1, f2 });
			}
			else if (line._Starts_with("vn "))
			{
				tmpVNormals.push_back({ f0, f1, f2 });
			}
			else if (line._Starts_with("vt "))
			{
				tmpUVs.push_back({ f0, 1 - f1 });
			}
		}
	}

	indexBuffer.reserve(tmpFaces.size());
	positions.reserve(tmpFaces.size());
	normals.reserve(tmpFaces.size());
	constexpr UINT vFaceCount{ 3 };

	//construct vertex and index buffer based on faces information
	for (const std::string& faceStr : tmpFaces)
	{
		int iVs[vFaceCount]{}, iNs[vFaceCount]{};
		[[maybe_unused]] int ret = sscanf_s(faceStr.c_str(), "f %d/%*d/%d %d/%*d/%d %d/%*d/%d", &iVs[0], &iNs[0], &iVs[1], &iNs[1], &iVs[2], &iNs[2]);

		for (int idx{}; idx < vFaceCount; ++idx)
		{
			const XMFLOAT3& position{ tmpVertices[iVs[idx] - 1] };
			XMFLOAT3 normal{};
			if (!tmpVNormals.empty())
				normal = tmpVNormals[iNs[idx] - 1];

			int64_t vIdx{ GetVertexIdx(position, positions) };

			if (vIdx == -1
				|| XMVector3NotEqual(XMLoadFloat3(&normal), XMLoadFloat3(&normals[static_cast<uint32_t>(vIdx)])))
			{
				vIdx = positions.size();
				positions.push_back(position);
				normals.push_back(normal);
			}

			indexBuffer.push_back(static_cast<uint32_t>(vIdx));
		}
	}

	indexBuffer.shrink_to_fit();
	positions.shrink_to_fit();
	normals.shrink_to_fit();
}

int64_t ObjReader::GetVertexIdx(const DirectX::XMFLOAT3& vertexPos, const std::vector<DirectX::XMFLOAT3>& vertexPositions)
{
	std::vector<DirectX::XMFLOAT3>::const_iterator cIt{ std::find_if(std::cbegin(vertexPositions), std::cend(vertexPositions), [&vertexPos](const DirectX::XMFLOAT3& pos)
		{
			return DirectX::XMVector3NotEqual(XMLoadFloat3(&vertexPos), XMLoadFloat3(&pos));
		})
	};

	return cIt == vertexPositions.cend() ? -1 : std::distance(vertexPositions.cbegin(), cIt);
}