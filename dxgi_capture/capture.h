#pragma once

#include <vector>
#include <assert.h>
#include <process.h>

#include "D3D11.h"
#include "DXGI1_2.h"
#include <iostream>
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")

#define RESET_OBJECT(obj) { if(obj) obj->Release(); obj = NULL; }

struct color {
	unsigned char b, g, r, a;
public:

	color operator=(const color& that) {
		r = that.r;
		g = that.g;
		b = that.b;
		a = that.a;
		return *this;
	}

	color(uint32_t col) {
		r = col >> 24;
		g = (col >> 16) & 0xFF;
		b = (col >> 8) & 0xFF;
		a = col & 0xFF;
	}

	color() {
		r = 0;
		g = 0;
		b = 0;
		a = 0;
	}

	color(int r, int g, int b) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = 255;
	}
};

class capture
{
public:
	capture();
	~capture();

	bool update();

	bool initialize();
	void release();

	int width() { return _width; }
	int height() { return _height; }

	color * pixel(int x, int y);

	const uint8_t* bitmap() { return _data.data(); }

private:
	bool update(uint8_t* buffer, int32_t& size, int time);

	bool attach_thread()
	{
		HDESK thread_desktop = GetThreadDesktop(GetCurrentThreadId());
		HDESK input_desktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
		if (!input_desktop)
		{
			return FALSE;
		}

		bool attached = SetThreadDesktop(input_desktop) ? true : false;

		int err = GetLastError();

		CloseDesktop(thread_desktop);
		CloseDesktop(input_desktop);

		return attached;
	}

	capture(const capture&) = delete;
	capture& operator=(const capture&) = delete;

	ID3D11Device* _device = nullptr;
	ID3D11DeviceContext* _device_ctx = nullptr;
	IDXGIOutputDuplication* _desktop_dup = nullptr;

	std::vector<uint8_t> _data;
	int _width = 0;
	int _height = 0;
	int _image_size = 0;
};
