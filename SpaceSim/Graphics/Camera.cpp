//#include "DXUT.h"

#include "Graphics/Camera.h"
#include "Input/InputSystem.h"
#include "Math/matrix33.h"
#include "Core/StringOperations/HashString.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <directxmath.h>

const float Camera::m_maxPitchAngle = 89.9f;
const HashString moveForward("move_forwards");
const HashString moveBackWards("move_backwards");
const HashString moveLeft("move_left");
const HashString moveRight("move_right");
const HashString yawLeft("move_yaw_left");
const HashString yawRight("move_yaw_right");
const HashString pitchUp("move_pitch_up");
const HashString pitchDown("move_pitch_down");
const HashString rollLeft("move_roll_left");
const HashString rollRight("move_roll_right");

Camera::Camera() :
m_roll(0.0f),
m_yaw(0.0f),
m_pitch(0.0f),
m_movementSpeed(50.0f),
m_rotationSpeed(25.0f)
{
}

Camera::Camera(float movementSpeed, float rotationSpeed) :
m_roll(0.0f),
m_yaw(0.0f),
m_pitch(0.0f),
m_movementSpeed(movementSpeed),
m_rotationSpeed(rotationSpeed)
{
}

Camera::~Camera()
{
}

void Camera::positionCamera(const Vector3& eye, const Vector3& lookat, const Vector3& up)
{
    m_eye = eye;
    m_cameraForward = lookat - eye;
    cross(m_cameraRight, up, m_cameraForward);
    m_cameraForward.normalize();
    m_cameraRight.normalize();
    cross(m_cameraUp, m_cameraForward, m_cameraRight);
    createCamera();
}

void Camera::roll(float angle)
{
    Matrix33 rollRotation;
    rotate(rollRotation, m_cameraForward, angle);
    m_roll += angle; //Keep track of the roll angle
    m_cameraRight = m_cameraRight * rollRotation;
    m_cameraUp    = m_cameraUp * rollRotation;
}

void Camera::pitch(float angle)
{
    Matrix33 pitchRotation;
    rotate(pitchRotation, m_cameraRight, angle);
    m_pitch += angle; //Keep track of the pitch angle
    m_cameraForward = m_cameraForward * pitchRotation;
    m_cameraUp      = m_cameraUp * pitchRotation;
}

void Camera::yaw(float angle)
{
    Matrix33 yawRotation;
    rotate(yawRotation, m_cameraUp, angle);
    m_yaw += angle; //Keep track of the yaw angle
    m_cameraForward = m_cameraForward * yawRotation;
    m_cameraRight   = m_cameraRight * yawRotation;
}

void Camera::moveOverPlane(float deltau, float deltav, float deltan)
{
    m_cameraPositionDelta = Vector3(
        deltau * m_cameraRight.x() + deltav * m_cameraUp.x() + deltan * m_cameraForward.x(),
        deltau * m_cameraRight.y() + deltav * m_cameraUp.y() + deltan * m_cameraForward.y(),
        deltau * m_cameraRight.z() + deltav * m_cameraUp.z() + deltan * m_cameraForward.z());

    m_eye = m_eye + m_cameraPositionDelta;
}

Matrix44 Camera::createCamera()
{
    m_cameraForward.normalize();
    m_cameraUp.normalize();
    cross(m_cameraRight, m_cameraUp, m_cameraForward);
    m_cameraRight.normalize();
    cross(m_cameraUp, m_cameraForward, m_cameraRight);
    m_cameraUp.normalize();
  
    Vector3 evec(m_eye);
    m_camera = Matrix44(m_cameraRight.x(),    m_cameraRight.y(),    m_cameraRight.z(),    -evec.dot(m_cameraRight),
                        m_cameraUp.x(),       m_cameraUp.y(),       m_cameraUp.z(),       -evec.dot(m_cameraUp),
                        m_cameraForward.x(),  m_cameraForward.y(),  m_cameraForward.z(),  -evec.dot(m_cameraForward),
                        0.0f,	              0.0f,	                0.0f,		          1.0f);

    m_camera.transpose(); //We want row major matrices
    m_invcamera = m_camera.inverted();
    return m_camera;
}

float Camera::restrictAngleTo360(float angle) const
{
    while(angle > 2 * math::gmPI)
        angle -= 2 * math::gmPI;

    while(angle < 0)
        angle += 2 * math::gmPI;

    return angle;
}
//-----------------------------------------------------------------------------
//! @brief   TODO enter a description
//! @remark
//-----------------------------------------------------------------------------
void Camera::update( float elapsedTime, double time, const Input& input )
{
    const InputState* inputState = input.getInput(0);
    if (inputState)
    {
		InputActions::ActionType fowardAction;
		InputActions::ActionType backwardAction;
		InputActions::ActionType moveLeftAction;
		InputActions::ActionType moveRightAction;
		InputActions::ActionType yawLeftAction;
		InputActions::ActionType yawRightAction;
		InputActions::ActionType pitchUpAction;
		InputActions::ActionType pitchDownAction;
		InputActions::ActionType rollLeftAction;
		InputActions::ActionType rollRightAction;
		InputSystem::getInputActionFromName(moveForward.getHash(), fowardAction);
		InputSystem::getInputActionFromName(moveBackWards.getHash(), backwardAction);
		InputSystem::getInputActionFromName(moveLeft.getHash(), moveLeftAction);
		InputSystem::getInputActionFromName(moveRight.getHash(), moveRightAction);
		InputSystem::getInputActionFromName(yawLeft.getHash(), yawLeftAction);
		InputSystem::getInputActionFromName(yawRight.getHash(), yawRightAction);
		InputSystem::getInputActionFromName(pitchUp.getHash(), pitchUpAction);
		InputSystem::getInputActionFromName(pitchDown.getHash(), pitchDownAction);
		InputSystem::getInputActionFromName(rollLeft.getHash(), rollLeftAction);
		InputSystem::getInputActionFromName(rollRight.getHash(), rollRightAction);

        float moveAlongDirectionFactor = inputState->getActionValue(fowardAction) - inputState->getActionValue(backwardAction);
        moveAlongDirection(moveAlongDirectionFactor * m_movementSpeed * elapsedTime );//Move forwared, backward
        float strafeFactor = inputState->getActionValue(moveLeftAction) - inputState->getActionValue(moveRightAction);
        strafe(strafeFactor   * m_movementSpeed * elapsedTime);//Move left/right
        float yawFactor = inputState->getActionValue(yawLeftAction) - inputState->getActionValue(yawRightAction);
        yaw(yawFactor * m_rotationSpeed * elapsedTime);
        float pitchFactor = inputState->getActionValue(pitchUpAction) - inputState->getActionValue(pitchDownAction);
        pitch(pitchFactor * m_rotationSpeed * elapsedTime); 
        float zAxisDelta = inputState->getActionValue(rollLeftAction) - inputState->getActionValue(rollRightAction);
        roll(zAxisDelta * m_rotationSpeed * 0.05f);
    }

    //Use the varibles to avoid a warning and allows us to use them in the code above without change
    time = 0.0;
    elapsedTime = 0.0f;
}