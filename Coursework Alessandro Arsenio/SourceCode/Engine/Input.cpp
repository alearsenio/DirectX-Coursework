#include "pch.h"
#include "Input.h"
#include <Mouse.h>
#include <DirectXMath.h>
Input::Input()
{
}

Input::~Input()
{
}

void Input::Initialise(HWND window)
{
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);
	m_quitApp = false;

	m_GameInput.forward = false;
	m_GameInput.back = false;
	m_GameInput.right = false;
	m_GameInput.left = false;
	m_GameInput.rotRight = false;
	m_GameInput.rotLeft = false;
	m_GameInput.mouseButton = false;
	m_GameInput.up = false;
	m_GameInput.down = false;
	m_GameInput.m_pitch = 0;
	m_GameInput.m_yaw = 0;
}

void Input::Update()
{
	auto kb = m_keyboard->GetState();	//updates the basic keyboard state
	m_KeyboardTracker.Update(kb);		//updates the more feature filled state. Press / release etc. 
	auto mouse = m_mouse->GetState();   //updates the basic mouse state
	m_MouseTracker.Update(mouse);		//updates the more advanced mouse state. 
	m_mouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	if (kb.Escape)// check has escape been pressed.  if so, quit out. 
	{
		m_quitApp = true;
	}

	//A key
	if (kb.A)	m_GameInput.left = true;
	else		m_GameInput.left = false;

	//D key
	if (kb.D)	m_GameInput.right = true;
	else		m_GameInput.right = false;

	//W key
	if (kb.W)	m_GameInput.forward = true;
	else		m_GameInput.forward = false;

	//S key
	if (kb.S)	m_GameInput.back = true;
	else		m_GameInput.back = false;

	//mouse left button
	if (mouse.leftButton)	m_GameInput.mouseButton = true;
	else					m_GameInput.mouseButton = false;

	//E key
	if (kb.E)	m_GameInput.up = true;
	else		m_GameInput.up = false;

	//Q key
	if (kb.Q)	m_GameInput.down = true;
	else		m_GameInput.down = false;

	//R key
	if (kb.R)	m_GameInput.restart = true;
	else		m_GameInput.restart = false;


	if (mouse.positionMode == DirectX::Mouse::MODE_RELATIVE)
	{
		DirectX::SimpleMath::Vector3 delta = DirectX::SimpleMath::Vector3(float(mouse.x), float(mouse.y), 0.f);

		m_GameInput.m_pitch = delta.y;
		m_GameInput.m_yaw = delta.x;

		// limit pitch to straight up or straight down
		// with a little fudge-factor to avoid gimbal lock
		float limit = 3.1415f / 2.0f - 0.01f;
		m_GameInput.m_pitch = std::max(-limit, m_GameInput.m_pitch);
		m_GameInput.m_pitch = std::min(+limit, m_GameInput.m_pitch);


	}




}

bool Input::Quit()
{
	return m_quitApp;
}

InputCommands Input::getGameInput()
{
	return m_GameInput;
}
