#pragma once
#include "GameObject.h"
#include "WaterShader.h"

class TerrainObject: public GameObject
{
public:

	void							Render(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix* view, DirectX::SimpleMath::Matrix* projection, Light* sceneLight);
	void							setTerrain(Terrain* terrain);
	Terrain*						getTerrain();
	void							setWaterShader(WaterShader *watershader);
	void							setHeightTextures(ID3D11ShaderResourceView* texture1, ID3D11ShaderResourceView* texture2, ID3D11ShaderResourceView* texture3);

private:

	Terrain							m_gameObjectTerrain;
	WaterShader						m_waterShader;
	bool							isWater = false;
	bool							hasHeightTextures = false;
	ID3D11ShaderResourceView*       m_heightTexture1;
	ID3D11ShaderResourceView*       m_heightTexture2;
	ID3D11ShaderResourceView*       m_heightTexture3;

};

