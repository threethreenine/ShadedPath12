#include "stdafx.h"
#include "testDotcross.h"


TestDotcross::TestDotcross() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


TestDotcross::~TestDotcross()
{
}

string TestDotcross::getWindowTitle() {
	return "DX12 Sample - Test Dotcross shader";
}

void TestDotcross::init()
{
	dotcrossEffect.init();
	//linesEffect.init();
	postEffect.init();
	float aspectRatio = xapp().aspectRatio;

	LineDef myLines[] = {
		// start, end, color
		{ XMFLOAT3(0.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT3(0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(-0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(0.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};
	vector<LineDef> lines;
	// add all intializer objects to vector:
	for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l);});
	//linesEffect.add(lines);
	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	// most from old kitchen.cpp
	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	//xapp->camera.pos.z = -3.0f;
	//xapp().camera.pos = XMFLOAT4(1.0f, 1.7f, 1.0f, 0.0f);
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;

	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	Grid *g = xapp().world.createWorldGrid(10.0f);
	//linesEffect.add(g->lines);
	XMFLOAT3 myPoints[] = {
		XMFLOAT3(0.1f, 0.1f, 0.1f)
	};
	vector<XMFLOAT3> crossPoints;
	for_each(begin(myPoints), end(myPoints), [&crossPoints](XMFLOAT3 p) {crossPoints.push_back(p); });
	dotcrossEffect.update(crossPoints);
	autoAdd = true; // set to false for disabling auto cross add
}

void TestDotcross::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (gameTime.getSecondsBetween(startTime, now) > 1) {
		startTime = now;
		vector<XMFLOAT3> addCrossPoints;
		for (int i = 0; i < 100; i++) {
			XMFLOAT3 rnd = xapp().world.getRandomPos();
			addCrossPoints.push_back(rnd);
		}
		dotcrossEffect.update(addCrossPoints);
		Log("# crosses: " << dotcrossEffect.points.size() << endl);
	}
	//linesEffect.update();
	dotcrossEffect.update();
	// WVP is now updated automatically during draw()
	//LinesEffect::CBV c;
	//XMStoreFloat4x4(&c.wvp, xapp().camera.worldViewProjection());
	//linesEffect.updateCBV(c);
}

void TestDotcross::draw()
{
	//linesEffect.draw(); // ALWAYS draw lineseffect first as it clears the screen
	dotcrossEffect.draw();
	postEffect.draw();
}

void TestDotcross::destroy()
{
	//linesEffect.destroy();
}

static TestDotcross testDotcross;