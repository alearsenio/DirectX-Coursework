#pragma once

#include "GameObject.h"

class Missile : public GameObject
{
public:

	void Update();

public:

	float lifeTime = 0;
};

