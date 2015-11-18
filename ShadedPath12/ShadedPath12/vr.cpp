#include "stdafx.h"

VR::VR(XApp *xapp) {
	this->xapp = xapp;
}


VR::~VR() {
#if defined(_OVR_)
	ovr_Destroy(session);
	ovr_Shutdown();
#endif
}

void VR::init()
{
#if defined(_OVR_)
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result)) {
		Error(L"ERROR: LibOVR failed to initialize.");
		return;
	}
	Log("LibOVR initialized ok!" << endl);
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		Error(L"No Oculus Rift detected. Cannot run in OVR mode without Oculus Rift device.");
		return;
	}

	desc = ovr_GetHmdDesc(session);
	resolution = desc.Resolution;
	// Start the sensor which provides the Rift�s pose and motion.
	result = ovr_ConfigureTracking(session, ovrTrackingCap_Orientation |
		ovrTrackingCap_MagYawCorrection |
		ovrTrackingCap_Position, 0);
	if (OVR_FAILURE(result)) Error(L"Could not enable Oculus Rift Tracking. Cannot run in OVR mode without Oculus Rift tracking.");

	// Setup VR components, filling out description
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, desc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, desc.DefaultEyeFov[1]);

	nextTracking();

	// tracking setup complete, now init rendering:

	// Configure Stereo settings.
	Sizei recommenedTex0Size = ovr_GetFovTextureSize(session, ovrEye_Left, desc.DefaultEyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovr_GetFovTextureSize(session, ovrEye_Right, desc.DefaultEyeFov[1], 1.0f);
	buffersize_width = recommenedTex0Size.w + recommenedTex1Size.w;
	buffersize_height = max(recommenedTex0Size.h, recommenedTex1Size.h);
#endif
}

void VR::initD3D()
{
#if defined(_OVR_)
	Sizei bufferSize;
	bufferSize.w = buffersize_width;
	bufferSize.h = buffersize_height;


	// xapp->d3d11Device.Get() will not work, we need a real D3D11 device
	
	for (int i = 0; i < xapp->FrameCount; i++) {
		ID3D12Resource *resource = xapp->renderTargets[i].Get();
		D3D12_RESOURCE_DESC rDesc = resource->GetDesc();
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		ThrowIfFailed(xapp->d3d11On12Device->CreateWrappedResource(
			resource,
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&xapp->wrappedBackBuffers[i])
			));
		//xapp->d3d11On12Device->AcquireWrappedResources(xapp->wrappedBackBuffers[i].GetAddressOf(), 1);
	}

	D3D11_TEXTURE2D_DESC dsDesc;
	dsDesc.Width = bufferSize.w;
	dsDesc.Height = bufferSize.h;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	if (ovr_CreateSwapTextureSetD3D11(session, xapp->reald3d11Device.Get(), &dsDesc, 0, &pTextureSet) == ovrSuccess)
	{
		for (int i = 0; i < pTextureSet->TextureCount; ++i)
		{
			ovrD3D11Texture* tex = (ovrD3D11Texture*)&pTextureSet->Textures[i];
			ComPtr<IDXGIResource> dxgires;
			tex->D3D11.pTexture->QueryInterface<IDXGIResource>(&dxgires);
			//Log("dxgires = " << dxgires.GetAddressOf() << endl);
			HANDLE shHandle;
			dxgires->GetSharedHandle(&shHandle);
			//Log("shared handle = " << shHandle << endl);
			xapp->d3d11Device->OpenSharedResource(shHandle, IID_PPV_ARGS(&xapp->wrappedTextures[i]));
			//xapp->reald3d11Device->CreateRenderTargetView(tex->D3D11.pTexture, NULL, &pTexRtv[i]);
		}
	}
	// Initialize our single full screen Fov layer.
	layer.Header.Type = ovrLayerType_EyeFov;
	layer.Header.Flags = 0;
	layer.ColorTexture[0] = pTextureSet;
	layer.ColorTexture[1] = nullptr;
	layer.Fov[0] = eyeRenderDesc[0].Fov;
	layer.Fov[1] = eyeRenderDesc[1].Fov;
	layer.Viewport[0] = Recti(0, 0, bufferSize.w / 2, bufferSize.h);
	layer.Viewport[1] = Recti(bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
	// ld.RenderPose and ld.SensorSampleTime are updated later per frame.
#endif
}

void VR::initFrame()
{
}

void VR::startFrame()
{
	curEye = EyeLeft;
}

void VR::endFrame()
{
}

void VR::prepareViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect)
{
	if (!enabled) {
		viewports[EyeLeft] = viewport;
		scissorRects[EyeLeft] = scissorRect;
		viewports[EyeRight] = viewport;
		scissorRects[EyeRight] = scissorRect;
	} else {
		//orig_viewport = viewport;
		//orig_scissorRect = scissorRect;
		viewports[EyeLeft] = viewport;
		scissorRects[EyeLeft] = scissorRect;
		viewports[EyeRight] = viewport;
		scissorRects[EyeRight] = scissorRect;
		viewports[EyeLeft].Width /= 2;
		scissorRects[EyeLeft].right /= 2;
		//scissorRects[EyeLeft].right--;
		viewports[EyeRight].Width /= 2;
		viewports[EyeRight].TopLeftX = viewports[EyeLeft].Width;
		scissorRects[EyeRight].left = scissorRects[EyeLeft].right;
	}
}

void VR::prepareDraw()
{
	//cam_pos = xapp->camera.pos;
	//cam_look = xapp->camera.look;
	//cam_up = xapp->camera.up;
	firstEye = true;
}

void VR::endDraw() {
	//xapp->camera.pos = cam_pos;
	//xapp->camera.look = cam_look;
	//xapp->camera.up = cam_up;
	//xapp->camera.worldViewProjection();
}

void VR::adjustEyeMatrix(XMMATRIX &m) {
	m = xapp->camera.worldViewProjection();
	//Camera c2 = xapp->camera;
	//if (curEye == EyeLeft) {
	//	c2.pos.x = cam_pos.x - 0.5f;
	//	c2.pos.y = cam_pos.y + 0.3f;
	//}
	//else {
	//	c2.pos.x = cam_pos.x + 0.5f;
	//	c2.pos.y = cam_pos.y + 0.3f;
	//}
	//m = c2.worldViewProjection();
	//c2.apply_pitch_yaw();
	//if (curEye == EyeLeft) {
	//	xapp->camera.pos.x = cam_pos.x - 0.5f;
	//	xapp->camera.pos.y = cam_pos.x + 0.3f;
	//} else {
	//	xapp->camera.pos.x = cam_pos.x + 0.5f;
	//	xapp->camera.pos.y = cam_pos.x + 0.3f;
	//}
	//m = xapp->camera.worldViewProjection();
	//xapp->camera.apply_pitch_yaw();
}

void VR::nextEye() {
	if (curEye == EyeLeft) curEye = EyeRight;
	else curEye = EyeLeft;
	firstEye = false;
}

bool VR::isFirstEye() {
	return firstEye;
}

void Matrix4fToXM(XMFLOAT4X4 &xm, Matrix4f &m) {
	xm._11 = m.M[0][0];
	xm._12 = m.M[0][1];
	xm._13 = m.M[0][2];
	xm._14 = m.M[0][3];
	xm._21 = m.M[1][0];
	xm._22 = m.M[1][1];
	xm._23 = m.M[1][2];
	xm._24 = m.M[1][3];
	xm._31 = m.M[2][0];
	xm._32 = m.M[2][1];
	xm._33 = m.M[2][2];
	xm._34 = m.M[2][3];
	xm._41 = m.M[3][0];
	xm._42 = m.M[3][1];
	xm._43 = m.M[3][2];
	xm._44 = m.M[3][3];
}

void VR::nextTracking()
{
	// Get both eye poses simultaneously, with IPD offset already included. 
	ovrVector3f useHmdToEyeViewOffset[2] = { eyeRenderDesc[0].HmdToEyeViewOffset, eyeRenderDesc[1].HmdToEyeViewOffset };
	//ovrPosef temp_EyeRenderPose[2];
	ovrTrackingState ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), false);
	ovr_CalcEyePoses(ts.HeadPose.ThePose, useHmdToEyeViewOffset, layer.RenderPose);

	// Render the two undistorted eye views into their render buffers.  
	for (int eye = 0; eye < 2; eye++)
	{
		ovrPosef    * useEyePose = &EyeRenderPose[eye];
		float       * useYaw = &YawAtRender[eye];
		float Yaw = XM_PI;
		*useEyePose = layer.RenderPose[eye];
		*useYaw = Yaw;

		// Get view and projection matrices (note near Z to reduce eye strain)
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(useEyePose->Orientation);
		// fix finalRollPitchYaw for LH coordinate system:
		Matrix4f s = Matrix4f::Scaling(1.0f, -1.0f, -1.0f);  // 1 1 -1
		finalRollPitchYaw = s * finalRollPitchYaw * s;

		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));//0 0 1
		Vector3f Posf;
		Posf.x = xapp->camera.pos.x;
		Posf.y = xapp->camera.pos.y;
		Posf.z = xapp->camera.pos.z;
		Vector3f diff = rollPitchYaw.Transform(useEyePose->Position);
		Vector3f shiftedEyePos;
		shiftedEyePos.x = Posf.x - diff.x;
		shiftedEyePos.y = Posf.y + diff.y;
		shiftedEyePos.z = Posf.z + diff.z;
		xapp->camera.look.x = finalForward.x;
		xapp->camera.look.y = finalForward.y;
		xapp->camera.look.z = finalForward.z;

		Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f projO = ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.2f, 2000.0f, false);
		Matrix4fToXM(this->viewOVR[eye], view.Transposed());
		Matrix4fToXM(this->projOVR[eye], projO.Transposed());
	}
}

void VR::submitFrame()
{
	UINT frameIndex = xapp->swapChain->GetCurrentBackBufferIndex();

	// Increment to use next texture, just before writing
	pTextureSet->CurrentIndex = (pTextureSet->CurrentIndex + 1) % pTextureSet->TextureCount;
	xapp->d3d11On12Device->AcquireWrappedResources(xapp->wrappedBackBuffers[frameIndex].GetAddressOf(), 1);
	xapp->d3d11DeviceContext->CopyResource(xapp->wrappedTextures[pTextureSet->CurrentIndex].Get(), xapp->wrappedBackBuffers[frameIndex].Get());
	xapp->d3d11On12Device->ReleaseWrappedResources(xapp->wrappedBackBuffers[frameIndex].GetAddressOf(), 1);
	xapp->d3d11DeviceContext->Flush();

	// Submit frame with one layer we have.
	ovrLayerHeader* layers = &layer.Header;
	ovrResult       result = ovr_SubmitFrame(session, 0, nullptr, &layers, 1);
	bool isVisible = (result == ovrSuccess); 
	//Log
}