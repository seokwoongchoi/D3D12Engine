#pragma once

class Freedom : public Camera
{
public:
	Freedom() :Camera(),  right(1, 0, 0), up(0, 1, 0),  rotation(0,-2.5f,0),
		matRotation{},X{},Y{},Z{}
		, acceleration(1.0f)
		, drag(acceleration * 0.9f)
		, movement_speed(0, 0, 0)
	{
		Move();
		Rotation();
	}
	~Freedom() = default;
public:
	void Update() override;
	void Speed(float move, float rotation);
	
	void Rotation(Vector3* rotation)
	{
		*rotation = this->rotation* 57.29677957f;
	}
private:
	void Move() override;
	void Rotation();

private:
	Vector3 right;
	Vector3 up;
	Vector3 rotation;
	Matrix matRotation;

	Matrix X, Y, Z;



private:
	float acceleration;
	float drag;
	Vector3 movement_speed;
};