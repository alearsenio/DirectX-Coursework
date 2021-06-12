#include "pch.h"
#include "Watermine.h"



void Watermine::Update()
{
	//take delta time from timer
	float deltaTime = m_timer.GetElapsedSeconds();

	//increment watermine total lifetime
	lifeTime += deltaTime;

	m_localPosition.y = startingPosition.y + sin(XMConvertToRadians(lifeTime * 80) + startingPosition.y) * 2;
	m_orientation.y = lifeTime * 40;
	GameObject::Update();
}

//save the starting position of the watermine to move it relatively from that point
void Watermine::setLocalPosition(DirectX::SimpleMath::Vector3 newPosition)
{
	GameObject::setLocalPosition(newPosition);
	startingPosition = m_localPosition;
}