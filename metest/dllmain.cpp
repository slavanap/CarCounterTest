#define _USE_MATH_DEFINES
#include <math.h>

#include <Windows.h>
#include <vector>

#include <common.h>
#include <Tools.WinApi.hpp>
#include <Tools.Motion.hpp>

class Sample : GenericVideoFilter {
public:
	PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;
	static AvsParams CreateParams;
	static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
private:
	Tools::Motion::Map mmap;
	Tools::Array2D<char> constMask;
	Sample(IScriptEnvironment* env, PClip input);
};

AvsParams Sample::CreateParams = "c";

AVSValue Sample::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Sample(env, args[0].AsClip());
}

Sample::Sample(IScriptEnvironment* env, PClip input) : GenericVideoFilter(input) {
	vi.num_frames--;
	vi.width = DIV_AND_ROUND(vi.width, Tools::Motion::BLOCK_SIZE);
	vi.height = DIV_AND_ROUND(vi.height, Tools::Motion::BLOCK_SIZE);
	mmap.resize(vi.width, vi.height);
	constMask.resize(vi.width, vi.height);
	memset(&constMask[0], 1, constMask.size());
}

Tools::AviSynth::Frame::Pixel hsv2rgb(float h, float s, float v) {
	if (s <= 0.0)
		return Tools::AviSynth::Frame::Pixel(v, v, v);
	if (h < 0.0f)
		h += 360.0f;
	else if (h >= 360.0f)
		h = 0.0;
	h /= 60.0;
	int i = (int)h;
	float ff = h - i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - (s * ff));
	float t = v * (1.0f - (s * (1.0f - ff)));

	switch (i) {
		case 0: return Tools::AviSynth::Frame::Pixel(v, t, p);
		case 1: return Tools::AviSynth::Frame::Pixel(q, v, p);
		case 2: return Tools::AviSynth::Frame::Pixel(p, v, t);
		case 3: return Tools::AviSynth::Frame::Pixel(p, q, v);
		case 4: return Tools::AviSynth::Frame::Pixel(t, p, v);
		case 5: 
		default:
			return Tools::AviSynth::Frame::Pixel(v, p, q);
	}
}

PVideoFrame Sample::GetFrame(int n, IScriptEnvironment* env) {
	Tools::AviSynth::Frame
		reference(env, child, n),
		input(env, child, n + 1);
	Tools::Motion::Estimate(reference, input, mmap, constMask);

	Tools::AviSynth::Frame result(env, vi);

	#pragma omp parallel for
	for (int y = 0; y < result.height(); ++y) {
		for (int x = 0; x < result.width(); ++x) {
			auto &vector = mmap((size_t)x, (size_t)y);
			float magnitude = sqrtf((float)(vector.dx * vector.dx + vector.dy * vector.dy));
			float angle = atan2f((float)vector.dy, (float)vector.dx) * 180.f / (float)(M_PI);
			result.write(x, y) = hsv2rgb(angle, 1, magnitude / 10);
		}
	}
	return result;
}

extern "C" __declspec(dllexport)
const char* WINAPI AvisynthPluginInit2(IScriptEnvironment* env) {
	env->AddFunction("Sample", Sample::CreateParams, Sample::Create, nullptr);
	return nullptr;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			return Tools::WinApi::InitLibrary(hModule);
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
