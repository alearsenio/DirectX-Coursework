#pragma once

#include "modelclass.h"
#include "Shader.h"
#include "Input.h"
#include "StepTimer.h"
#include "directxcollision.h"
#include "Terrain.h"

//#include "Game.h"

using namespace DirectX;

class GameObject
{
public:

	GameObject();
	~GameObject();

	void							Initialise(DX::StepTimer const& timer, InputCommands* inputCommands, GameObject* gameObject);
	virtual void					Update();
	DirectX::SimpleMath::Matrix		getCameraMatrix();
	virtual void					setLocalPosition(DirectX::SimpleMath::Vector3 newPosition);
	virtual DirectX::SimpleMath::Vector3	getPosition();
	void							setScale(DirectX::SimpleMath::Vector3 scale);
	DirectX::SimpleMath::Vector3	getScale();
	void							setTag(std::string tag);
	std::string						getTag();
	//void							setSceneObjectsList(Game* game);
	void							setBoxCollider(BoundingOrientedBox boundingBox);
	BoundingOrientedBox				getBoxCollider();
	void							setSphereCollider(BoundingSphere boundingSphere);
	BoundingSphere					getSphereCollider();
	DirectX::SimpleMath::Vector3	getForward();
	DirectX::SimpleMath::Vector3	getRight();
	DirectX::SimpleMath::Vector3	getUp();
	void							setRotation(DirectX::SimpleMath::Vector3 newRotation);
	DirectX::SimpleMath::Vector3	getRotation();
	float							getMoveSpeed();
	float							getRotationSpeed();
	void							setModel(ModelClass	*model);
	ModelClass						getModel();
	void							setTexture(ID3D11ShaderResourceView * texture);
	ID3D11ShaderResourceView*		getTexture();
	virtual void					Render(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix* view, DirectX::SimpleMath::Matrix* projection, Light* sceneLight);
	void							setShader(Shader *shaderPair);
	void							setReflective(bool isReflective, ID3D11ShaderResourceView* enviromentTexture, GameObject* cameraObject);
	void							setParentObject(GameObject *gameObject);


public:
	DirectX::SimpleMath::Matrix		m_gameObjectMatrix;			//GameObject matrix to be passed out and used to set GameObject position and angle
	DirectX::SimpleMath::Vector3	m_lookat;
	DirectX::SimpleMath::Vector3	m_localPosition;
	DirectX::SimpleMath::Vector3	m_globalPosition;
	DirectX::SimpleMath::Vector3	m_scale;
	DirectX::SimpleMath::Vector3	m_forward;
	DirectX::SimpleMath::Vector3	m_right;
	DirectX::SimpleMath::Vector3	m_up;
	DirectX::SimpleMath::Vector3	m_orientation;			//vector storing pitch yaw and roll. 


	//input manager. 
	Input																	m_input;
	InputCommands															m_InputCommands;
	std::string																m_tag;

	// Rendering loop timer.
	DX::StepTimer															m_timer;

	ModelClass																m_gameObjectModel;
	Shader																	m_gameObjectShaderPair;
	ID3D11ShaderResourceView*												m_albedoTexture;
	ID3D11ShaderResourceView*												m_enviromentTexture;

	GameObject*																m_parentObject;	//GameObject parent  to be used as relative world coordinate
	GameObject*																m_cameraObject;	//GameObject camera  to be used for reflection

	BoundingOrientedBox														m_boxCollider;
	BoundingSphere															m_sphereCollider;

	//Game*																	m_Game;

	float	m_movespeed;
	float	m_rotateSpeed;
	bool	m_isReflective;
	bool	hasBoxCollider;
	bool	hasSphereCollider;
	bool	isTerrain;
	bool    toDelete = false;

};

