//
// Game.h
//
#pragma once

#include <algorithm>
#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shader.h"
#include "WaterShader.h"
#include "modelclass.h"
#include "Light.h"
#include "Input.h"
#include "Camera.h"
#include "RenderTexture.h"
#include "Terrain.h"
#include "GameObject.h"
#include "Player.h"
#include "CameraObject.h"
#include "Missile.h"
#include "TerrainObject.h"
#include "Watermine.h"
#include <random>
#include <iostream>
#include <vector>



const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);
#ifdef DXTK_AUDIO
    void NewAudioDevice();
#endif

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
	
public:

	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	}; 

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderSceneObjects();
    void PostProcessing();
    void Clear();
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void RenderReflectedScene();
    void RenderPlanarShadows();
    void RenderTexturePass1();
    void CreateMissile();
    void RestartGame();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	//input manager. 
	Input									m_input;
	InputCommands							m_gameInputCommands;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;	
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;

	//lights
	Light																	m_Light;

	//Cameras
	CameraObject														    m_Camera01;

	//textures 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureLeaves;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureSky;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureWood;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureWater;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureShadow;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureSubmarine;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureFirstBoat;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureSecondBoat;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureRock;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureDolphins;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureGameOver;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureMoss;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_textureSand;

	//Shaders
	Shader																	m_DefaultShader;
	Shader																	m_TerrainShader;
    Shader																	m_SkyboxShader;
    Shader																	m_ReflectiveShader;
    Shader																	m_WaterShader;
    Shader																	m_PlanarShadowShader;
    WaterShader																m_WaterWithGeometryShader;


    //models
	ModelClass																m_BasicModel;
	ModelClass																boatModel;
    ModelClass																waterModel;
    ModelClass																SkyboxBox;
    ModelClass																terrainModel;
    ModelClass																submarineModel;
    ModelClass																palmsTrunckModel;
    ModelClass																palmsLeavesModel;
    ModelClass																rocksModel;
    ModelClass																dolphinsModel;
    ModelClass																birdsModel;
    ModelClass																missileModel;
    ModelClass																watermineModel;


	//RenderTextures
	RenderTexture*															m_FirstRenderPass;
	RECT																	m_fullscreenRect;
	RECT																	m_CameraViewRect;
	RECT																	m_UIRect;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>								m_pixelShader;
	
    //total time
    float                                                                   time = 0;

   //game objects
    std::vector<Watermine*>                                                 m_waterminesList;
    std::vector<GameObject*>                                                m_missilesList;
    std::vector<GameObject*>                                                m_sceneObjectsList;
    Player                                                                  m_playerObject;
    TerrainObject                                                           m_terrainObject;
    TerrainObject                                                           m_waterObject;

    bool                                                                    previousFireButtonState = false;
    bool                                                                    currentFireButtonState = false;
    bool                                                                    isGameOver = false;
#ifdef DXTK_AUDIO
    std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif
    

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;
    bool                                                                    m_retryDefault;
#endif

    DirectX::SimpleMath::Matrix                                             m_world;
    DirectX::SimpleMath::Matrix                                             m_view;
    DirectX::SimpleMath::Matrix                                             m_viewMinimap;
    DirectX::SimpleMath::Matrix                                             m_projection;

    //position of cameras
    DirectX::SimpleMath::Vector3                                            cameraPosition;

    // variables for the mirror effect and shadow
    ID3D11BlendState* NoRenderTargetWritesBS = 0;
    ID3D11DepthStencilState* MarkMirrorDSS = 0;
    ID3D11RasterizerState* CullClockwiseRS = 0;
    ID3D11DepthStencilState* DrawReflectionDSS = 0;
    ID3D11BlendState* TransparentBS = 0;
    ID3D11DepthStencilState* NoDoubleBlendDSS = 0;
    ID3D11DepthStencilState* depthDisabledDSS;


    //Post Process
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_gaussianBlurPS;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurParamsWidth;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurParamsHeight;



};