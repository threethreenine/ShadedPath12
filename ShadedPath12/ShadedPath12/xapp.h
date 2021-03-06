#pragma once

class WorldObjectStore;

namespace Colors {
	const XMFLOAT4 xm{ 1.0f, 0.0f, 1.0f, 1.0f };
	const XMFLOAT4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	const XMFLOAT4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const XMFLOAT4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	const XMFLOAT4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	const XMFLOAT4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	const XMFLOAT4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	const XMFLOAT4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	const XMFLOAT4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	const XMFLOAT4 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
	const XMFLOAT4 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
};

class PakEntry {
public:
	long len;    // file length in bytes
	long offset; // offset in pak - will be transferred to absolute
				 // position in pak file on save
	string name; // directory entry - may contain fake folder names
				 // 'sub/t.dds'
	//ifstream *pakFile; // reference to pak file, stream should be open and ready to read at all times
	wstring pakname; // we open and close the pak file for every read, so we store filename here
};

class XAppBase
{
public:
	XAppBase();
	~XAppBase();

	virtual void init();
	//virtual void resize();
	virtual void update();
	virtual void draw();
	virtual void destroy();
	virtual string getWindowTitle() = 0;

protected:
	string myClass;

};

// Multi Scene Apps inherit from this:
class XAppMultiBase
{
	public:
		// inital all apps - call after all apps were added to vector
		void initAllApps();
		virtual void initScenes() = 0;
	protected:
		vector<XAppBase *> apps;	// all apps maintained by this Mult-Scene App
};

// helper class for some statistics like frame timing 
class Stats
{
public:
	void startUpdate(GameTime &gameTime);
	void startDraw(GameTime &gameTime);
	void endUpdate(GameTime &gameTime);
	void endDraw(GameTime &gameTime);

	static const int numFramesGathered = 10 * 3;
	static const int frameNumStartGathering = 10 * 3;
	LONGLONG started[numFramesGathered] = { 0 };
	LONGLONG ended[numFramesGathered] = { 0 };
	virtual ~Stats();

	// measure single methods ans similar
	struct StatTopic {
		string topicName;
		long long cumulated;  // total time spent
		long long called = 0; // number of calls so far
		long long curStart;   // record current invocation time
	};
	unordered_map<string, StatTopic> statTopics;
	void start(string topic);
	void end(string topic);
	StatTopic *get(string name) { return &statTopics[name]; };
	long long getNow();
	wstring getInfo(string name);
};

class XApp
{
public:
	XApp();
	~XApp();

	string buildInfo = "Shaded Path 12 Engine V 0.1.3"; // version text

	void init();
	void initPakFiles();
	void resize();
	void update();
	void draw();
	void destroy();
	void report();
	void calcBackbufferSizeAndAspectRatio();
	// signal shutdown - game loop will end in 3 frames
	void setShutdownMode() { shutdownMode = true; shutdownFrameNumStart = framenum; };
	bool isShutdownMode() { return shutdownMode; };
	bool isShudownFinished() { return shutdownMode && (framenum > (shutdownFrameNumStart + 3)); };

	// asset handling
	enum FileCategory { FX, TEXTURE, MESH, SOUND, TEXTUREPAK };
	// find absolute filename for a name and category, defaults to display error dialog, returns empty filename if not found and errorIfNotFound is set to false,
	// returns full file path if generateFilenameMode == true (use to create files)
	wstring findFile(wstring filename, FileCategory cat, bool errorIfNotFound = true, bool generateFilenameMode = false);
	wstring findFileForCreation(wstring filename, FileCategory cat) { return findFile(filename, cat, false, true); };
	void readFile(wstring filename, vector<byte>& buffer, FileCategory cat);
	void readFile(PakEntry *pakEntry, vector<byte>& buffer, FileCategory cat);

	void registerApp(string name, XAppBase*);
	XAppBase* getApp(string appName);
	void setRunningApp(string appName);
	unordered_map<string, string> parameters;
	void parseCommandLine(string commandline);
	bool getBoolParam(string key, bool default_value = false);
	int getIntParam(string key, int default_value = 0);
	// last shader should call this directly after Present() call
	void frameFinished();
	// check if RTV still has to be cleared - take VR rendering into account
	bool rtvHasToBeCleared() {
		if (rtvCleared) return false;  // nothing to do - already cleared during this frame
		if (!ovrRendering || (ovrRendering /*&& vr.isFirstEye()*/)) {
			return true;
		}
		return false;
	};
	void setBackgroundColor(XMFLOAT4 bgColor) {
		clearColor[0] = bgColor.x;
		clearColor[1] = bgColor.y;
		clearColor[2] = bgColor.z;
		clearColor[3] = bgColor.w;
	};
	void handleRTVClearing(ID3D12GraphicsCommandList *commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle, D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle, ID3D12Resource* resource);

	CD3DX12_CPU_DESCRIPTOR_HANDLE getRTVHandle(int frameIndex) {
#if defined(_OVR_)
		if (ovrRendering) return vr.getRTVHandle(frameIndex);
#endif
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
		return rtvHandle;
	};

	int getCurrentBackBufferIndex() {
		int frameIndex = 0;
		if (!ovrRendering) {
			frameIndex = swapChain->GetCurrentBackBufferIndex();
		} else {
			frameIndex = vr.getCurrentFrameBufferIndex();
		}
		return frameIndex;
	}
	// query virtual key definitions (VK_) from Winuser.h
	bool keyDown(BYTE key);

	void setHWND(HWND h);
	void resize(int width, int height);
	HWND getHWND();
	// command line and parameters
	string commandline;
	HWND hwnd = 0;
	bool ovrRendering = false;   // use split screen ovr rendering
	bool ovrMirror = true;       // if ovrRendering() then this flag indicates if mirroring to app window should occur, always false for non-ovr rendering
	bool warp; //use warp device to render (a.k.a. software rendering)
	// on some systems there is no debug version of DX 11 available,
	// then set -disableDX11Debug on command line to be able to let debug build run
	bool disableDX11Debug = false;
	// Graphics debugging does not like line shaders - it crashes on 2nd line shader initialization
	bool disableLineShaders = false;

	int requestWidth, requestHeight;
	// all drawing takes place in backbuffer - output to whatever size the window currently has is only last step
	unsigned int backbufferWidth = 0, backbufferHeight = 0;
	float aspectRatio;

	// 
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D11Device> d3d11Device;
	ComPtr<ID3D11DeviceContext> d3d11DeviceContext;
	ComPtr<ID3D11Device> reald3d11Device;
	ComPtr<ID3D11DeviceContext> reald3d11DeviceContext;
	ComPtr<ID3D11On12Device> d3d11On12Device;
	ComPtr<ID3D12RootSignature> rootSignature;
	static const UINT FrameCount = 3;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // Resource Target View Heap
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	ComPtr<ID3D11Resource> wrappedBackBuffers[FrameCount];
	ComPtr<ID3D11Texture2D> wrappedTextures[FrameCount];
	ComPtr<ID3D12Resource> depthStencils[FrameCount];
	ComPtr<ID3D12DescriptorHeap> dsvHeaps[FrameCount];
	UINT lastPresentedFrame = 0;  // set directly before Present() to have 'old' frame number available
	BYTE key_state[256];
	int mouseDx;
	int mouseDy;
	bool mouseTodo;
	int fps;
	bool anyKeyDown = false;

	// Other framework instances:
	TextureStore textureStore;
	WorldObjectStore objectStore;
	Lights lights;
	World world;
	Camera camera;
	GameTime gametime;
	Sound sound;
	VR vr;
	IDXGraphicsAnalysis* pGraphicsAnalysis = nullptr; // check for nullpointer before using - only available during graphics diagnostics session
	thread mythread;
	Stats stats;
private:
	bool shutdownMode = false;
	UINT shutdownFrameNumStart;
	long long framenum;
	UINT frameIndex;
	bool rtvCleared = false; // shaders can ask if ClearRenderTargetView still has to be called (usually only first shader needs to)

	unordered_map<string, XAppBase *> appMap;
	bool initialized = false;
	string appName;

	XAppBase *app = nullptr;
	float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

	// pak files:
	unordered_map<string, PakEntry> pak_content;
public:
	// find entry in pak file, return nullptr if not found
	PakEntry* findFileInPak(wstring filename);
	long long getFramenum() { return framenum; };
};

// reference to global instance:
XApp& xapp();
void xappDestroy();