#include "stdafx.h"
#include "simpleguidx11.h"

#include <complex>
#include <iostream>
#include <fstream>
#include <chrono>

#include "raytracer.h"
#include "Utils.h"
#include "glm/gtx/string_cast.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "NEEPathIntegrator.h"
#include "ReSTIRIntegrator.h"
#include "stb_image_write.h"
#include "tutorials.h"
#include "oidn.hpp"


Reservoir* SimpleGuiDX11::reservoirsPingPong[2]{nullptr,nullptr};
Reservoir* SimpleGuiDX11::reservoirsLastFrame{nullptr};
GBuffer SimpleGuiDX11::gBuffer{};
GBuffer SimpleGuiDX11::gBufferLastFrame{};
RenderPass SimpleGuiDX11::currentRenderPass{ INITIAL };

int SimpleGuiDX11::readReservoirBufferIndex{1};
int SimpleGuiDX11::writeReservoirBufferIndex{0};
glm::vec<2,int> SimpleGuiDX11::debugPixel{ 320, 240};
int SimpleGuiDX11::width_{ 640 };
int SimpleGuiDX11::height_{ 480 };

SimpleGuiDX11::SimpleGuiDX11( const int width, const int height) {
	width_ = width;
	height_ = height;

	Init();

	naivePathIntegrator = std::make_unique<NaivePathIntegrator>(renderParams);
	neePathIntegrator = std::make_unique<NEEPathIntegrator>(renderParams);

	currentIntegrator = neePathIntegrator.get();

	gBuffer = GBuffer{ {width_,height_} };
	gBufferLastFrame = GBuffer{ {width_,height_} };


	InitDevice("threads=0,verbose=3");
	initOIDN();
}

void SimpleGuiDX11::initOIDN() {

	oidnDevice = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
	oidnCommitDevice(oidnDevice);

	oidnColorBuf = oidnNewSharedBuffer(oidnDevice, accumulator, width_ * height_ * sizeof(glm::vec3));
	oidnAlbedoBuf = oidnNewSharedBuffer(oidnDevice,gBuffer.diffuseColorBuf.data(), width_ * height_ * sizeof(glm::vec3));
	oidnNormalBuf = oidnNewSharedBuffer(oidnDevice,gBuffer.wSpaceNormalBuf.data(), width_ * height_ * sizeof(glm::vec3));
	oidnOutBuf = oidnNewBuffer(oidnDevice, width_ * height_ * sizeof(glm::vec3));


	oidnFilter = oidnNewFilter(oidnDevice, "RT");; // generic ray tracing filter
	oidnFilter.setImage("color", oidnColorBuf, oidn::Format::Float3, width_, height_); // beauty
	oidnFilter.setImage("albedo", oidnAlbedoBuf, oidn::Format::Float3, width_, height_); // auxiliary
	oidnFilter.setImage("normal", oidnNormalBuf, oidn::Format::Float3, width_, height_); // auxiliary
	oidnFilter.setImage("output", oidnOutBuf, oidn::Format::Float3, width_, height_); // denoised beauty
	oidnFilter.set("hdr", true);
	oidnFilter.set("quality", oidn::Quality::High);
	oidnFilter.commit();

	const char* errorMessage;
	if (oidnGetDeviceError(oidnDevice, &errorMessage) != OIDN_ERROR_NONE)
		printf("Error: %s\n", errorMessage);
}

int SimpleGuiDX11::Init()
{
	// Create application window
	wc_ = { sizeof( WNDCLASSEX ), CS_CLASSDC, s_WndProc, 0L, 0L,
		GetModuleHandle( NULL ), NULL, NULL, NULL, NULL, _T( "ImGui Example" ), NULL };
	RegisterClassEx( &wc_ );
	hwnd_ = CreateWindow( _T( "ImGui Example" ), _T( "PG1 Ray Tracer" ),
		WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc_.hInstance, this );

	// Initialize Direct3D
	if ( CreateDeviceD3D( hwnd_ ) < 0 )
	{
		CleanupDeviceD3D();
		UnregisterClass( _T( "ImGui Example" ), wc_.hInstance );
		return S_OK;
	}

	// Show the window
	ShowWindow( hwnd_, SW_SHOWDEFAULT );
	UpdateWindow( hwnd_ );

	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	//ImGuiIO & io = ImGui::GetIO(); ( void )io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Initialize helper Platform and Renderer bindings
	// (here we are using imgui_impl_win32 and imgui_impl_dx11)
	ImGui_ImplWin32_Init( hwnd_ );
	ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	frame_data = new glm::vec3[width_ * height_]{};
	accumulator = new glm::vec3[width_ * height_]{};
	display_data = new  glm::vec4[width_ * height_]{};

	reservoirsPingPong[0] = new Reservoir[width_ * height_]{};
	reservoirsPingPong[1] = new Reservoir[width_ * height_]{};
	reservoirsLastFrame = new Reservoir[width_ * height_]{};

	doRestir = true;

	CreateTexture();

	return 0;
}

SimpleGuiDX11::~SimpleGuiDX11() {
	Cleanup();
	 
	delete[] accumulator;
	accumulator = nullptr;

	delete[] display_data;
	display_data = nullptr;

	delete[] reservoirsPingPong[0];
	reservoirsPingPong[0] = nullptr;

	delete[] reservoirsPingPong[1];
	reservoirsPingPong[1] = nullptr;

	delete[] reservoirsPingPong;
	delete[] reservoirsLastFrame;
}

int SimpleGuiDX11::Cleanup() {

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	DestroyWindow( hwnd_ );
	UnregisterClass( _T( "ImGui Example" ), wc_.hInstance );

	return 0;

}

void SimpleGuiDX11::drawGui() {
	ImGui::Begin("Ray Tracer Params");
	ImGui::Checkbox("Accumulate", &accumulate);

	if (accumulate) {
		ImGui::Text(std::to_string(accFrameCtr).c_str());
		ImGui::Text( ("Time: " + std::to_string(t) + " s").c_str()	);
	}
	ImGui::DragInt("Max iteration count",&maxAccCount,1,1);
	if (ImGui::Button("Reset accumulator"))
	{
		{
			std::lock_guard<std::mutex> lock(tex_data_lock_);
			accFrameCtr = 0;
			memset(accumulator, 0, width_ * height_ * 3 * sizeof(float));
			memset(display_data, 0, width_ * height_ * 3 * sizeof(float));
		}
		
	}
	ImGui::InputText("exported file name",exportFileName,sizeof(exportFileName));
	ImGui::DragInt("Auto export after number of iterations:", &autoExportAfterCount,1,0,INT_MAX);

	if( ImGui::Button("export now"))
		exportImage(t);

	ImGui::DragInt2("Debug pixel position", &debugPixel[0], 1, 0, 640);

	static const char* items[]{ /*"Ray Tracer", */"Naive Path Tracer", "NEE Path Tracer", "ReSTIR test" };
	static int integratorIndex = 2;

	//bool headerOpen = ImGui::CollapsingHeader("Integrator", ImGuiTreeNodeFlags_CollapsingHeader);

	if (true) {
		
		if (ImGui::Combo("Integrator", &integratorIndex, items, IM_ARRAYSIZE(items))) {
			switch (integratorIndex) {
			case 0:
				currentIntegrator = naivePathIntegrator.get();
				this->doRestir = false;
				break;
			case 1:
				currentIntegrator = neePathIntegrator.get();
				this->doRestir = false;
				break;
			default:
			case 2:
				this->doRestir = true;
				break;
			}
		}

		if (doRestir)
			ReSTIRIntegrator::gui2();
		else
			currentIntegrator->drawGui();
	}
}

glm::vec3 SimpleGuiDX11::get_pixel(const int x, const int y, const float t) {
	return glm::vec4{ 1.0f, 0.0f, 1.0f, 1.0f };
}

void SimpleGuiDX11::Producer() {

	t = 0.0f; // time
	auto t0 = std::chrono::high_resolution_clock::now();

	while ( !finish_request_.load( std::memory_order_acquire ) )
	{

		//grab new position from GUI, render the frame with it
		camera_.setPosition(newCamPos);
		camera_.recalculate_m_c_w();

		auto t1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> dt = t1 - t0;
		t += dt.count();
		t0 = t1;

		if (doRestir)
			produceRestir(t);
		else
			produceStandard(t);


		for (int y = 0; y < height_; ++y) {
			for (int x = 0; x < width_; ++x) {

				const int offset = y * width_ + x;

				accumulator[offset] = glm::mix(accumulator[offset], frame_data[offset], 1.0f / static_cast<float>(accFrameCtr + 1));
			}
		}

		oidnFilter.commit();
		oidnFilter.execute();

		const char* errorMessage;
		if (oidnGetDeviceError(oidnDevice, &errorMessage) != OIDN_ERROR_NONE)
			printf("Error: %s\n", errorMessage);

		{
			std::lock_guard<std::mutex> lock(tex_data_lock_);

			#ifndef _DEBUG
				#pragma omp parallel for
			#endif
			for (int y = 0; y < height_; ++y) {
				for (int x = 0; x < width_; ++x) {

					const int offset = y * width_ + x;

					auto denoisedBuf = static_cast<glm::vec3*>(oidnOutBuf.getData());
					//auto denoisedBuf = static_cast<glm::vec3*>(oidnAlbedoBuf.getData());

					glm::vec3 pix{};
					if (renderParams.denoise)
						pix = denoisedBuf[offset];
					else
						pix = accumulator[offset];
			
					//tonemap, gamma correct for displaying
					if (renderParams.tonemap)
						Utils::aces(pix);

					if (Raytracer::gammaCorrect)
						Utils::compress(pix);

					if (glm::vec<2,int>{x,y} == debugPixel)
						display_data[offset] = { 1,0,1,1 };
					else
						display_data[offset] = glm::vec4{ pix,1.0f };
				}
			}
		}

		accFrameCtr++;
		frameCtr++;

		if (accFrameCtr > maxAccCount)
			accumulate = false;

		if (!accumulate) {
			accFrameCtr = 0;
			t = 0.0f;
		}

		double pixelSum{0};
		double pixelSqrSum{0};

		#ifndef  _DEBUG
			#pragma omp parallel for reduction(+:pixelSum,pixelSqrSum)
		#endif
		for (int y = 0; y < height_; ++y) {
			for (int x = 0; x < width_; ++x) {
				const int offset = (y * width_ + x) ;

				glm::vec3 pixel{ accumulator[offset]};
				float pixelMean = (pixel.x + pixel.y + pixel.z) / 3.0f;
				pixelSum += pixelMean;
				pixelSqrSum += pixelMean * pixelMean;
			}
		}

		accumulatorMean = pixelSum / static_cast<double>(width_ * height_);
		double accSqrMean = pixelSqrSum / static_cast<double>(width_ * height_);

		//D(X) = E(X^2) - E(X)^2
		accumulatorVariance = accSqrMean - accumulatorMean * accumulatorMean;

		if (accFrameCtr == autoExportAfterCount)
			exportImage(t);
	}
}

void SimpleGuiDX11::produceStandard(float t) {
	#ifndef _DEBUG
		# pragma omp parallel for
	#endif

	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {

			const int offset = y * width_ + x;

			glm::vec3 pixel = get_pixel(x, y, t);

			Integrator::sanitize(pixel);

			frame_data[offset] = pixel;

			if (glm::vec<2, int>{x, y} == debugPixel){
				frame_data[offset] = {10,0,10};
			}
		}
	}
}

void SimpleGuiDX11::produceRestir(float t) {

	auto t0 = std::chrono::high_resolution_clock::now();
	gBuffer.setViewMat(camera_.getViewMat());
	gBuffer.setInvViewMat(camera_.getInvViewMat());
	gBuffer.setCameraPos(camera_.getPosition());
	gBuffer.setFocalLength(camera_.getFocalLength());

	//initial G buffer fill pass
	#ifndef _DEBUG
		# pragma omp parallel for
	#endif
	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {
			ReSTIRIntegrator::gBufferFillPass(scene, { x,y },camera_);
		}
	}

	auto tGbufferFill = std::chrono::high_resolution_clock::now();
	gBUfferFillDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tGbufferFill - t0).count();

	//initial candidate picking pass
	#ifndef _DEBUG
		# pragma omp parallel for
	#endif
	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {
				ReSTIRIntegrator::initialRenderPass(scene, { x,y });
		}
	}

	auto tInitCandidates = std::chrono::high_resolution_clock::now();
	initialCandidatesGenDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tInitCandidates - tGbufferFill).count();

	if (ReSTIRIntegrator::doVisibilityPass){
		#ifndef _DEBUG
			# pragma omp parallel for
		#endif
		for (int y = 0; y < height_; ++y) {
			for (int x = 0; x < width_; ++x) {
				ReSTIRIntegrator::visibilityPass(scene, { x,y });
			}
		}
	}

	auto tVisibility  = std::chrono::high_resolution_clock::now();
	visibilityPassDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tVisibility -tInitCandidates).count();

	//temporal reuse pass
	if (ReSTIRIntegrator::doTemporalReuse && frameCtr > 0) {
		swapReservoirBuffers();

		#ifndef _DEBUG
			# pragma omp parallel for
		#endif

		for (int y = 0; y < height_; ++y) {
			for (int x = 0; x < width_; ++x) {
				ReSTIRIntegrator::temporalReusePass({ x,y }, scene, camera_);
			}
		}
	}

	auto tTemporal = std::chrono::high_resolution_clock::now();
	temporalReusePassDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tTemporal - tVisibility).count();

	//assert(ReSTIRIntegrator::testReprojection());

	//spatial reuse pass
	if (ReSTIRIntegrator::doSpatialReuse) {
		for (int i = 0; i < ReSTIRIntegrator::spatialPassCount; ++i){
			swapReservoirBuffers();

			#ifndef _DEBUG
				# pragma omp parallel for
			#endif

			for (int y = 0; y < height_; ++y) {
				for (int x = 0; x < width_; ++x) {
					ReSTIRIntegrator::spatialReusePass({ x,y }, scene);
					//ReSTIRIntegrator::spatialReusePassDebug({ x,y }, scene);
				}
			}
		}
	}
	auto tSpatial = std::chrono::high_resolution_clock::now();
	spatialReusePassDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tSpatial - tTemporal).count();

	swapReservoirBuffers();
	//final shading evaluation
	#ifndef _DEBUG
		# pragma omp parallel for
	#endif
	for (int y = 0; y < height_; ++y) {
		for (int x = 0; x < width_; ++x) {
			const int offset = (y * width_ + x);


			glm::vec3 pixel{};

			Reservoir& r = getReservoirRead({ x,y });

			if (r.hasSample()) {
				glm::vec3 f_value = ReSTIRIntegrator::evaluateF(r.bestSample, scene, gBuffer.getCameraPos(), gBuffer.getAt({x,y}), true);
				pixel = f_value * r.W;;
			}
			else
				pixel = gBuffer.getAt({x,y}).emission;

			Integrator::sanitize(pixel);

			frame_data[offset] = pixel;
		}
	}

	auto tShading = std::chrono::high_resolution_clock::now();
	shadingPassDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tShading - tSpatial).count();

	//copy last valid reservoir buffer to last frame buffer
	memcpy(reservoirsLastFrame, reservoirsPingPong[readReservoirBufferIndex], width_ * height_ * sizeof(Reservoir));

	//copy gBuffer
	gBufferLastFrame.setDataFrom(gBuffer);

	auto tBufferCopy = std::chrono::high_resolution_clock::now();
	bufferCopyDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tBufferCopy -tShading).count();

	totalFrameDuration = gBUfferFillDuration + initialCandidatesGenDuration + visibilityPassDuration + spatialReusePassDuration + temporalReusePassDuration + shadingPassDuration + bufferCopyDuration;
}

int SimpleGuiDX11::width() const {
	return width_;
}

int SimpleGuiDX11::height() const {
	return height_;
}

int SimpleGuiDX11::MainLoop() {
	// start image producing threads
	std::thread producer_thread( &SimpleGuiDX11::Producer, this );
	BOOL r = SetThreadPriority( producer_thread.native_handle(), THREAD_PRIORITY_NORMAL );

	// and enter message loop
	MSG msg;
	ZeroMemory( &msg, sizeof( msg ) );
	while ( msg.message != WM_QUIT ) {
		// Poll and handle messages (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		if ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) ) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		drawGui();
		handleMouse();

		{
			D3D11_MAPPED_SUBRESOURCE mapped;
			ZeroMemory( &mapped, sizeof( mapped ) );
			HRESULT hr = g_pd3dDeviceContext->Map( tex_id_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ); // D3D11_MAP_WRITE, D3D11_MAP_WRITE_DISCARD
			{
				std::lock_guard<std::mutex> lock( tex_data_lock_ );
				memcpy( mapped.pData, display_data, mapped.RowPitch * height_ );
			}
			g_pd3dDeviceContext->Unmap( tex_id_, 0 );
		}


		ImGui::Begin( "Image", 0, ImGuiWindowFlags_None );
        ImVec2 available_size = ImGui::GetContentRegionAvail();
        ImGui::Image( ImTextureID( tex_view_ ), available_size );
		ImVec2 p = ImGui::GetWindowPos();
		auto size = ImGui::GetWindowSize();
        ImGui::End();


		//ImGui_ImplDX11_RenderDrawData()
		// Rendering
		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets( 1, &g_mainRenderTargetView, NULL );
		const FLOAT clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };		
		g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView, ( float* )&clear_color );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
		
		g_pSwapChain->Present( ( ( vsync_ ) ? 1 : 0 ), 0 ); // present with or without vsync
	}

	finish_request_.store( true, std::memory_order_release );
	producer_thread.join();

	return 0;
}

int SimpleGuiDX11::InitDevice(const char* config){
	device_ = rtcNewDevice(config);
	error_handler(nullptr, rtcGetDeviceError(device_), "Unable to create a new device.\n");
	rtcSetDeviceErrorFunction(device_, error_handler, nullptr);

	ssize_t triangle_supported = rtcGetDeviceProperty(device_, RTC_DEVICE_PROPERTY_TRIANGLE_GEOMETRY_SUPPORTED);

	return S_OK;
}

void SimpleGuiDX11::handleMouse() {
	if (ImGui::GetIO().MouseDown[1]) {

		//delta in spherical coordinate angles
		auto mouseDelta = ImGui::GetMouseDragDelta(1);

		glm::vec3 pos =  camera_.getPosition() - camera_.getViewAt();
		glm::vec3 sphericalPos = Utils::cartesianToSpherical(pos);

		sphericalPos.x -= mouseDelta.x * 0.0001f;
		sphericalPos.y -= mouseDelta.y * 0.0001f;
		sphericalPos.z -= ImGui::GetIO().MouseWheel * 0.5f;

		newCamPos = Utils::sphericalToCartesian(sphericalPos) + camera_.getViewAt();
	}

	else if (ImGui::GetIO().MouseDown[2]) {
		auto mouseDelta = ImGui::GetMouseDragDelta(2);

		glm::vec3 cameraSpaceOffset{ mouseDelta.x,mouseDelta.y,0.0f };
		cameraSpaceOffset *= 0.0001f;

		if (glm::length(cameraSpaceOffset) > 0.0f)
			int h = 10;

		glm::vec3 worldSpaceOffset{ camera_.getViewMat() * glm::vec4(cameraSpaceOffset,1.0 )};

		/*camera_.setPosition(camera_.getPosition() - worldSpaceOffset);
		camera_.setViewAt(camera_.getViewAt() - worldSpaceOffset);
		std::cout << glm::to_string(camera_.getViewAt()) << std::endl;
		camera_.recalculate_m_c_w();*/
	}
}


void SimpleGuiDX11::exportImage(float time) {

	//create image
	std::vector<glm::vec<4,uint8_t>> byteImage{};
	byteImage.reserve(width_ * height_);

	char prepend[] = "../../../screenshots/";
	std::string fullRelPath{ prepend };

	{
		std::lock_guard<std::mutex> lock(tex_data_lock_);

		for (int i = 0; i < width_ * height_; i++) {
			byteImage.push_back(display_data[i] * 255.0f);
		}

	}
	fullRelPath.append(exportFileName);
	stbi_write_png(fullRelPath.c_str(), width_, height_, 4, byteImage.data(), width_ * 4);
	
	std::ofstream outfile{ fullRelPath + ".txt" };

	outfile << "Image name: " << fullRelPath << "\n\n";
	outfile << "Iteration count: " << accFrameCtr << "\n";
	outfile << "Area samples: " << ReSTIRIntegrator::M_Area << "\n";
	outfile << "BRDF samples: " << ReSTIRIntegrator::M_Brdf << "\n\n";
	outfile << "Spatial reuse: " << (ReSTIRIntegrator::doSpatialReuse ? "True" : "False") << "\n";

	outfile << "\tPass count: " << ReSTIRIntegrator::spatialPassCount << "\n";
	outfile << "\tNeighbor count: " << ReSTIRIntegrator::spatialReuseNeighborCount << "\n";
	outfile << "\tReuse radius: " << ReSTIRIntegrator::spatialReuseRadius<< "\n\n";

	outfile << "Temporal reuse: " << (ReSTIRIntegrator::doTemporalReuse ? "True" : "False") << "\n\n";

	outfile << "Render time: " << time << " s" << "\n";
	outfile << "Image mean: " << accumulatorMean << "\n";
	outfile << "Image variance: " << accumulatorVariance << "\n\n";

	outfile << "Camera position: " << glm::to_string (camera_.getPosition()) << "\n";
	outfile << "Camera view at: " << glm::to_string(camera_.getViewAt()) << "\n";
	outfile << "Camera vertical FOV: " << camera_.getFOV() << std::endl;

	outfile.close();
}

void SimpleGuiDX11::CreateRenderTarget()
{
	ID3D11Texture2D * pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID * )&pBackBuffer );
	g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_mainRenderTargetView );
	pBackBuffer->Release();
}

void SimpleGuiDX11::CleanupRenderTarget()
{
	if ( g_mainRenderTargetView )
	{
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

HRESULT SimpleGuiDX11::CreateDeviceD3D( HWND hWnd )
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
	if ( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION,
		&sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext ) != S_OK )
	{
		return E_FAIL;
	}
	
	CreateRenderTarget();

	return S_OK;
}

void SimpleGuiDX11::CleanupDeviceD3D()
{
	CleanupRenderTarget();

	if ( g_pSwapChain )
	{
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}

	if ( g_pd3dDeviceContext )
	{
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = nullptr;
	}

	if ( g_pd3dDevice )
	{
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}
}

HRESULT SimpleGuiDX11::CreateTexture()
{
	if ( tex_id_ == nullptr )
	{
		// set up texture description
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory( &desc, sizeof( desc ) );
		desc.Width = width_;
		desc.Height = height_;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		// set up initial data description for the texture
		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory( &initData, sizeof( initData ) );
		initData.pSysMem = ( void * )display_data;
		initData.SysMemPitch = width_ * ( 3 * sizeof( float ) );
		initData.SysMemSlicePitch = height_ * initData.SysMemPitch;

		// create the texture
		HRESULT hr = g_pd3dDevice->CreateTexture2D( &desc, &initData, &tex_id_ );

		//https://github.com/ocornut/imgui/issues/1877
		//https://github.com/ocornut/imgui/issues/1265

		// create a view of the texture
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory( &srvDesc, sizeof( srvDesc ) );
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
			srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Format = desc.Format;
		hr = g_pd3dDevice->CreateShaderResourceView( tex_id_, &srvDesc, &tex_view_ );

		return hr;
	}

	return S_OK;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

LRESULT SimpleGuiDX11::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
		return true;

	switch ( msg )
	{
	case WM_SIZE:
		if ( g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED )
		{
			ImGui_ImplDX11_InvalidateDeviceObjects();
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers( 0, ( UINT )LOWORD( lParam ), ( UINT )HIWORD( lParam ), DXGI_FORMAT_UNKNOWN, 0 );
			CreateRenderTarget();
			ImGui_ImplDX11_CreateDeviceObjects();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ( ( wParam & 0xfff0 ) == SC_KEYMENU ) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

// https://blogs.msdn.microsoft.com/oldnewthing/20140203-00/?p=1893/
LRESULT CALLBACK SimpleGuiDX11::s_WndProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	SimpleGuiDX11 * pThis = nullptr; // our "this" pointer will go here

	if ( uMsg == WM_NCCREATE )
	{
		// Recover the "this" pointer which was passed as a parameter
		// to CreateWindow(Ex).
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>( lParam );
		pThis = static_cast<SimpleGuiDX11*>( lpcs->lpCreateParams );
		// Put the value in a safe place for future use
		SetWindowLongPtr( hwnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>( pThis ) );
	}
	else
	{
		// Recover the "this" pointer from where our WM_NCCREATE handler
		// stashed it.
		pThis = reinterpret_cast<SimpleGuiDX11*>(
			GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	}

	if ( pThis )
	{
		// Now that we have recovered our "this" pointer, let the
		// member function finish the job.
		return pThis->WndProc( hwnd, uMsg, wParam, lParam );
	}

	// We don't know what our "this" pointer is, so just do the default
	// thing. Hopefully, we didn't need to customize the behavior yet.
	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}