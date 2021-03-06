#include "Gameplay/door.h"
#include "Math/matrix44.h"
#include "Core/Settings/SettingsManager.h"
#include "Graphics/texturemanager.h"
#include "Graphics/DeviceManager.h"

#include <iostream>
#include <D3D11.h>


HASH_ELEMENT_IMPLEMENTATION(Door) 

Door::Door(Resource* resource, const Vector3& position):
GameObject(resource)
{
	//position is the middle of a corridor
	m_position = position;
	m_move = 0.0f;
	m_plus = true;
	m_active = true;
}

//-----------------------------------------------------------------------------
//! @brief   TODO enter a description
//! @remark
//-----------------------------------------------------------------------------
void Door::initialise(const ShaderInstance& shaderInstance, bool changeWindingOrder)
{
    Face::CreationParams params;
    params.shaderInstance = &shaderInstance;
    params.resource = m_resource;
    params.fillvalue = 0.0f;
    params.fillx = true;
    params.changeWindingOrder = changeWindingOrder;
    //CreatedModel face = Face::CreateFace(params);
    //m_drawableObject = face.model;
    //m_active = true;

    //Super::initialise(shaderInstance);
}

//-------------------------------------------------------------------------
// @brief 
//-------------------------------------------------------------------------
const ShaderInstance Door::deserialise( const tinyxml2::XMLElement* element)
{
    ShaderInstance shaderInstance;

    const tinyxml2::XMLAttribute* attribute = element->FindAttribute("name");
    if (attribute != nullptr)
    {
        m_name = attribute->Value();
    }

    for (element = element->FirstChildElement(); element != 0; element = element->NextSiblingElement())
    {
        unsigned int typeHash = hashString(element->Value());
        if (Material::m_hash == typeHash)
        {
            shaderInstance.getMaterial().deserialise(m_resource, getGameResource().getDeviceManager(), getGameResource().getTextureManager(), getGameResource().getLightManager(), element);
            //shaderInstance.getMaterial().setBlendState(true);
        }
        else if (Vector3::m_hash == typeHash)
        {
            m_position.deserialise(element);
            translate(m_world, m_position.x(), m_position.y(), m_position.z());
        }
    }

    return shaderInstance;
}

//-------------------------------------------------------------------------
// @brief
//-------------------------------------------------------------------------
void Door::update( RenderInstanceTree& renderInstances, float elapsedTime, const Input& input )
{
    //move
    if (!m_active)
    {
        if (m_move < 50.0f)
            m_move += 10.0f*elapsedTime;

        Matrix44 transform;
        translate(transform, 0.0f, 0.0f, m_move);
        m_world = m_world * transform;
    }
    else
    {
        //if (m_move > 0.0f)
        //    m_move -= 10.0f*elapsedTime;
        //translate(m_world, 0.0f, 0.0f, m_move);
    }
    //Super::update(renderInstances, elapsedTime, input);
#ifdef _DEBUG
    //renderInstances.back()->m_name = L"Door";
#endif

    UNUSEDPARAM(renderInstances);
    UNUSEDPARAM(input);
}

//-------------------------------------------------------------------------
// @brief 
//-------------------------------------------------------------------------
void Door::handleMessage( const Message& msg )
{
    const ActivationMessage& activateMsg = (const ActivationMessage&)msg;
    setActive(activateMsg.shouldActivate());
}
