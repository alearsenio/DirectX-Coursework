#include "pch.h"
#include "Player.h"


void Player::Update()
{
	
    float deltaTime = m_timer.GetElapsedSeconds();

    //m_localPosition += (m_forward * m_movespeed) * deltaTime;
    
    if (m_InputCommands.m_yaw != 0)
    {
        m_orientation.y = m_orientation.y - m_InputCommands.m_yaw * m_rotateSpeed * deltaTime;
    }
    if (m_InputCommands.forward)
    {
        m_localPosition += (m_forward * m_movespeed) * deltaTime; //add the forward vector
    }
    if (m_InputCommands.right)
    {
        m_localPosition += (m_right * m_movespeed) * deltaTime; //add the forward vector
    }
    if (m_InputCommands.left)
    {
        m_localPosition -= (m_right * m_movespeed) * deltaTime; //add the forward vector
    }
    if (m_InputCommands.back)
    {
        m_localPosition -= (m_forward * m_movespeed) * deltaTime; //add the forward vector
    }
    if (m_InputCommands.up && m_globalPosition.y < -1.0f)
    {
        m_localPosition += (SimpleMath::Vector3(0.0f, 1.0f, 0.0f) * m_movespeed) * deltaTime; //add the forward vector
    }
    if (m_InputCommands.down)
    {
        m_localPosition -= (SimpleMath::Vector3(0.0f, 1.0f, 0.0f) * m_movespeed) * deltaTime; //add the forward vector
    }
    
    GameObject::Update();
}