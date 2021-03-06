#include "Graphics/CubeMapRenderer.h"
#include "Graphics/DeviceManager.h"
#include "Graphics/Effect.h"
#include "Core/Settings/Settings.h"
#include "Core/Settings/SettingsManager.h"
#include <assert.h>

#include "Application/BaseApplication.h"
#include "Graphics/RenderInstance.h"
#include "Core/StringOperations/StringHelperFunctions.h"

#include "Graphics/OrientationAxis.h"
#include "Core/Types/Types.h"
#include <d3d11.h>
#include <dxgi.h>
#include <list>
#include <directxmath.h>
#include "Graphics/texturemanager.h"
#include "Graphics/modelmanager.h"

#ifdef _DEBUG
#include <d3d11_1.h>
#include <atlbase.h>

#include "ScreenGrab.h"

#include "brofiler.h"
#endif


//-------------------------------------------------------------------------
// @brief 
//-------------------------------------------------------------------------
CubeMapRenderer::CubeMapRenderer(DeviceManager& deviceManager, ID3D11BlendState* alphaBlendState, ID3D11BlendState* blendState, unsigned int cubeMapWidhtHeight /*= 1024*/) :
    m_alphaBlendState(alphaBlendState),
    m_blendState(blendState)
{
    m_cubeMapWidthHeight = cubeMapWidhtHeight;

    D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
    depthStencilTextureDesc.Width = m_cubeMapWidthHeight;
    depthStencilTextureDesc.Height = m_cubeMapWidthHeight;
    depthStencilTextureDesc.MipLevels = 1;
    depthStencilTextureDesc.ArraySize = 6;
    depthStencilTextureDesc.SampleDesc.Count = 1;
    depthStencilTextureDesc.SampleDesc.Quality = 0;
    depthStencilTextureDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilTextureDesc.CPUAccessFlags = 0;
    depthStencilTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    HRESULT hr = deviceManager.getDevice()->CreateTexture2D(&depthStencilTextureDesc, NULL, &m_depthStencil);
    if (FAILED(hr))
    {
        MSG_TRACE_CHANNEL("CubemapRenderer_ERROR", "Failed to create depth stencil for the cubemap renderer: 0x%x", hr);
    }

    // Create the depth stencil view for the entire cube
    D3D11_DEPTH_STENCIL_VIEW_DESC cubeDepthStencilDescriptor;
    cubeDepthStencilDescriptor.Format = DXGI_FORMAT_D32_FLOAT;
    cubeDepthStencilDescriptor.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    cubeDepthStencilDescriptor.Texture2DArray.FirstArraySlice = 0;
    cubeDepthStencilDescriptor.Texture2DArray.ArraySize = 1;
    cubeDepthStencilDescriptor.Texture2DArray.MipSlice = 0;
    cubeDepthStencilDescriptor.Flags = 0;
    hr = deviceManager.getDevice()->CreateDepthStencilView(m_depthStencil, &cubeDepthStencilDescriptor, &m_depthStencilView);
    if (FAILED(hr))
    {
        MSG_TRACE_CHANNEL("CubemapRenderer_ERROR", "Failed to create depth stencil view for the cubemap renderer: 0x%x", hr);
    }

    ZeroMemory(&m_cubeViewPort, sizeof(D3D11_VIEWPORT));
    m_cubeViewPort.Height = (float)m_cubeMapWidthHeight;
    m_cubeViewPort.Width = (float)m_cubeMapWidthHeight;
    m_cubeViewPort.MinDepth = 0;
    m_cubeViewPort.MaxDepth = 1;
    m_cubeViewPort.TopLeftX = 0;
    m_cubeViewPort.TopLeftY = 0;

    m_cubeProjection = math::createLeftHandedFOVPerspectiveMatrix(math::gmPI / 2.0f, 1.0f, 1.0f, 500.0f);

    D3D11_BUFFER_DESC lightContantsDescriptor;
    ZeroMemory(&lightContantsDescriptor, sizeof(D3D11_BUFFER_DESC));
    lightContantsDescriptor.ByteWidth = sizeof(LightConstants) * 8 + 4 * sizeof(float);
    lightContantsDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = deviceManager.getDevice()->CreateBuffer(&lightContantsDescriptor, 0, &m_perFrameConstants);
}

CubeMapRenderer::~CubeMapRenderer()
{
}

void CubeMapRenderer::initialise(Vector3 position)
{
    m_position = position;
    createViewArray(position);
}



void CubeMapRenderer::renderCubeMap(Resource* resource, Texture* renderTarget, const RenderInstanceTree& renderInstances, const DeviceManager& deviceManager, PerFrameConstants& perFrameConstants, const TextureManager& textureManager)
{
    BROFILER_CATEGORY("CubeMapRenderer::renderCubeMap", Profiler::Color::LightBlue);

    ID3D11DeviceContext* deviceContext = deviceManager.getDeviceContext();
    ID3D11RenderTargetView* rtView[1] = { nullptr };
    ID3D11DepthStencilView* dsView = nullptr;
    deviceContext->OMGetRenderTargets(1, rtView, &dsView);

    D3D11_VIEWPORT OldVP;
    UINT cRT = 1;
    deviceContext->RSGetViewports(&cRT, &OldVP);
    deviceContext->RSSetViewports(1, &m_cubeViewPort);

    perFrameConstants.m_cameraPosition[0] = m_position.x();
    perFrameConstants.m_cameraPosition[1] = m_position.y();
    perFrameConstants.m_cameraPosition[2] = m_position.z();

    deviceContext->UpdateSubresource(m_perFrameConstants, 0, 0, (void*)&perFrameConstants, 0, 0);

    deviceContext->PSSetConstantBuffers(1, 1, &m_perFrameConstants);

    const ShaderCache& shaderCache = GameResourceHelper(resource).getGameResource().getShaderCache();

    for (size_t rtCounter = 0; rtCounter < 6; ++rtCounter)
    {
        BROFILER_CATEGORY("CubeMapRenderer::renderFace", Profiler::Color::LightBlue);
        ID3D11RenderTargetView* aRTViews[1] = { renderTarget->getRenderTargetView(rtCounter) };
        deviceContext->OMSetRenderTargets(sizeof(aRTViews) / sizeof(aRTViews[0]), aRTViews, m_depthStencilView);
        deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);

        RenderInstanceTree::const_iterator renderInstanceIt = renderInstances.begin();
        RenderInstanceTree::const_iterator renderInstanceEnd = renderInstances.end();
        unsigned int stride = 0;
        unsigned int offset = 0;
        size_t oldTechniqueId = 0;
        std::vector<ID3D11ShaderResourceView*> resourceViews;
        std::vector<ID3D11SamplerState*> samplerStates;
        resourceViews.reserve(128);
        samplerStates.reserve(128);
        for (; renderInstanceIt != renderInstanceEnd; ++renderInstanceIt)
        {
            resourceViews.clear();
            samplerStates.clear();
            RenderInstance& renderInstance = (RenderInstance&)(*(*renderInstanceIt));

            const Material& material = renderInstance.getShaderInstance().getMaterial();
            const Effect* effect = material.getEffect();
            const Technique* technique = effect->getTechnique(material.getTechnique());
            technique->setMaterialContent(deviceManager, material.getMaterialCB());
            WVPBufferContent wvpConstants = renderInstance.getShaderInstance().getWVPConstants();
            wvpConstants.m_projection = m_cubeProjection;
            wvpConstants.m_view = m_viewArray[rtCounter];
            technique->setWVPContent(deviceManager, wvpConstants);

	        const std::vector<Material::TextureSlotMapping>& textureHashes = material.getTextureHashes();
	        //const std::vector<ID3D11SamplerState*>& samplerStates = renderInstance.getMaterial().getTextureSamplers();

            size_t currentTextureIndex = 0;
            for (unsigned int counter = 0; counter < Material::TextureSlotMapping::NumSlots && currentTextureIndex < textureHashes.size(); ++counter) //We assume these are put in as order for the slots demand
            {
                const Texture* texture = nullptr;
                if (static_cast<Material::TextureSlotMapping::TextureSlot>(counter) == textureHashes[currentTextureIndex].m_textureSlot)
                {
                    texture = textureManager.getTexture(textureHashes[currentTextureIndex].m_textureHash);
                    ++currentTextureIndex;
                }
                ID3D11ShaderResourceView* srv = texture != nullptr ? texture->getShaderResourceView() : nullptr;
                ID3D11SamplerState* const samplerState = textureManager.getSamplerState();
                resourceViews.push_back(srv);
                samplerStates.push_back(samplerState);
            }

            if (!resourceViews.empty())
            {
                deviceContext->PSSetShaderResources(0, static_cast<uint32>(resourceViews.size()), &resourceViews[0]);
                deviceContext->PSSetSamplers(0, 1, &samplerStates[0]); //THis shoudl not be this way
            }

            if (technique->getTechniqueId() != oldTechniqueId)
            {
                //this will crash, also we shouldnt set this if the shader id hasnt changed from the previous set
                deviceContext->VSSetShader(shaderCache.getVertexShader(technique->getVertexShader()) ? shaderCache.getVertexShader(technique->getVertexShader())->getShader() : nullptr, nullptr, 0);
                deviceContext->HSSetShader(shaderCache.getHullShader(technique->getHullShader()) ? shaderCache.getHullShader(technique->getHullShader())->getShader() : nullptr, nullptr, 0);
                deviceContext->DSSetShader(shaderCache.getDomainShader(technique->getDomainShader()) ? shaderCache.getDomainShader(technique->getDomainShader())->getShader() : nullptr, nullptr, 0);
                deviceContext->GSSetShader(shaderCache.getGeometryShader(technique->getGeometryShader()) ? shaderCache.getGeometryShader(technique->getGeometryShader())->getShader() : nullptr, nullptr, 0);
                deviceContext->PSSetShader(shaderCache.getPixelShader(technique->getPixelShader()) ? shaderCache.getPixelShader(technique->getPixelShader())->getShader() : nullptr, nullptr, 0);
                oldTechniqueId = technique->getTechniqueId();
            }

            technique->setupTechnique();

	        if (material.getBlendState())
	        {
	            deviceContext->OMSetBlendState(m_alphaBlendState, 0, 0xffffffff);
	        }
	        else
	        {
	            deviceContext->OMSetBlendState(m_blendState, 0, 0xffffffff);
	        }

            // Set vertex buffer stride and offset.
            const VertexBuffer* vb = renderInstance.getGeometryInstance().getVB();
            ID3D11Buffer* buffer = vb->getBuffer();
            stride = static_cast<unsigned int>(vb->getVertexStride());
            if (renderInstance.getGeometryInstance().getIB() != nullptr)
            {
                deviceContext->IASetInputLayout(renderInstance.getGeometryInstance().getVB()->getInputLayout());
                deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
                deviceContext->IASetIndexBuffer(renderInstance.getGeometryInstance().getIB()->getBuffer(), DXGI_FORMAT_R32_UINT, 0);
                deviceContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)renderInstance.getPrimitiveType());
                deviceContext->DrawIndexed(renderInstance.getGeometryInstance().getIB()->getNumberOfIndecis(), 0, 0);
            }
            else
            {
                deviceContext->IASetInputLayout(renderInstance.getGeometryInstance().getVB()->getInputLayout());
                deviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
                deviceContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)renderInstance.getPrimitiveType());
                deviceContext->Draw((unsigned int)renderInstance.getGeometryInstance().getVB()->getVertexCount(), 0);
            }
        }
        deviceContext->Flush();
    }

    deviceContext->OMSetRenderTargets(1, rtView, dsView);
    rtView[0]->Release();
    dsView->Release();

    deviceContext->RSSetViewports(cRT, &OldVP);
    deviceContext->GenerateMips(renderTarget->getShaderResourceView());
}

//-------------------------------------------------------------------------
// @brief 
//-------------------------------------------------------------------------
void CubeMapRenderer::createViewArray(Vector3 position)
{
    Camera viewArray[6];
    viewArray[0].positionCamera(position, position + Vector3(1.0f, 0.0f, 0.0f), Vector3::yAxis());
    viewArray[1].positionCamera(position, position + Vector3(-1.0f, 0.0f, 0.0f), Vector3::yAxis());
    viewArray[2].positionCamera(position, position + Vector3(0.0f, 0.0f, 1.0f), Vector3::yAxis());
    viewArray[3].positionCamera(position, position + Vector3(0.0f, 0.0f, -1.0f), Vector3::yAxis());
    viewArray[4].positionCamera(position, position + Vector3(0.0f, 0.0f, 1.0f), Vector3::yAxis());
    viewArray[5].positionCamera(position, position + Vector3(0.0f, 0.0f, -1.0f), Vector3::yAxis());

    Matrix44 rotateX90;
    rotateX(rotateX90, math::toRadian(90.0f));

    m_viewArray[0] = viewArray[0].createCamera();
    m_viewArray[1] = viewArray[1].createCamera();
    m_viewArray[2] = viewArray[2].createCamera() * rotateX90;
    rotateX(rotateX90, math::toRadian(-90.0f));
    m_viewArray[3] = viewArray[3].createCamera() * rotateX90;
    m_viewArray[4] = viewArray[4].createCamera();
    m_viewArray[5] = viewArray[5].createCamera();
}
