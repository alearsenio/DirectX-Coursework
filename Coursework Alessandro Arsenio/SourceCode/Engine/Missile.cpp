#include "pch.h"
#include "Missile.h"

#define MAXLIFETIME 5
#define SPEED 50

void Missile::Update()
{
    //take delta time from timer
    float deltaTime = m_timer.GetElapsedSeconds();

    //increment life time of missile
    lifeTime += deltaTime;
    //if the missile didn't hit anything after MAXLIFETIME seconds, delete it
    if (lifeTime > MAXLIFETIME)
    {
        toDelete = true;
    }
    
    //move missile
    m_localPosition += (m_forward * SPEED) * deltaTime;

    GameObject::Update();
}