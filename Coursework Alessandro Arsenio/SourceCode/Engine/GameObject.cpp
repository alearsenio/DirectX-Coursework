#include "pch.h"
#include "GameObject.h"

GameObject::GameObject()
{
	//initalise values. 
	//Orientation and Position are how we control the GameObject. 
	m_orientation.x = 0.0f;		//rotation around x - pitch
	m_orientation.y = 0.0f;		//rotation around y - yaw
	m_orientation.z = 0.0f;		//rotation around z - roll	//we tend to not use roll a lot in first person

	m_localPosition.x = 0.0f;		//GameObject position in space. 
	m_localPosition.y = 0.0f;
	m_localPosition.z = 0.0f;

	m_globalPosition.x = 0.0f;		//GameObject position in space. 
	m_globalPosition.y = 0.0f;
	m_globalPosition.z = 0.0f;

	//These variables are used for internal calculations and not set.  but we may want to queary what they 
	//externally at points
	m_lookat.x = 0.0f;		//Look target point
	m_lookat.y = 0.0f;
	m_lookat.z = 0.0f;

	m_forward.x = 0.0f;		//forward/look direction
	m_forward.y = 0.0f;
	m_forward.z = 0.0f;

	m_right.x = 0.0f;
	m_right.y = 0.0f;
	m_right.z = 0.0f;

	//movement and rotation default speed
	m_movespeed = 20;
	m_rotateSpeed = 100;

	//set all the flag to false
	m_parentObject = NULL;
	m_isReflective = false;
	hasBoxCollider = false;
	hasSphereCollider = false;
	isTerrain = false;

	//set default tag
	m_tag = "default";

	//set defalut scale
	m_scale = DirectX::SimpleMath::Vector3(1, 1, 1);

	//force update with initial values to generate other camera data correctly for first update. 
	Update();
}

GameObject::~GameObject()
{
}


void GameObject::Initialise(DX::StepTimer const& timer, InputCommands *inputCommands, GameObject* gameObject)
{
	m_InputCommands = *inputCommands;
	m_timer = timer;
	setParentObject(gameObject);
}



void GameObject::Update()
{
	//rotation in yaw and pitch - using the paramateric equation of a sphere
	m_forward.x = sin((m_orientation.y) * 3.1415f / 180.0f) * cos((m_orientation.x) * 3.1415f / 180.0f);
	m_forward.z = cos((m_orientation.y) * 3.1415f / 180.0f) * cos((m_orientation.x) * 3.1415f / 180.0f);
	m_forward.y = sin((m_orientation.x) * 3.1415f / 180.0f);
	m_forward.Normalize();

	//create right vector from look Direction
	m_forward.Cross(DirectX::SimpleMath::Vector3::UnitY, m_right);
	m_right.Cross(m_forward, m_up);

	//update lookat point
	m_globalPosition = m_localPosition;
	m_lookat = m_globalPosition + m_forward;

	//if the gameobject have a parent object, its coordinates are relative to it
	if (m_parentObject != NULL)
	{
		m_globalPosition = m_parentObject->getPosition() + (m_parentObject->getForward()  * m_localPosition.z);
		m_globalPosition += (m_parentObject->getRight()  * m_localPosition.x);
		m_globalPosition += (m_parentObject->getUp()  * m_localPosition.y);
		m_lookat = m_parentObject->getPosition();
		
	}

	//apply GameObject vectors and create GameObject matrix
	m_gameObjectMatrix = (DirectX::SimpleMath::Matrix::CreateLookAt(m_globalPosition, m_lookat, DirectX::SimpleMath::Vector3::UnitY));


	//update colliders
	if (hasSphereCollider)
	{
		m_sphereCollider.Center = m_globalPosition;
	}
}


void GameObject::Render(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix* view, DirectX::SimpleMath::Matrix* projection, Light* sceneLight)
{

	SimpleMath::Matrix m_world = SimpleMath::Matrix::Identity;
	SimpleMath::Matrix translation = SimpleMath::Matrix::CreateTranslation(m_globalPosition);
	SimpleMath::Matrix rotationX = SimpleMath::Matrix::CreateRotationX(XMConvertToRadians(m_orientation.x));
	SimpleMath::Matrix rotationY = SimpleMath::Matrix::CreateRotationY(XMConvertToRadians(m_orientation.y));
	SimpleMath::Matrix rotationZ = SimpleMath::Matrix::CreateRotationZ(XMConvertToRadians(m_orientation.z));
	SimpleMath::Matrix rotation = rotationX * rotationY * rotationZ;
	SimpleMath::Matrix scale = SimpleMath::Matrix::CreateScale(m_scale);
	m_world = scale * rotation * m_world * translation ;

	//check if the object is reflective
	m_gameObjectShaderPair.EnableShader(context);
	if (!m_isReflective)
	{
		m_gameObjectShaderPair.SetShaderParameters(context, &m_world,view,projection, sceneLight, m_albedoTexture);
	}
	else
	{
		m_gameObjectShaderPair.SetReflectionShaderParameters(context, &m_world,view,projection, sceneLight, m_albedoTexture, m_enviromentTexture, &m_cameraObject->getPosition());
	}

	m_gameObjectModel.Render(context);
}

void GameObject::setModel(ModelClass *model)
{
	m_gameObjectModel = *model;
}

ModelClass GameObject::getModel()
{
	return m_gameObjectModel;
}

void GameObject::setTexture(ID3D11ShaderResourceView *texture)
{
	m_albedoTexture = texture;
}

ID3D11ShaderResourceView* GameObject::getTexture()
{
	return m_albedoTexture;
}

void GameObject::setParentObject(GameObject* gameObject)
{
	m_parentObject = gameObject;
}

void GameObject::setTag(std::string tag)
{
	m_tag = tag;
}
std::string	GameObject::getTag()
{
	return m_tag;
}

void GameObject::setShader(Shader *shaderPair)
{
	m_gameObjectShaderPair = *shaderPair;
}

DirectX::SimpleMath::Matrix GameObject::getCameraMatrix()
{
	return m_gameObjectMatrix;
}

void GameObject::setLocalPosition(DirectX::SimpleMath::Vector3 newPosition)
{
	m_localPosition = newPosition;
}

DirectX::SimpleMath::Vector3 GameObject::getPosition()
{
	return m_globalPosition;
}

void GameObject::setScale(DirectX::SimpleMath::Vector3 scale)
{
	m_scale = scale;
}

DirectX::SimpleMath::Vector3 GameObject::getScale()
{
	return m_scale;
}

//void GameObject::setSceneObjectsList(Game* game)
//{
//	m_Game = game;
//}

DirectX::SimpleMath::Vector3 GameObject::getForward()
{
	return m_forward;
}

DirectX::SimpleMath::Vector3 GameObject::getRight()
{
	return m_right;
}

DirectX::SimpleMath::Vector3 GameObject::getUp()
{
	return m_up;
}

void GameObject::setRotation(DirectX::SimpleMath::Vector3 newRotation)
{

	m_orientation = newRotation;
}

DirectX::SimpleMath::Vector3 GameObject::getRotation()
{
	return m_orientation;
}

void GameObject::setReflective(bool isReflective, ID3D11ShaderResourceView* enviromentTexture, GameObject* cameraObject)
{
	m_isReflective = isReflective;
	m_enviromentTexture = enviromentTexture;
	m_cameraObject = cameraObject;
}

float GameObject::getMoveSpeed()
{
	return m_movespeed;
}

float GameObject::getRotationSpeed()
{
	return m_rotateSpeed;
}

void  GameObject::setBoxCollider(BoundingOrientedBox boundingOrientedBox)
{
	hasBoxCollider = true;
	m_boxCollider = boundingOrientedBox;
}

BoundingOrientedBox	GameObject::getBoxCollider()
{
	return m_boxCollider;
}
void  GameObject::setSphereCollider(BoundingSphere boundingSphere)
{
	hasSphereCollider = true;
	m_sphereCollider = boundingSphere;
}

BoundingSphere	GameObject::getSphereCollider()
{
	return m_sphereCollider;
}
