#include "Framework.h"
#include "Freedom.h"


void Freedom::Update()
{
	
	
		
	if (Keyboard::Get()->Press('W'))
		movement_speed += forward * acceleration*move *delta;
	else if (Keyboard::Get()->Press('S'))
		movement_speed -= forward * acceleration*move *delta;

	if (Keyboard::Get()->Press('D'))
		movement_speed += right * acceleration*move *delta;
	else if (Keyboard::Get()->Press('A'))
		movement_speed -= right * acceleration*move *delta;
	if (Keyboard::Get()->Press('E'))
		movement_speed += up * acceleration*move *delta;
	else if (Keyboard::Get()->Press('Q'))
		movement_speed -= up * acceleration*move *delta;

	if (Mouse::Get()->Press(1) == true)
	{
		auto& moveValue = Mouse::Get()->GetMoveValue();

		rotation.x += moveValue.y*R* delta;
		rotation.y += moveValue.x*R* delta;
	

		Rotation();
	}
	
	position += movement_speed;
	
	movement_speed *= drag * (1.0f - Time::Delta());

	Move();
	
}

void Freedom::Move()
{
	Matrix::LookAtLH(&matView, position, (position + forward), up);
	

	float aspect = static_cast<float>(D3D::Width()) / static_cast<float>(D3D::Height());

	// Matrix::PerspectiveFovLH(&proj,fov, aspect, 0.1f, 1000.0f);
	
	Matrix::PerspectiveFovLH(&proj, static_cast<float>(Math::PI)* 0.35f, aspect, 0.1f, 1000.0f);
	memcpy(&data.view, &matView, sizeof(Matrix));
     memcpy(&data.proj, &proj, sizeof(Matrix));
	data.pos = position;
	data.dir = forward;
	data.lookAt = position + forward;
	GlobalData::SetWorldViewData(&data);
}

void Freedom::Rotation()
{
	
	Matrix::RotationX(&X, rotation.x);
	Matrix::RotationY(&Y, rotation.y);
	Matrix::RotationZ(&Z, rotation.z);


	matRotation = X * Y*Z;
	forward = Vector3(0, 0, 1)*matRotation;
	right = Vector3(1, 0, 0)*matRotation;
	up = Vector3(0, 1,0)*matRotation;
	
}

void Freedom::Speed(float move, float rotation)
{
	this->move = move;
	this->R = rotation;
}
