#pragma once
#include "GameObject.h"

class Watermine: public GameObject
{
public:
	void Update();
	void setLocalPosition(DirectX::SimpleMath::Vector3 newPosition);

public:
	float lifeTime = 0;
	SimpleMath::Vector3 startingPosition;
};

