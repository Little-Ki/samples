#include "capture.h"

capture::capture() {
}

capture::~capture() {
    release();
}

bool capture::initialize() {
    HRESULT hr = S_OK;

    // Driver types supported
    D3D_DRIVER_TYPE DriverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // Feature levels supported
    D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_1};
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    //
    // Create D3D device
    //
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
        hr = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, 0, FeatureLevels, NumFeatureLevels, D3D11_SDK_VERSION, &_device, &FeatureLevel, &_device_ctx);
        if (SUCCEEDED(hr)) {
            break;
        }
    }
    if (FAILED(hr)) {
        return false;
    }

    //
    // Get DXGI device
    //
    IDXGIDevice *hDxgiDevice = NULL;
    hr = _device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&hDxgiDevice));
    if (FAILED(hr)) {
        return false;
    }

    //
    // Get DXGI adapter
    //
    IDXGIAdapter *hDxgiAdapter = NULL;
    hr = hDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(&hDxgiAdapter));
    RESET_OBJECT(hDxgiDevice);
    if (FAILED(hr)) {
        return false;
    }

    //
    // Get output
    //
    int32_t nOutput = 0;
    IDXGIOutput *hDxgiOutput = NULL;
    hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
    RESET_OBJECT(hDxgiAdapter);
    if (FAILED(hr)) {
        return false;
    }

    //
    // get output description struct
    //
    DXGI_OUTPUT_DESC m_dxgiOutDesc;
    hDxgiOutput->GetDesc(&m_dxgiOutDesc);
    _width = m_dxgiOutDesc.DesktopCoordinates.right - m_dxgiOutDesc.DesktopCoordinates.left;
    _height = m_dxgiOutDesc.DesktopCoordinates.bottom - m_dxgiOutDesc.DesktopCoordinates.top;
    _image_size = _width * _height * 4;
    //
    // QI for Output 1
    //
    IDXGIOutput1 *hDxgiOutput1 = NULL;
    hr = hDxgiOutput->QueryInterface(__uuidof(hDxgiOutput1), reinterpret_cast<void **>(&hDxgiOutput1));
    RESET_OBJECT(hDxgiOutput);
    if (FAILED(hr)) {
        return false;
    }

    //
    // Create desktop duplication
    //
    hr = hDxgiOutput1->DuplicateOutput(_device, &_desktop_dup);
    RESET_OBJECT(hDxgiOutput1);
    if (FAILED(hr)) {
        return false;
    }

    _data.resize(_image_size);

    return true;
}

void capture::release() {
    RESET_OBJECT(_desktop_dup);
    RESET_OBJECT(_device_ctx);
    RESET_OBJECT(_device);
}

color *capture::pixel(int x, int y) {
    static color black;
    if (x >= 0 && x < _width && y >= 0 && y < _height) {
        return (color *)(&_data[y * _width * 4 + x * 4]);
    }
    return &black;
}

bool capture::update(uint8_t *buffer, int32_t &size, int time) {
    if (!attach_thread()) {
        return false;
    }

    size = 0;

    IDXGIResource *dxgi_resource = NULL;
    DXGI_OUTDUPL_FRAME_INFO frame_info;
    _desktop_dup->ReleaseFrame();
    HRESULT hr = _desktop_dup->AcquireNextFrame(1000, &frame_info, &dxgi_resource);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return true;
        }
        release();
        initialize();
        return false;
    }

    //
    // query next frame staging buffer
    //
    ID3D11Texture2D *desktop_image = NULL;
    hr = dxgi_resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&desktop_image));
    RESET_OBJECT(dxgi_resource);
    if (FAILED(hr)) {
        return false;
    }

    //
    // copy old description
    //
    D3D11_TEXTURE2D_DESC frame_desc;
    desktop_image->GetDesc(&frame_desc);

    //
    // create a new staging buffer for fill frame image
    //
    ID3D11Texture2D *new_desktop_image = NULL;
    frame_desc.Usage = D3D11_USAGE_STAGING;
    frame_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    frame_desc.BindFlags = 0;
    frame_desc.MiscFlags = 0;
    frame_desc.MipLevels = 1;
    frame_desc.ArraySize = 1;
    frame_desc.SampleDesc.Count = 1;
    hr = _device->CreateTexture2D(&frame_desc, NULL, &new_desktop_image);
    if (FAILED(hr)) {
        RESET_OBJECT(desktop_image);
        _desktop_dup->ReleaseFrame();
        return false;
    }

    //
    // copy next staging buffer to new staging buffer
    //
    _device_ctx->CopyResource(new_desktop_image, desktop_image);

    RESET_OBJECT(desktop_image);
    _desktop_dup->ReleaseFrame();

    //
    // create staging buffer for map bits
    //
    IDXGISurface *staging_surface = NULL;
    hr = new_desktop_image->QueryInterface(__uuidof(IDXGISurface), (void **)(&staging_surface));
    RESET_OBJECT(new_desktop_image);
    if (FAILED(hr)) {
        return false;
    }
    DXGI_SURFACE_DESC staging_surface_desc;

    // BGRA8
    staging_surface->GetDesc(&staging_surface_desc);

    //
    // copy bits to user space
    //
    DXGI_MAPPED_RECT mapped_rect;
    hr = staging_surface->Map(&mapped_rect, DXGI_MAP_READ);
    int image_size = _width * _height * 4;
    if (SUCCEEDED(hr)) {
        size = image_size;
        std::copy(mapped_rect.pBits, mapped_rect.pBits + image_size, buffer);
        staging_surface->Unmap();
    } else
        return false;

    RESET_OBJECT(staging_surface);

    return SUCCEEDED(hr);
}

bool capture::update() {
    return update(_data.data(), _image_size, 0);
}