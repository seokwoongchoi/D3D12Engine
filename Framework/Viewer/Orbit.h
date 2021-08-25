#pragma once
class Orbit : public Camera
{
public:
	Orbit():Camera(), targetPosition(0,0,0), deltaPos(0,0,2), R(0.375414f, -0.562102f), moveValue(0,0)
	{
		Move();
	}
	~Orbit() = default;
public:
	inline void SetTargetPosition(const Vector3& targetPosition)
	{
		this->targetPosition = targetPosition;
	}

	inline void SetDeltaPosition(const Vector3& deltaPos)
	{
		this->deltaPos = deltaPos;
	}

	inline void SetMoveValue(const Vector2& value)
	{
		moveValue = value;
	}
	void PreviewUpdate();

public:
	void Update() override;
private:
	void Move() override;

private:

	
	Vector3 targetPosition;
	Vector3 deltaPos;
	Vector2 moveValue;
	Vector2 R;
};

