#include <DirectXComponentsPCH.h>
#include "Camera.h"

namespace Fab
{
	const float Camera::DefaultFov              = XM_PIDIV4;
	const float Camera::DefaultNearZ            = 0.01f;
	const float Camera::DefaultFarZ             = 100.0f;
	const float Camera::DefaultTranslationSpeed = 2.0f;
	const float Camera::DefaultRotationSpeed    = 0.4f;
	const CameraMode Camera::DefaultMode        = CameraMode::FLY;

	Camera::Camera()
		: _fov(DefaultFov)
		, _nearZ(DefaultNearZ)
		, _farZ(DefaultFarZ)
		, _translationSpeed(DefaultTranslationSpeed)
		, _rotationSpeed(DefaultRotationSpeed)
		, _mode(DefaultMode)
		, IComponent(ComponentType::CAMERA)
	{
		Initialise();
	}

	Camera::Camera(float fov, float nearZ, float farZ, float translationSpeed, float rotationSpeed, CameraMode mode)
		: _fov(fov)
		, _nearZ(nearZ)
		, _farZ(farZ)
		, _translationSpeed(translationSpeed)
		, _rotationSpeed(rotationSpeed)
		, _mode(mode)
		, IComponent(ComponentType::CAMERA)
	{
		Initialise();
	}

	Camera::~Camera()
	{
	}

	void Camera::Initialise()
	{
		_eye = XMFLOAT3(1.0f, 0.0f, 0.0f);
		_at = XMFLOAT3(0.0f, -0.4f, 1.0f);
		_up = XMFLOAT3(0.0f, 1.0f, 0.0f);

		_position = XMFLOAT3(-3.0f, 1.5f, -7.0f);
		_oldPosition = _position;

		_lastAngle = 0;

		ComputeProjectionMatrix();
	}

	void Camera::Draw()
	{
		D3D11RenderSystem& renderSystem = D3D11RenderSystem::GetRenderSystem();
		ConstantBuffer*    pConstantBufferUpdate = renderSystem.GetConstantBufferUpdate();

		XMMATRIX view = XMLoadFloat4x4(&_view);
		XMMATRIX projection = XMLoadFloat4x4(&_projection);

		pConstantBufferUpdate->_view = XMMatrixTranspose(view);
		pConstantBufferUpdate->_projection = XMMatrixTranspose(projection);
	}

	void Camera::Update(float deltaTime, float totalTime)
	{
		Mouse& mouse       = static_cast<Mouse&>(Application::GetApplication().GetComponent(ComponentType::MOUSE));
		Keyboard& keyboard = static_cast<Keyboard&>(Application::GetApplication().GetComponent(ComponentType::KEYBOARD));

		if (keyboard.IsKeyPressed(KeyName::SHIFT))
			return;

		switch (_mode)
		{
		case CameraMode::FLY: {
			float angleY = mouse.GetDistanceX() * _rotationSpeed * deltaTime * G_PI / 180.0f;
			float angleX = mouse.GetDistanceY() * _rotationSpeed * deltaTime * G_PI / 180.0f;
			Rotate(angleX, angleY);

			if (keyboard.IsKeyPressed(KeyName::Z) || keyboard.IsKeyPressed(KeyName::ARROW_UP))
			{
				Fly(_translationSpeed * deltaTime);
			}

			if (keyboard.IsKeyPressed(KeyName::S) || keyboard.IsKeyPressed(KeyName::ARROW_DOWN))
			{
				Fly(-_translationSpeed * deltaTime);
			}
		}break;

		case CameraMode::WALK: {
			D3D11RenderSystem& renderSystem = D3D11RenderSystem::GetRenderSystem();
			float centerPosition = (float)renderSystem.GetWindowWidth() / 2;

			float angleY = (mouse.GetDistanceX() / centerPosition) * G_PI * 2.0f;
			RotateY(-_lastAngle);
			RotateY(angleY);
			_lastAngle = angleY;

			if (keyboard.IsKeyPressed(KeyName::Z) || keyboard.IsKeyPressed(KeyName::ARROW_UP))
			{
				Walk(_translationSpeed * deltaTime);
			}

			if (keyboard.IsKeyPressed(KeyName::S) || keyboard.IsKeyPressed(KeyName::ARROW_DOWN))
			{
				Walk(-_translationSpeed * deltaTime);
			}
		}break;
		}

		if (keyboard.IsKeyPressed(KeyName::Q) || keyboard.IsKeyPressed(KeyName::ARROW_LEFT))
		{
			Move(-_translationSpeed * deltaTime, 0.0f, 0.0f);
		}

		if (keyboard.IsKeyPressed(KeyName::D) || keyboard.IsKeyPressed(KeyName::ARROW_RIGHT))
		{
			Move(_translationSpeed * deltaTime, 0.0f, 0.0f);
		}

		if (keyboard.IsKeyPressed(KeyName::SPACE))
		{
			Move(0.0f, 0.0f, _translationSpeed * deltaTime);
		}

		if (keyboard.IsKeyPressed(KeyName::CTRL))
		{
			Move(0.0f, 0.0f, -_translationSpeed * deltaTime);
		}
	}

	void Camera::ComputeProjectionMatrix()
	{
		D3D11RenderSystem& renderSystem = D3D11RenderSystem::GetRenderSystem();
		UINT windowWidth                = renderSystem.GetWindowWidth();
		UINT windowHeight               = renderSystem.GetWindowHeight();

		XMVECTOR R = XMLoadFloat3(&_eye);
		XMVECTOR U = XMLoadFloat3(&_up);
		XMVECTOR L = XMLoadFloat3(&_at);
		XMVECTOR P = XMLoadFloat3(&_position);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&_eye, R);
		XMStoreFloat3(&_up, U);
		XMStoreFloat3(&_at, L);

		_view(0, 0) = _eye.x;
		_view(1, 0) = _eye.y;
		_view(2, 0) = _eye.z;
		_view(3, 0) = x;

		_view(0, 1) = _up.x;
		_view(1, 1) = _up.y;
		_view(2, 1) = _up.z;
		_view(3, 1) = y;

		_view(0, 2) = _at.x;
		_view(1, 2) = _at.y;
		_view(2, 2) = _at.z;
		_view(3, 2) = z;

		_view(0, 3) = 0.0f;
		_view(1, 3) = 0.0f;
		_view(2, 3) = 0.0f;
		_view(3, 3) = 1.0f;

		XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(_fov, windowWidth / (FLOAT)windowHeight, _nearZ, _farZ));
	}

	void Camera::RotateX(float angle)
	{
		_oldPosition = _position;
		XMMATRIX R = XMMatrixRotationX(angle);

		XMStoreFloat3(&_eye, XMVector3TransformNormal(XMLoadFloat3(&_eye), R));
		XMStoreFloat3(&_up, XMVector3TransformNormal(XMLoadFloat3(&_up), R));
		XMStoreFloat3(&_at, XMVector3TransformNormal(XMLoadFloat3(&_at), R));

		ComputeProjectionMatrix();
	}

	void Camera::RotateY(float angle)
	{
		_oldPosition = _position;
		XMMATRIX R = XMMatrixRotationY(angle);

		XMStoreFloat3(&_eye, XMVector3TransformNormal(XMLoadFloat3(&_eye), R));
		XMStoreFloat3(&_up, XMVector3TransformNormal(XMLoadFloat3(&_up), R));
		XMStoreFloat3(&_at, XMVector3TransformNormal(XMLoadFloat3(&_at), R));

		ComputeProjectionMatrix();
	}

	void Camera::RotateZ(float angle)
	{
		_oldPosition = _position;
		XMMATRIX R = XMMatrixRotationZ(angle);

		XMStoreFloat3(&_eye, XMVector3TransformNormal(XMLoadFloat3(&_eye), R));
		XMStoreFloat3(&_up, XMVector3TransformNormal(XMLoadFloat3(&_up), R));
		XMStoreFloat3(&_at, XMVector3TransformNormal(XMLoadFloat3(&_at), R));

		ComputeProjectionMatrix();
	}

	void Camera::Pitch(float angle)
	{
		XMMATRIX T = XMMatrixRotationAxis(XMLoadFloat3(&_eye), angle);

		XMVECTOR up = XMVector3TransformCoord(XMLoadFloat3(&_up), T);
		XMVECTOR at = XMVector3TransformCoord(XMLoadFloat3(&_at), T);

		XMStoreFloat3(&_up, up);
		XMStoreFloat3(&_at, at);
	}

	void Camera::Yaw(float angle)
	{
		XMMATRIX T = XMMatrixRotationY(angle);

		XMVECTOR eye = XMVector3TransformCoord(XMLoadFloat3(&_eye), T);
		XMVECTOR at = XMVector3TransformCoord(XMLoadFloat3(&_at), T);

		XMStoreFloat3(&_eye, eye);
		XMStoreFloat3(&_at, at);

		ComputeProjectionMatrix();
	}

	void Camera::Roll(float angle)
	{
		XMMATRIX T = XMMatrixRotationAxis(XMLoadFloat3(&_at), angle);

		XMVECTOR eye = XMVector3TransformCoord(XMLoadFloat3(&_eye), T);
		XMVECTOR up = XMVector3TransformCoord(XMLoadFloat3(&_up), T);
		
		XMStoreFloat3(&_eye, eye);
		XMStoreFloat3(&_up, up);
	}

	void Camera::Rotate(float x, float y)
	{
		Pitch(x);
		Yaw(y);

		ComputeProjectionMatrix();
	}

	void Camera::Fly(float distance)
	{
		_oldPosition = _position;
		XMVECTOR s = XMVectorReplicate(distance);
		XMVECTOR r = XMLoadFloat3(&_at);
		XMVECTOR p = XMLoadFloat3(&_position);
		XMStoreFloat3(&_position, XMVectorMultiplyAdd(s, r, p));

		ComputeProjectionMatrix();
	}

	void Camera::Walk(float distance)
	{
		_oldPosition = _position;
		XMVECTOR s = XMVectorReplicate(distance);
		XMVECTOR r = XMLoadFloat3(&_at);
		XMVECTOR p = XMLoadFloat3(&_position);
		XMStoreFloat3(&_position, XMVectorMultiplyAdd(s, r, p));
		_position.y = _oldPosition.y;

		ComputeProjectionMatrix();
	}

	void Camera::Move(float distanceX, float distanceY, float distanceZ)
	{
		XMFLOAT3 movementAmount(distanceX, distanceY, distanceZ);

		XMVECTOR eye      = XMLoadFloat3(&_eye);
		XMVECTOR at       = XMLoadFloat3(&_at);
		XMVECTOR up       = XMLoadFloat3(&_up);
		XMVECTOR position = XMLoadFloat3(&_position);
		XMVECTOR movement = XMLoadFloat3(&movementAmount);

		XMVECTOR strafe = eye * XMVectorGetX(movement);
		position += strafe;

		XMVECTOR forward = at * XMVectorGetY(movement);
		position += forward;

		XMVECTOR climb = up * XMVectorGetZ(movement);
		position += climb;

		XMStoreFloat3(&_position, position);

		ComputeProjectionMatrix();
	}

	void Camera::SetFov(float fov)
	{
		_fov = fov;
		ComputeProjectionMatrix();
	}

	void Camera::SetNearZ(float nearZ)
	{
		_nearZ = nearZ;
		ComputeProjectionMatrix();
	}

	void Camera::SetFarZ(float farZ)
	{
		_farZ = farZ;
		ComputeProjectionMatrix();
	}

	void Camera::SetRotationSpeed(float rotationSpeed)
	{
		_rotationSpeed = rotationSpeed;
		ComputeProjectionMatrix();
	}

	void Camera::SetTranslationSpeed(float translationSpeed)
	{
		_translationSpeed = translationSpeed;
		ComputeProjectionMatrix();
	}

	CameraMode Camera::GetMode()
	{
		return _mode;
	}
	
	XMFLOAT3& Camera::GetPosition()
	{
		return _position;
	}

	XMFLOAT3& Camera::GetEye()
	{
		return _eye;
	}

	XMFLOAT3& Camera::GetAt()
	{
		return _at;
	}

	XMFLOAT3& Camera::GetUp()
	{
		return _up;
	}

	XMFLOAT4X4& Camera::GetView()
	{
		return _view;
	}

	XMFLOAT4X4& Camera::GetProjection()
	{
		return _projection;
	}
}