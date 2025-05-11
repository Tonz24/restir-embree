#pragma once
#include <oidn.hpp>

#include "camera.h"
#include "GBufferElement.h"
#include "IDrawGui.h"
#include "Integrator.h"
#include "Reservoir.h"

#include "glm/glm.hpp"

enum RenderPass : uint8_t{
	INITIAL,
	SPATIAL,
	TEMPORAL,
	VISIBILITY
};

class SimpleGuiDX11 : public IDrawGui {

public:
	SimpleGuiDX11( const int width, const int height );	
	~SimpleGuiDX11();		
	
	int MainLoop();

	static Reservoir& getReservoirRead(const glm::vec<2,int>& coords) {

		return reservoirsPingPong[readReservoirBufferIndex][coords.y * width_ + coords.x];
	}

	static Reservoir& getReservoirWrite(const glm::vec<2,int>& coords){

		return reservoirsPingPong[writeReservoirBufferIndex][coords.y * width_ + coords.x];
	}

	static Reservoir& getReservoirLastFrame(const glm::vec<2, int>& coords) {

		return reservoirsLastFrame[coords.y * width_ + coords.x];
	}

	static RenderPass getCurrentRenderPass()
	{
		return currentRenderPass;
	}

	int InitDevice(const char* config);

	static Reservoir* reservoirsPingPong[2];
	static Reservoir* reservoirsLastFrame;

	static GBuffer gBuffer;
	static GBuffer gBufferLastFrame;

	static int readReservoirBufferIndex;
	static int writeReservoirBufferIndex;

	bool doRestir{false};
	float t;
	

	static RenderPass currentRenderPass;

	static int width_;
	static int height_;
	static glm::vec<2, int> debugPixel;

protected:
	int Init();
	int Cleanup();	

	void CreateRenderTarget();
	void CleanupRenderTarget();
	HRESULT CreateDeviceD3D( HWND hWnd );
	void CleanupDeviceD3D();

	HRESULT CreateTexture();
	LRESULT WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK s_WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );		

	void drawGui() override;
	virtual glm::vec3 get_pixel(const int x, const int y, const float t = 0.0f);

	void Producer();

	void produceStandard(float t);
	void produceRestir(float t);

	int width() const;
	int height() const;

	bool vsync_{ true };

	void handleMouse();

	glm::vec<2, int> imgPos{};
	glm::vec<2, int> imgSize{};
	glm::vec<2, int> mousePos{};

	int accFrameCtr{0};
	int frameCtr{0};

	Integrator* currentIntegrator{};

	std::unique_ptr<Integrator> naivePathIntegrator{};
	std::unique_ptr<Integrator> neePathIntegrator{};

	RenderParams renderParams{};

	RTCDevice device_;
	Scene scene;

	double accumulatorMean{};
	double accumulatorVariance{};
	
	void swapReservoirBuffers(){
		std::swap(writeReservoirBufferIndex, readReservoirBufferIndex);
	};

	long long gBUfferFillDuration{};
	long long initialCandidatesGenDuration{};
	long long visibilityPassDuration{};
	long long temporalReusePassDuration{};
	long long spatialReusePassDuration{};
	long long shadingPassDuration{};
	long long bufferCopyDuration{};
	long long totalFrameDuration{};


	OIDNDevice oidnDevice{};

	oidn::BufferRef oidnColorBuf;
	oidn::BufferRef oidnAlbedoBuf;
	oidn::BufferRef oidnNormalBuf;
	oidn::BufferRef oidnOutBuf;
	oidn::FilterRef oidnFilter;

private:

	WNDCLASSEX wc_;
	HWND hwnd_;

	ID3D11Device * g_pd3dDevice{ nullptr };
	ID3D11DeviceContext * g_pd3dDeviceContext{ nullptr };
	IDXGISwapChain * g_pSwapChain{ nullptr };
	ID3D11RenderTargetView * g_mainRenderTargetView{ nullptr };

	ID3D11Texture2D * tex_id_{ nullptr };
	ID3D11ShaderResourceView * tex_view_{nullptr};

	//accumulator texture - HDR averaged values
	glm::vec3* frame_data{ nullptr };
	glm::vec3* accumulator{ nullptr };

	//data to display - tonemapped and gamma compressed
	glm::vec4* display_data{ nullptr }; // DXGI_FORMAT_R32G32B32A32_FLOAT

	std::mutex tex_data_lock_;
	std::mutex reuse_index_lock_;
		
	std::atomic<bool> finish_request_{ false };

	void initOIDN();

protected:
	Camera camera_;
	glm::vec3 newCamPos{};


	glm::vec<2, int> safeArea{ 10,30 };
	bool accumulate{ false };
	int maxAccCount{ 300000 };
	int autoExportAfterCount{300000};

	void exportImage(float time);
	char exportFileName[100]{};

};