#include "Graphics/DeviceManager.h"
#include <iostream>

#include "Core/StringOperations/StringHelperFunctions.h"

//-----------------------------------------------------------------------------
//! @brief   TODO enter a description
//! @remark
//-----------------------------------------------------------------------------
bool DeviceManager::createDevice(IDXGIAdapter* adapter)
{
    MSG_TRACE_CHANNEL("DEVICEMANAGER", "Creating Device");
    //Figure out what the highest feature level is we support
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_9_1;
    HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, 0, 0, D3D11_SDK_VERSION, 0, &featureLevel, 0);
    if (FAILED(hr))
    {
        MSG_TRACE_CHANNEL("DEVICEMANAGER", "Failed to create a D3D device with error code: 0x%x", hr )
        return false;
    }
    
    MSG_TRACE_CHANNEL("DEVICEMANAGER", "Selected Feature level: 0x%x", featureLevel )

    DWORD deviceCreationFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
    deviceCreationFlags |= D3D11_CREATE_DEVICE_DEBUG ;//| D3D11_CREATE_DEVICE_DEBUGGABLE;
#endif
    hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, deviceCreationFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_device, &m_featureLevel, &m_deviceContext);
    if (FAILED(hr))
    {
        MSG_TRACE_CHANNEL("DEVICEMANAGER", "Failed to create a D3D device with error code: 0x%x", hr )
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
//! @brief  Cleanup the device, D3D object and swap chains
//! @remark
//-----------------------------------------------------------------------------
void DeviceManager::cleanup()
{
    if (m_device)
    {
        m_device->Release();
    }
    if (m_deviceContext)
    {
        m_deviceContext->Release();
    }
}