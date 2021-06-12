//
// Game.cpp
//

#include "pch.h"
#include "Game.h"


//toreorganise
#include <fstream>

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
	struct VS_BLOOM_PARAMETERS
	{
		float bloomThreshold;
		float blurAmount;
		float bloomIntensity;
		float baseIntensity;
		float bloomSaturation;
		float baseSaturation;
		uint8_t na[8];
	};

	static_assert(!(sizeof(VS_BLOOM_PARAMETERS) % 16),
		"VS_BLOOM_PARAMETERS needs to be 16 bytes aligned");

	struct VS_BLUR_PARAMETERS
	{
		static const size_t SAMPLE_COUNT = 15;

		XMFLOAT4 sampleOffsets[SAMPLE_COUNT];
		XMFLOAT4 sampleWeights[SAMPLE_COUNT];

		void SetBlurEffectParameters(float dx, float dy,
			const VS_BLOOM_PARAMETERS& params)
		{
			sampleWeights[0].x = ComputeGaussian(0, params.blurAmount);
			sampleOffsets[0].x = sampleOffsets[0].y = 0.f;

			float totalWeights = sampleWeights[0].x;

			// Add pairs of additional sample taps, positioned
			// along a line in both directions from the center.
			for (size_t i = 0; i < SAMPLE_COUNT / 2; i++)
			{
				// Store weights for the positive and negative taps.
				float weight = ComputeGaussian(float(i + 1.f), params.blurAmount);

				sampleWeights[i * 2 + 1].x = weight;
				sampleWeights[i * 2 + 2].x = weight;

				totalWeights += weight * 2;

				// To get the maximum amount of blurring from a limited number of
				// pixel shader samples, we take advantage of the bilinear filtering
				// hardware inside the texture fetch unit. If we position our texture
				// coordinates exactly halfway between two texels, the filtering unit
				// will average them for us, giving two samples for the price of one.
				// This allows us to step in units of two texels per sample, rather
				// than just one at a time. The 1.5 offset kicks things off by
				// positioning us nicely in between two texels.
				float sampleOffset = float(i) * 2.f + 1.5f;

				Vector2 delta = Vector2(dx, dy) * sampleOffset;

				// Store texture coordinate offsets for the positive and negative taps.
				sampleOffsets[i * 2 + 1].x = delta.x;
				sampleOffsets[i * 2 + 1].y = delta.y;
				sampleOffsets[i * 2 + 2].x = -delta.x;
				sampleOffsets[i * 2 + 2].y = -delta.y;
			}

			for (size_t i = 0; i < SAMPLE_COUNT; i++)
			{
				sampleWeights[i].x /= totalWeights;
			}
		}

	private:
		float ComputeGaussian(float n, float theta)
		{
			return (float)((1.0 / sqrtf(2 * XM_PI * theta))
				* expf(-(n * n) / (2 * theta * theta)));
		}
	};

	static_assert(!(sizeof(VS_BLUR_PARAMETERS) % 16),
		"VS_BLUR_PARAMETERS needs to be 16 bytes aligned");

	enum BloomPresets
	{
		Default = 0,
		Soft,
		Desaturated,
		Saturated,
		Blurry,
		Subtle,
		None
	};

	BloomPresets g_Bloom = Blurry;

	static const VS_BLOOM_PARAMETERS g_BloomPresets[] =
	{
		//Thresh  Blur Bloom  Base  BloomSat BaseSat
		{ 0.25f,  4,   1.25f, 1,    1,       1 }, // Default
		{ 0,      3,   1,     1,    1,       1 }, // Soft
		{ 0.5f,   8,   2,     1,    0,       1 }, // Desaturated
		{ 0.25f,  4,   2,     1,    2,       0 }, // Saturated
		{ 0,      1.2,   1.2,     0.1f, 1,       1 }, // Blurry
		{ 0.5f,   2,   1,     1,    1,       1 }, // Subtle
		{ 0.25f,  4,   1.25f, 1,    1,       1 }, // None
	};
}

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
	m_deviceResources = std::make_unique<DX::DeviceResources>();
	m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
#ifdef DXTK_AUDIO
	if (m_audEngine)
	{
		m_audEngine->Suspend();
	}
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_input.Initialise(window);
	m_deviceResources->SetWindow(window, width, height);

	m_CameraViewRect.left = 0;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = width;
	m_CameraViewRect.bottom = height;

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();

	auto device = m_deviceResources->GetD3DDevice();

	m_fullscreenRect.left = 0;
	m_fullscreenRect.top = 0;
	m_fullscreenRect.right = 1000;
	m_fullscreenRect.bottom = 800;
	
	int border = 100;
	m_UIRect.left = border;
	m_UIRect.right = width - border;
	m_UIRect.top = border;
	m_UIRect.bottom = height - border;

	isGameOver = false;

	//setup light
	m_Light.setAmbientColour(0.42f, 0.4f, 0.4f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.setPosition(10.0f, 20.0f, -50.0f);
	m_Light.setDirection(0.5f, -1.0f, 0.8f);

	//setup camera
	m_Camera01.setLocalPosition(Vector3(0.0f, 5.0f, -12.0f));
	m_Camera01.setRotation(Vector3(0.0f, 0.0f, 0.0f));
	m_Camera01.setTag("camera");

	//set up player object
	m_playerObject.Initialise(m_timer, &m_gameInputCommands, NULL);
	m_playerObject.setModel(&submarineModel);
	m_playerObject.setTag("player");
	m_playerObject.setShader(&m_ReflectiveShader);
	m_playerObject.setTexture(m_textureSubmarine.Get());
	m_playerObject.setReflective(true, m_textureSky.Get(), &m_Camera01);
	m_playerObject.setLocalPosition(Vector3(190.0f, -1.0f, 290.0f));
	m_playerObject.setRotation(Vector3(0.0f, 180.0f, 0.0f));
	m_playerObject.setScale(Vector3(0.5f, 0.5f, 0.2f));
	m_playerObject.setSphereCollider(BoundingSphere(m_playerObject.getPosition(), 3));
	m_sceneObjectsList.push_back(&m_playerObject); //add player to scene list

	//setup terrain object
	Terrain proceduralTerrain;
	proceduralTerrain.Initialize(device, 512, 512);
	m_terrainObject.Initialise(m_timer, &m_gameInputCommands, NULL);
	m_terrainObject.setTag("terrain");
	m_terrainObject.setShader(&m_TerrainShader);
	m_terrainObject.setTexture(m_texture1.Get());
	m_terrainObject.setHeightTextures(m_textureSand.Get(),m_textureRock.Get(),m_textureMoss.Get());
	m_terrainObject.setLocalPosition(Vector3(0.0f, 0.0f, 0.0f));
	m_terrainObject.setRotation(Vector3(0.0f, 0.0f, 0.0f));
	m_terrainObject.setScale(Vector3(1.0f, 1.0f, 1.0f));
	m_terrainObject.setTerrain(&proceduralTerrain);
	m_sceneObjectsList.push_back(&m_terrainObject); //add terrain object to scene list
	//setup terrain heigh map
	m_terrainObject.getTerrain()->GenerateRandomHeightMap(device, 240, 200, 400);

	//setup water object
	Terrain proceduralWater;
	proceduralWater.Initialize(device, 512, 512);
	m_waterObject.Initialise(m_timer, &m_gameInputCommands, NULL);
	m_waterObject.setTag("water");
	m_waterObject.setWaterShader(&m_WaterWithGeometryShader);
	m_waterObject.setTexture(m_textureWater.Get());
	m_waterObject.setReflective(true, m_textureSky.Get(), &m_Camera01);
	m_waterObject.setLocalPosition(Vector3(0.0f, 0.0f, 0.0f));
	m_waterObject.setRotation(Vector3(0.0f, 0.0f, 0.0f));
	m_waterObject.setScale(Vector3(1.0f, 1.0f, 1.0f));
	m_waterObject.setTerrain(&proceduralWater);

	//setup watermine objects
	std::random_device rd; //obtain a random number from hardware
	std::mt19937 gen(rd()); //seed the generator
	for (int i = 0; i < 25; i++)
	{
		for (int j = 0; j < 25; j++) {
			float heightPosition = m_terrainObject.getTerrain()->GetHeightMapY(j * 20, i * 20);
			if (heightPosition < -4.0f)
			{
				std::uniform_int_distribution<> distr(heightPosition, -4.0f);
				float randomHeight = distr(gen);
				Watermine* watermine = new Watermine();
				watermine->Initialise(m_timer, &m_gameInputCommands, NULL);
				watermine->setModel(&watermineModel);
				watermine->setTag("watermine");
				watermine->setShader(&m_ReflectiveShader);
				watermine->setTexture(m_textureSecondBoat.Get());
				watermine->setReflective(true, m_textureSky.Get(), &m_Camera01);
				watermine->setLocalPosition(Vector3(j * 20, randomHeight, i * 20));
				watermine->setRotation(Vector3(0.0f, 90.0f, 0.0f));
				watermine->setScale(Vector3(1.0f, 1.0f, 1.0f));
				watermine->setSphereCollider(BoundingSphere(watermine->getPosition(), 2));
				m_waterminesList.push_back(watermine); //add watermine to watermines list
				m_sceneObjectsList.push_back(watermine); //add watermine to scene
			}
		}
	}

#ifdef DXTK_AUDIO
	// Create DirectXTK for Audio objects
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
#endif

	m_audEngine = std::make_unique<AudioEngine>(eflags);

	m_audioEvent = 0;
	m_audioTimerAcc = 10.f;
	m_retryDefault = false;

	m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

	m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
	m_effect1 = m_soundEffect->CreateInstance();
	m_effect2 = m_waveBank->CreateInstance(10);

	m_effect1->Play(true);
	m_effect2->Play();
#endif
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game

	//Update all game objects
	m_timer.Tick([&]()
		{
			Update(m_timer);
		});

	//Render all game content. 
	Render();

#ifdef DXTK_AUDIO
	// Only update audio engine once per frame
	if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
#endif


}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	//check if the mouse left button was released after being pressed
	previousFireButtonState = currentFireButtonState;
	currentFireButtonState = m_gameInputCommands.mouseButton;

	//shoot only when releasing the left mouse button
	if (previousFireButtonState && !currentFireButtonState)
	{
		CreateMissile();
	}

	//update game objects delta time and inputs
	m_Camera01.Initialise(m_timer, &m_gameInputCommands, &m_playerObject);
	m_waterObject.Initialise(m_timer, &m_gameInputCommands, NULL);

	//update camera and water
	m_Camera01.Update();
	m_waterObject.Update();

	//update the rest of game objects
	for (std::vector<GameObject*>::iterator it = m_sceneObjectsList.begin(); it != m_sceneObjectsList.end(); ++it) 
	{
		(*it)->Initialise(m_timer, &m_gameInputCommands, NULL);
		(*it)->Update();
	}

	//check collisions between missiles and watermines
	for (std::vector<Watermine*>::iterator waterminesIterator = m_waterminesList.begin(); waterminesIterator != m_waterminesList.end(); ++waterminesIterator)
	{
		for (std::vector<GameObject*>::iterator missilesIterator = m_missilesList.begin(); missilesIterator != m_missilesList.end(); ++missilesIterator)
		{
			//if a missile hit a watermine, set both to be deleted
			if ((*waterminesIterator)->toDelete != true && (*missilesIterator)->toDelete != true && (*missilesIterator)->getSphereCollider().Intersects((*waterminesIterator)->getSphereCollider()))
			{
				(*waterminesIterator)->toDelete = true;
				(*missilesIterator)->toDelete = true;
			}
		}
		if ((*waterminesIterator)->toDelete != true && m_playerObject.getSphereCollider().Intersects((*waterminesIterator)->getSphereCollider()))
		{
			m_playerObject.toDelete = true;
			isGameOver = true;
		}
	}

	//delete all the objects which are set to be deleted because of a collision or because their life time ended
	for (int i = 0; i < m_sceneObjectsList.size(); i++)
	{
		if (m_sceneObjectsList[i]->toDelete)
		{
			if (m_sceneObjectsList[i]->getTag() != "player")
			{
				delete m_sceneObjectsList[i];
			}
			//delete object from scene list
			m_sceneObjectsList.erase(m_sceneObjectsList.begin() + i);
		}
	}
	//remove dead watermines from the watermines list
	for (int i = 0; i < m_waterminesList.size(); i++)
	{
		if (m_waterminesList[i]->toDelete)
		{
			m_waterminesList.erase(m_waterminesList.begin() + i);
		}
	}
	//remove dead missiles from the missiles list
	for (int i = 0; i < m_missilesList.size(); i++)
	{
		if (m_missilesList[i]->toDelete)
		{
			m_missilesList.erase(m_missilesList.begin() + i);
		}
	}

	//if the game is over and the player press the restart button
	if (isGameOver && m_gameInputCommands.restart)
	{
		isGameOver = false;
		RestartGame();
	}

	//update world and view matrix
	cameraPosition = m_Camera01.getPosition();
	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

#ifdef DXTK_AUDIO
	m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
	if (m_audioTimerAcc < 0)
	{
		if (m_retryDefault)
		{
			m_retryDefault = false;
			if (m_audEngine->Reset())
			{
				// Restart looping audio
				m_effect1->Play(true);
			}
		}
		else
		{
			m_audioTimerAcc = 4.f;

			m_waveBank->Play(m_audioEvent++);

			if (m_audioEvent >= 11)
				m_audioEvent = 0;
		}
	}
#endif

	if (m_input.Quit())
	{
		ExitGame();
	}
}

#pragma endregion

#pragma region Frame Render

//Draws the scene.
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}
	Clear();
	m_deviceResources->PIXBeginEvent(L"Render");
	auto context = m_deviceResources->GetD3DDeviceContext();

	//Draw Text to the screen
	m_sprites->Begin();
	m_font->DrawString(m_sprites.get(), L"DirectXTK Demo Window", XMFLOAT2(100, 100), Colors::Yellow);
	m_sprites->End();

	//Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
	//context->RSSetState(m_states->Wireframe());

	//RENDER TO TEXTURE
	RenderTexturePass1();
	Clear();

	//RENDER SCENE
	RenderSceneObjects();

	//UNDERWATER POST-PROCESSING (if the camera is below the level of water)
	if (m_Camera01.getPosition().y < m_waterObject.getPosition().y)
	{
		PostProcessing();
	}

	//if the game is over print on screen
	if(isGameOver) {
		m_sprites->Begin();
		m_sprites->Draw(m_textureGameOver.Get(), m_UIRect);
		m_sprites->End();

		//Draw Text to the screen
		m_sprites->Begin();
		m_font->DrawString(m_sprites.get(), L"Press R to restart the game", XMFLOAT2(10, 10), Colors::Red);
		m_sprites->End();
	}
	// Show the new frame.
	m_deviceResources->Present();
}

void Game::RenderTexturePass1()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();
	auto device = m_deviceResources->GetD3DDevice();

	int width = m_CameraViewRect.right;
	int height = m_CameraViewRect.bottom;
	//Initialise Render to texture
	delete m_FirstRenderPass;
	m_FirstRenderPass = new RenderTexture(device, width, height, 1, 2);
	// Set the render target to be the render to texture.
	m_FirstRenderPass->setRenderTarget(context);
	// Clear the render to texture.
	m_FirstRenderPass->clearRenderTarget(context, 0.0f, 0.0f, 1.0f, 1.0f);

	//RENDER SCENE
	RenderSceneObjects();

	//Reset the render target back to the original back buffer and not the render to texture anymore.
	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
}

void Game::RenderSceneObjects()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	//SKYBOX-----------------------
	//disabling the depth buffer
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition0 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition());
	m_world = m_world * newPosition0;

	//setup and draw skybox
	m_SkyboxShader.EnableShader(context);
	m_SkyboxShader.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureSky.Get());
	SkyboxBox.Render(context);

	//enabling the depth buffer
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	//END SKYBOX------------------

	//RENDER SCENE OBJECTS--------
	for (std::vector<GameObject*>::iterator it = m_sceneObjectsList.begin(); it != m_sceneObjectsList.end(); ++it)
	{
		(*it)->Render(context, &m_view, &m_projection, &m_Light);
	}
	//END RENDER SCENE OBJECTS--------

	//WATER REFLECTION----------------------
	//RenderReflectedScene();

	//render the water with a little bit of trasparence
	context->OMSetBlendState(TransparentBS, blendFactor, 0xFFFFFFFF);

	//if the camera is below the level of water render the water upside down
	if (m_Camera01.getPosition().y < m_waterObject.getPosition().y)
	{
		context->RSSetState(m_states->CullCounterClockwise());
	}
	//prepare transform for water mirror
	m_waterObject.Render(context, &m_view, &m_projection, &m_Light);

	//set the render back to clockwise
	context->RSSetState(m_states->CullClockwise());

	//clear the stencil buffer
	auto depthStencil = m_deviceResources->GetDepthStencilView();
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_STENCIL, 1.0f, 0);

	//set states back to normal
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	//END WATER REFLECTION------------------

	//PLANAR SHADOWS------------------------
	//RenderPlanarShadows();
}

void Game::PostProcessing()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

	//get the screen dimension
	m_CameraViewRect.right = m_deviceResources->GetScreenViewport().Width;
	m_CameraViewRect.bottom = m_deviceResources->GetScreenViewport().Height;
	//(blur horizontal)
	m_FirstRenderPass->setRenderTarget(context);
	m_sprites->Begin(SpriteSortMode_Immediate,
		nullptr, nullptr, nullptr, nullptr,
		[=]() {
			context->PSSetShader(m_gaussianBlurPS.Get(), nullptr, 0);
			context->PSSetConstantBuffers(0, 1,
				m_blurParamsWidth.GetAddressOf());
		});
	m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_CameraViewRect);
	m_sprites->End();

	//(blur vertical)
	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
	m_sprites->Begin(SpriteSortMode_Immediate,
		nullptr, nullptr, nullptr, nullptr,
		[=]() {
			context->PSSetShader(m_gaussianBlurPS.Get(), nullptr, 0);
			context->PSSetConstantBuffers(0, 1,
				m_blurParamsHeight.GetAddressOf());
		});
	m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_CameraViewRect);
	m_sprites->End();
}

void Game::CreateMissile()
{
	Missile* missile = new Missile();
	//set up player object
	missile->Initialise(m_timer, &m_gameInputCommands, NULL);
	missile->setModel(&missileModel);
	missile->setShader(&m_ReflectiveShader);
	missile->setTag("watermine");
	missile->setTexture(m_textureSubmarine.Get());
	//missile->setReflective(true, m_textureSky.Get(), &m_Camera01);
	missile->setLocalPosition(m_playerObject.getPosition() + m_playerObject.getForward() * 4);
	missile->setRotation(m_playerObject.getRotation());
	missile->setScale(Vector3(1.0f, 1.0f, 1.0f));
	missile->setSphereCollider(BoundingSphere(missile->getPosition(), 2));
	m_missilesList.push_back(missile); //add missile to missile list
	m_sceneObjectsList.push_back(missile); //add missile to the scene
}
void Game::RenderReflectedScene()
{
	//float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	//auto context = m_deviceResources->GetD3DDeviceContext();
	//auto renderTargetView = m_deviceResources->GetRenderTargetView();
	//auto depthTargetView = m_deviceResources->GetDepthStencilView();

	/////////////////////////////////////////rendering the water mirror on the stencil buffer only

	//context->OMSetBlendState(NoRenderTargetWritesBS, blendFactor, 0xFFFFFFFF);
	//context->OMSetDepthStencilState(MarkMirrorDSS, 1);

	////prepare transform for mirror object. 
	//m_world = SimpleMath::Matrix::Identity; //set world back to identity
	//SimpleMath::Matrix newPosition4 = SimpleMath::Matrix::CreateTranslation(-64.0f, 0.0f, -64.0f);
	//m_world = m_world * newPosition4;

	////setup and draw mirror	
	//m_BasicShaderPair.EnableShader(context);
	//m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureWater.Get());
	//proceduralWater.Render(context);

	////set states back to normal
	//context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	//context->OMSetDepthStencilState(m_states->DepthDefault(), 0);

	//// Cull clockwise triangles for reflection.
	////context->RSSetState(CullClockwiseRS);
	//context->RSSetState(m_states->CullCounterClockwise());


	//// Only draw reflection into visible mirror pixels as marked by the stencil buffer.
	//context->OMSetDepthStencilState(DrawReflectionDSS, 1);

	////create the plane to reflect
	//DirectX::SimpleMath::Vector3 planePosition = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	//DirectX::SimpleMath::Vector3 planeNormal = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
	//SimpleMath::Plane mirrorPlane = SimpleMath::Plane(planePosition, planeNormal);
	//SimpleMath::Matrix reflection = SimpleMath::Matrix::CreateReflection(mirrorPlane);

	////prepare transform for reflected object. 
	//m_world = SimpleMath::Matrix::Identity;
	//SimpleMath::Matrix newPosition5 = SimpleMath::Matrix::CreateTranslation(20.0f, 0.4f, 30.0f);
	//m_world = m_world * newPosition5;
	//m_world = m_world * reflection;

	////set states back to normal
	//context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	//context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	//context->RSSetState(m_states->CullClockwise());
}

void Game::RenderPlanarShadows()
{

	//float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	//auto context = m_deviceResources->GetD3DDeviceContext();
	//auto renderTargetView = m_deviceResources->GetRenderTargetView();
	//auto depthTargetView = m_deviceResources->GetDepthStencilView();

	//context->OMSetBlendState(TransparentBS, blendFactor, 0xFFFFFFFF);
	//context->OMSetDepthStencilState(NoDoubleBlendDSS, 0);

	////matrix to project the shadow into the plane
	//XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	//XMVECTOR toMainLight = -XMLoadFloat3(&m_Light.getDirection());
	//XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
	//XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.01f, 0.0f);

	////prepare transform for the subamrine shadow
	//m_world = SimpleMath::Matrix::Identity;
	//SimpleMath::Matrix submarineShadowPosition = SimpleMath::Matrix::CreateTranslation(-10.0f, -1.0f, 32.0f);
	//SimpleMath::Matrix submarineShadowRotation = SimpleMath::Matrix::CreateRotationY(XMConvertToRadians(90));
	//m_world = m_world * submarineShadowRotation * submarineShadowPosition;
	//m_world = m_world * S * shadowOffsetY;

	////setup and draw submarine shadow
	//m_PlanarShadowShader.EnableShader(context);
	//m_PlanarShadowShader.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureShadow.Get());
	//submarineModel.Render(context);

	////set states back to normal
	//context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	//context->OMSetDepthStencilState(m_states->DepthDefault(), 0);

}

void Game::RestartGame()
{
	m_playerObject.toDelete = false;
	m_playerObject.setLocalPosition(Vector3(190.0f, -1.0f, 290.0f));
	m_playerObject.setRotation(Vector3(0.0f, 180.0f, 0.0f));
	m_sceneObjectsList.push_back(&m_playerObject);
}
// Helper method to clear the back buffers.
void Game::Clear()
{
	m_deviceResources->PIXBeginEvent(L"Clear");

	// Clear the views.
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTarget = m_deviceResources->GetRenderTargetView();
	auto depthStencil = m_deviceResources->GetDepthStencilView();

	context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, &renderTarget, depthStencil);

	// Set the viewport.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
	m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
	m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
	m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
	auto r = m_deviceResources->GetOutputSize();
	m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
	if (!m_deviceResources->WindowSizeChanged(width, height))
		return;

	CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
	if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
	width = 1366;
	height = 768;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto device = m_deviceResources->GetD3DDevice();

	m_states = std::make_unique<CommonStates>(device);
	m_fxFactory = std::make_unique<EffectFactory>(device);
	m_sprites = std::make_unique<SpriteBatch>(context);
	m_font = std::make_unique<SpriteFont>(device, L"Assets/SegoeUI_18.spritefont");

	//setup models
	m_BasicModel.InitializeTeapot(device);
	boatModel.InitializeModel(device, "Assets/boat.obj");
	waterModel.InitializeModel(device, "Assets/water.obj");	//box includes dimensions
	SkyboxBox.InitializeBox(device, 5.0f, 5.0f, 5.0f);
	terrainModel.InitializeModel(device, "Assets/terrain.obj");
	submarineModel.InitializeModel(device, "Assets/submarine.obj");
	palmsTrunckModel.InitializeModel(device, "Assets/palmsTrunck.obj");
	palmsLeavesModel.InitializeModel(device, "Assets/palmsLeaves.obj");
	rocksModel.InitializeModel(device, "Assets/rocks.obj");
	dolphinsModel.InitializeModel(device, "Assets/dolphins.obj");
	birdsModel.InitializeModel(device, "Assets/birds.obj");
	missileModel.InitializeModel(device, "Assets/rocket.obj");
	watermineModel.InitializeModel(device, "Assets/watermine.obj");


	//load and set up our Vertex and Pixel Shaders
	m_DefaultShader.InitStandard(device, L"light_vs.cso", L"light_ps.cso");
	m_TerrainShader.InitStandard(device, L"light_vs.cso", L"terrain_ps.cso");
	m_SkyboxShader.InitStandard(device, L"skybox_vs.cso", L"skybox_ps.cso");
	m_ReflectiveShader.InitStandard(device, L"reflective_vs.cso", L"reflective_ps.cso");
	m_WaterShader.InitStandard(device, L"water_vs.cso", L"water_ps.cso");
	m_PlanarShadowShader.InitStandard(device, L"light_vs.cso", L"shadow_ps.cso");
	m_WaterWithGeometryShader.InitStandard(device, L"water_vs.cso", L"water_ps.cso", L"water_gs.cso");

	//load Textures
	CreateDDSTextureFromFile(device, L"Assets/rock.dds", nullptr, m_texture1.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/leavesTexture.dds", nullptr, m_textureLeaves.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/wood.dds", nullptr, m_textureWood.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/water3.dds", nullptr, m_textureWater.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/shadow.dds", nullptr, m_textureShadow.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/submarineTexture.dds", nullptr, m_textureSubmarine.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/firstBoatTexture.dds", nullptr, m_textureFirstBoat.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/secondBoatTexture.dds", nullptr, m_textureSecondBoat.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/rock.dds", nullptr, m_textureRock.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/dolphinsTexture.dds", nullptr, m_textureDolphins.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/fatality.dds", nullptr, m_textureGameOver.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/moss.dds", nullptr, m_textureMoss.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/sand.dds", nullptr, m_textureSand.ReleaseAndGetAddressOf());
	CreateDDSTextureFromFile(device, L"Assets/skybox3.dds", nullptr, m_textureSky.ReleaseAndGetAddressOf(), D3D11_RESOURCE_MISC_TEXTURECUBE);
	
	int width = m_CameraViewRect.right;
	int height = m_CameraViewRect.bottom;

	//Initialise Render to texture
	m_FirstRenderPass = new RenderTexture(device, width, height, 1, 2);	//for our rendering, We dont use the last two properties. but.  they cant be zero and they cant be the same. 

	//gaussian blur
	auto blob = DX::ReadData(L"GaussianBlur.cso");
	DX::ThrowIfFailed(device->CreatePixelShader(blob.data(), blob.size(),
		nullptr, m_gaussianBlurPS.ReleaseAndGetAddressOf()));

	//blur resources
	CD3D11_BUFFER_DESC cbDesc(sizeof(VS_BLUR_PARAMETERS),
		D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(device->CreateBuffer(&cbDesc, nullptr,
		m_blurParamsWidth.ReleaseAndGetAddressOf()));
	DX::ThrowIfFailed(device->CreateBuffer(&cbDesc, nullptr,
		m_blurParamsHeight.ReleaseAndGetAddressOf()));

	VS_BLUR_PARAMETERS blurData;
	blurData.SetBlurEffectParameters(1.f / (m_CameraViewRect.right / 2), 0,
		g_BloomPresets[g_Bloom]);
	context->UpdateSubresource(m_blurParamsWidth.Get(), 0, nullptr,
		&blurData, sizeof(VS_BLUR_PARAMETERS), 0);

	blurData.SetBlurEffectParameters(0, 1.f / (m_CameraViewRect.bottom / 2),
		g_BloomPresets[g_Bloom]);
	context->UpdateSubresource(m_blurParamsHeight.Get(), 0, nullptr,
		&blurData, sizeof(VS_BLUR_PARAMETERS), 0);

	///////////////////////////////////////////
	//Title: RenderStates.cpp 
	//Author: Frank Luna
	//Date : 2012
	//Availability : https://github.com/jjuiddong/Introduction-to-3D-Game-Programming-With-DirectX11/tree/master/Chapter%2010%20Stenciling/Mirror
	////////////////////////////////////////////

	// NoRenderTargetWritesBS
	D3D11_BLEND_DESC noRenderTargetWritesDesc = { 0 };
	noRenderTargetWritesDesc.AlphaToCoverageEnable = false;
	noRenderTargetWritesDesc.IndependentBlendEnable = false;

	noRenderTargetWritesDesc.RenderTarget[0].BlendEnable = false;
	noRenderTargetWritesDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	noRenderTargetWritesDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	noRenderTargetWritesDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	noRenderTargetWritesDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	noRenderTargetWritesDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	noRenderTargetWritesDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	noRenderTargetWritesDesc.RenderTarget[0].RenderTargetWriteMask = 0;

	device->CreateBlendState(&noRenderTargetWritesDesc, &NoRenderTargetWritesBS);

	// MarkMirrorDSS
	D3D11_DEPTH_STENCIL_DESC mirrorDesc;
	mirrorDesc.DepthEnable = true;
	mirrorDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	mirrorDesc.DepthFunc = D3D11_COMPARISON_LESS;
	mirrorDesc.StencilEnable = true;
	mirrorDesc.StencilReadMask = 0xff;
	mirrorDesc.StencilWriteMask = 0xff;

	mirrorDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// We are not rendering backfacing polygons, so these settings do not matter.
	mirrorDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&mirrorDesc, &MarkMirrorDSS);

	// CullClockwiseRS
	D3D11_RASTERIZER_DESC cullClockwiseDesc;
	ZeroMemory(&cullClockwiseDesc, sizeof(D3D11_RASTERIZER_DESC));
	cullClockwiseDesc.FillMode = D3D11_FILL_SOLID;
	cullClockwiseDesc.CullMode = D3D11_CULL_BACK;
	cullClockwiseDesc.FrontCounterClockwise = true;
	cullClockwiseDesc.DepthClipEnable = true;

	device->CreateRasterizerState(&cullClockwiseDesc, &CullClockwiseRS);


	// DrawReflectionDSS
	D3D11_DEPTH_STENCIL_DESC drawReflectionDesc;
	drawReflectionDesc.DepthEnable = true;
	drawReflectionDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	drawReflectionDesc.DepthFunc = D3D11_COMPARISON_LESS;
	drawReflectionDesc.StencilEnable = true;
	drawReflectionDesc.StencilReadMask = 0xff;
	drawReflectionDesc.StencilWriteMask = 0xff;

	drawReflectionDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	drawReflectionDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	device->CreateDepthStencilState(&drawReflectionDesc, &DrawReflectionDSS);


	// NoDoubleBlendDSS
	D3D11_DEPTH_STENCIL_DESC noDoubleBlendDesc;
	noDoubleBlendDesc.DepthEnable = true;
	noDoubleBlendDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	noDoubleBlendDesc.DepthFunc = D3D11_COMPARISON_LESS;
	noDoubleBlendDesc.StencilEnable = true;
	noDoubleBlendDesc.StencilReadMask = 0xff;
	noDoubleBlendDesc.StencilWriteMask = 0xff;

	noDoubleBlendDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	noDoubleBlendDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	noDoubleBlendDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	noDoubleBlendDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	device->CreateDepthStencilState(&noDoubleBlendDesc, &NoDoubleBlendDSS);


	// TransparentBS
	D3D11_BLEND_DESC transparentDesc = { 0 };
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;

	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device->CreateBlendState(&transparentDesc, &TransparentBS);

	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;

	depthDisabledStencilDesc.DepthEnable = false;
	depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDisabledStencilDesc.StencilEnable = true;
	depthDisabledStencilDesc.StencilReadMask = 0xFF;
	depthDisabledStencilDesc.StencilWriteMask = 0xFF;
	depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&depthDisabledStencilDesc, &depthDisabledDSS);

}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	auto size = m_deviceResources->GetOutputSize();
	float aspectRatio = float(size.right) / float(size.bottom);
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	m_projection = Matrix::CreatePerspectiveFieldOfView(
		fovAngleY,
		aspectRatio,
		0.01f,
		1000.0f
	);
}


void Game::OnDeviceLost()
{
	m_states.reset();
	m_fxFactory.reset();
	m_sprites.reset();
	m_font.reset();
}

void Game::OnDeviceRestored()
{
	CreateDeviceDependentResources();

	CreateWindowSizeDependentResources();
}
#pragma endregion
