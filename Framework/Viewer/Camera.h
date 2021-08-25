#pragma once

class Camera
{
public:
	Camera() :move(20), fov(Math::ToRadian(45)), R(2), position(0, 0, 0), forward(0, 0, 1), data{}, delta(0.04f)
	{
		
		
	}
	virtual ~Camera()=default;

	virtual void Update() = 0;
	virtual void Move()=0;

	void Position(Vector3* pos)
	{
		*pos = position;
	}
	
protected:
	float delta;
	float fov;
	float move;
	float R;
	Vector3 forward;
	Vector3 position;

	Matrix matView;
	Matrix proj;

	GlobalViewData data;
};