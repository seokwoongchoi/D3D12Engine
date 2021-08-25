#include "Framework.h"
#include "Orbit.h"


void Orbit::PreviewUpdate()
{
}

void Orbit::Update()
{
	//if (Mouse::Get()->Press(1) == false)
	//	return;

	//Vector3 val = Mouse::Get()->GetMoveValue();
	
	
	R.y += (moveValue.y *0.15f)* 0.02f;
	Math::Clamp(R.y ,-2.66f, -0.154f);
	
	R.x += (moveValue.x *0.15f)* 0.02f;

	
	

	Move();
}

void Orbit::Move()
{
	const float& dist = D3DXVec3Length(&deltaPos);

	position.x = targetPosition.x + dist * sinf(R.y)*cosf(-R.x - 89.2f);
	position.y = targetPosition.y + dist * cosf(R.y);
	position.z = targetPosition.z + dist * sinf(R.y)*sinf(-R.x - 89.2f);

	D3DXVec3Normalize(&forward, &(targetPosition - position));


	D3DXMatrixLookAtLH(&matView, &position, &(targetPosition), &Vector3(0, 1, 0));

	float aspect = static_cast<float>(D3D::Width()) / static_cast<float>(D3D::Height());
	D3DXMatrixPerspectiveFovLH(&proj, static_cast<float>(D3DX_PI)* 0.25f, aspect, 0.1f, 1000.0f);
	data.view = matView;
	data.proj = proj;
	data.pos = position;
	data.dir = forward;
	data.lookAt = targetPosition;
	GlobalData::SetWorldViewData(&data);
}



