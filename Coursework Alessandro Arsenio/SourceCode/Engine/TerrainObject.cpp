#include "pch.h"
#include "TerrainObject.h"


void TerrainObject::Render(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix* view, DirectX::SimpleMath::Matrix* projection, Light* sceneLight)
{
	SimpleMath::Matrix m_world = SimpleMath::Matrix::Identity;
	SimpleMath::Matrix translation = SimpleMath::Matrix::CreateTranslation(m_globalPosition);
	SimpleMath::Matrix rotationX = SimpleMath::Matrix::CreateRotationX(XMConvertToRadians(m_orientation.x));
	SimpleMath::Matrix rotationY = SimpleMath::Matrix::CreateRotationY(XMConvertToRadians(m_orientation.y));
	SimpleMath::Matrix rotationZ = SimpleMath::Matrix::CreateRotationZ(XMConvertToRadians(m_orientation.z));
	SimpleMath::Matrix scale = SimpleMath::Matrix::CreateScale(m_scale);
	m_world = scale * m_world * rotationX * rotationY * rotationZ * translation;

	if (isWater)
	{
		m_waterShader.EnableShader(context);
		if (!m_isReflective)
		{
			m_waterShader.SetWaterShaderParameters(context, &m_world, view, projection, sceneLight, m_albedoTexture, m_timer.GetTotalSeconds());
		}
		else
		{
			m_waterShader.SetWaterReflectionShaderParameters(context, &m_world, view, projection, sceneLight, m_albedoTexture, m_enviromentTexture, &m_cameraObject->getPosition(), m_timer.GetTotalSeconds());
		}
	}
	else
	{
		m_gameObjectShaderPair.EnableShader(context);
		//check if the terrain has more than one texture
		if (hasHeightTextures)
		{
			m_gameObjectShaderPair.SetMultiTextureShaderParameters(context, &m_world, view, projection, sceneLight, m_heightTexture1, m_heightTexture2, m_heightTexture3, m_timer.GetTotalSeconds());
		}
		else
		{
			m_gameObjectShaderPair.SetShaderParameters(context, &m_world, view, projection, sceneLight, m_albedoTexture);
		}
	}

	m_gameObjectTerrain.Render(context);
	context->GSSetShader(NULL, 0, 0);
}

void TerrainObject::setTerrain(Terrain* terrain)
{
	m_gameObjectTerrain = *terrain;
}

Terrain* TerrainObject::getTerrain()
{
	return &m_gameObjectTerrain;
}

void TerrainObject::setWaterShader(WaterShader* watershader)
{
	isWater = true;
	m_waterShader = *watershader;
}

void TerrainObject::setHeightTextures(ID3D11ShaderResourceView* texture1,ID3D11ShaderResourceView* texture2,ID3D11ShaderResourceView* texture3)
{
	hasHeightTextures = true;
	m_heightTexture1 = texture1;
	m_heightTexture2 = texture2;
	m_heightTexture3 = texture3;
}