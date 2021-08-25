#include "Framework.h"
#include "QuadTree.h"
//#include "../Editor/Debug/DebugLine.h"

QuadTree::QuadTree()
	:pos(0,0,0), height(512.0f),width(512.0f)
{
	
	root = make_shared< QuadTreeNode>();

}


QuadTree::~QuadTree()
{
}



void QuadTree::UpdateHeight(shared_ptr< QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom, float** heightMap)
{
	

	static const float TileSize = QuadTreeSize;
	static const float  tolerance = 0.01f;

	


	
		float minY = FLT_MAX;
		float maxY = -FLT_MAX;


		uint lt = ((uint)leftTop.y - 512)*-1;
		uint rb = ((uint)rightBottom.y - 512)*-1;


		for (uint x = (uint)leftTop.x; x < (uint)rightBottom.x; x++)
			for (uint y = rb; y < lt; y++)
			{


				float data = 0.0f;

				data = heightMap[x][y];

				minY = min(minY, data);
				maxY = max(maxY, data);


			}

	

	node->boundsMin.y = minY;
	node->boundsMax.y = maxY;

	


	if (node->childs.empty()) return;

	float nodeWidth = (rightBottom.x - leftTop.x)*0.5f;
	float nodeDepth = (rightBottom.y - leftTop.y)*0.5f;


    UpdateHeight(node->childs[0], leftTop, Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), heightMap);
	UpdateHeight(node->childs[1], Vector2(leftTop.x + nodeWidth, leftTop.y), Vector2(rightBottom.x, leftTop.y + nodeDepth), heightMap);
	UpdateHeight(node->childs[2], Vector2(leftTop.x, leftTop.y + nodeDepth), Vector2(leftTop.x + nodeDepth, rightBottom.y), heightMap);
	UpdateHeight(node->childs[3], Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), rightBottom, heightMap);

}

void QuadTree::UpdateHeight(shared_ptr<QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom, Vector3 min, Vector3 max)
{


	static const float TileSize = QuadTreeSize;
	static const float  tolerance = 0.01f;



	Vector3 pos = (node->boundsMax + node->boundsMin)*0.5f;

	bool IsInside = true;
	if (
		pos.x < min.x || pos.x > max.x ||
		pos.y < min.y || pos.y > max.y ||
		pos.z < min.z || pos.z > max.z)
		IsInside = false;


	if (IsInside)
	{
		node->boundsMin.y = max.y - 2.0f;
		node->boundsMax.y = max.y;
	}


	




	if (node->childs.empty()) return;

	float nodeWidth = (rightBottom.x - leftTop.x)*0.5f;
	float nodeDepth = (rightBottom.y - leftTop.y)*0.5f;


	UpdateHeight(node->childs[0], leftTop, Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), min,max);
	UpdateHeight(node->childs[1], Vector2(leftTop.x + nodeWidth, leftTop.y), Vector2(rightBottom.x, leftTop.y + nodeDepth), min, max);
	UpdateHeight(node->childs[2], Vector2(leftTop.x, leftTop.y + nodeDepth), Vector2(leftTop.x + nodeDepth, rightBottom.y), min, max);
	UpdateHeight(node->childs[3], Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), rightBottom, min, max);
}

shared_ptr<QuadTreeNode> QuadTree::FindNode(shared_ptr<QuadTreeNode> node, const uint & id)
{
	if (node->ID == id)
	{
		return node;
	}
	
	for (auto& child : node->childs)
	{
		
		if (FindNode(child, id))
		{
			return child;
		}
	}

	return nullptr;
}

bool QuadTree::Intersection(uint & id)
{
	Matrix V, P;
	 GlobalData::GetView(&V);
	 GlobalData::GetProj(&P);

	GetRay(&ray, V, P);
	
	if (!root->Intersection(ray, pos, id))
	{
		return false;
	}


	
	return true;
}


bool QuadTree::Intersection(Vector3 & Pos)
{
	Matrix V, P;
	GlobalData::GetView(&V);
	GlobalData::GetProj(&P);

	GetRay(&ray,  V, P);
	uint id;
	if (!root->Intersection(ray, pos,id))
	{
		return false;
	}

	
	Pos = pos;

	return true;
}
void QuadTree::Intersection( Matrix *const matrix, uint& ID)
{
	
//	ray.org = Vector3(matrix->m[3][0], matrix->m[3][1] + 100, matrix->m[3][2]);
	ray.dir = Vector3(0.0f, -1.0f, 0.0f);
	Vector3 actorPos;
	uint id;
	if (!root->Intersection(ray, actorPos, id))
	{
		return;
	}
	Vector2 lerp;
	//lerp.Lerp(&Vector2(0, matrix->m[3][1]), &Vector2(0,actorPos.y), Time::Delta()*5.0f);
	//matrix->m[3][1] = lerp.y;
	ID = id;
}
bool QuadTree::InBounds(uint row, uint col)
{
	return row >= 0 && row < height && col >= 0 && col < width;
}
void QuadTree::CreateQuadTree(Vector2 lt, Vector2 rb,float** heightMap)
{
	CreateQuadTree(root, lt, rb,-1, heightMap);
}

void QuadTree::CreateQuadTree(shared_ptr<QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom,int hierarchyIndex,float** heightMap)
{
	

	static const float TileSize = QuadTreeSize;
	static const float  tolerance = 0.01f;

	Vector2 minMaxY;
	float minX;
	float maxX;
	float minZ;
	float maxZ;
	
	minX = leftTop.x ;
	maxX = rightBottom.x;
	minZ = leftTop.y;
	maxZ = rightBottom.y;

	minX -= tolerance;
	maxX += tolerance;
	minZ += tolerance;
	maxZ -= tolerance;

	if (heightMap != nullptr)
	{
		float minY = FLT_MAX;
		float maxY = -FLT_MAX;

		
		uint lt = ((uint)leftTop.y-512)*-1;
		uint rb =((uint)rightBottom.y-512)*-1;


			for (uint x = (uint)leftTop.x; x < (uint)rightBottom.x; x++)
			for (uint y = rb; y < lt; y++)
            {
             
             
            		float data = 0.0f;
             
            		data = heightMap[x][y];
             
            		minY = min(minY, data);
            		maxY = max(maxY, data);
            		
            	
            }
                
		minMaxY = Vector2(minY, maxY);
		
	}
	else
	{
		minMaxY = Vector2(0.0f, 5.0f);
	}



	node->boundsMin = Vector3(minX, minMaxY.x, minZ);
	node->boundsMax = Vector3(maxX, minMaxY.y, maxZ);
	
	auto temp = (node->boundsMin + node->boundsMax)*0.5f;

	uint x = static_cast<uint>(temp.x)-1;
	uint z = static_cast<uint>(temp.z)-1;


	uint index = 512 * z + x;
	node->ID = index;

	float nodeWidth = (rightBottom.x - leftTop.x)*0.5f;
	float nodeDepth = (rightBottom.y - leftTop.y)*0.5f;


	// we will recurse until the terrain regions match our logical terrain tile sizes

	node->childs.clear();
	node->hierarchyIndex = hierarchyIndex + 1;
	
	if (nodeWidth >= TileSize && nodeDepth >= TileSize)
	{
		shared_ptr<QuadTreeNode> child1 = make_shared<QuadTreeNode>();
		
		shared_ptr<QuadTreeNode> child2 = make_shared<QuadTreeNode>();
		
		shared_ptr<QuadTreeNode> child3 = make_shared<QuadTreeNode>();
		
		shared_ptr<QuadTreeNode> child4 = make_shared<QuadTreeNode>();
		

		node->childs.emplace_back(child1);
		node->childs.emplace_back(child2);
		node->childs.emplace_back(child3);
		node->childs.emplace_back(child4);


		CreateQuadTree(node->childs[0],  leftTop, Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth),  node->hierarchyIndex, heightMap);
		CreateQuadTree(node->childs[1],  Vector2(leftTop.x + nodeWidth, leftTop.y), Vector2(rightBottom.x, leftTop.y + nodeDepth), node->hierarchyIndex, heightMap);
		CreateQuadTree(node->childs[2],  Vector2(leftTop.x, leftTop.y + nodeDepth), Vector2(leftTop.x + nodeDepth, rightBottom.y), node->hierarchyIndex, heightMap);
		CreateQuadTree(node->childs[3],  Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), rightBottom, node->hierarchyIndex, heightMap);



	}

}
void QuadTree::UpdateHeight(Vector2 leftTop, Vector2 rightBottom, float ** heightMap)
{
	UpdateHeight(root, leftTop, rightBottom, heightMap);
}
void QuadTree::UpdateHeight(Vector3 min, Vector3 max)
{
	UpdateHeight(root, Vector2(0,0), Vector2(512,512), min,max);
}

void QuadTree::GetRay(Ray* ray,const Matrix & v, const Matrix & p)
{
	Vector3 mouse = Mouse::Get()->GetPosition();

	Vector2 point;
	//Inv Viewport
	{
		point.x = (((2.0f*mouse.x) / D3D::Width()) - 1.0f);
		point.y = (((2.0f*mouse.y) / D3D::Height()) - 1.0f)*-1.0f;

	}

	//Inv Projection
	{
		point.x = point.x / p._11;
		point.y = point.y / p._22;
	}

	Vector3 cameraPosition;
	//inv View
	{
		Matrix invView;
		Matrix::Inverse(&invView, &v);
	
		cameraPosition = Vector3(invView._41, invView._42, invView._43);

		ray->dir=Vector3::TransformNormal(Vector3(point.x, point.y, 1), &invView);
		
		ray->dir=Vector3::Normalize(ray->dir);
	}
	//inv world
	{

		Matrix invWorld;
		
		ray->org=Vector3::TransformCoord(cameraPosition, &invWorld);
		ray->dir=Vector3::TransformNormal( ray->dir, &invWorld);//직선이 맞을 물체와 카메라의 공간을 일치시켜주기위해서
		ray->dir=Vector3::Normalize(ray->dir);
		ray->org=Vector3::TransformCoord(ray->org, &invWorld);
	}
}
bool Compare(shared_ptr<QuadTreeNode>& a, shared_ptr<QuadTreeNode>& b)
{
	return a->dist > b->dist;
}

bool QuadTreeNode::Intersection(const Ray& ray, Vector3 & Pos, uint& ID, const uint& FindHierarchyIndex)
{
    Pos = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
	hitted = false;
	if (childs.empty())
	{
		float d;
		if (!IntersectionAABB(ray,Pos, d))
		{
    		return false;
		}
		Pos = (boundsMin + boundsMax)*0.5f;
		hitted = true;
		
		if (hierarchyIndex == FindHierarchyIndex)
		ID = this->ID;
		
		return true;
	}
	
	hittedChilds.clear();
	hittedChilds.shrink_to_fit();
	
	for (auto& child : childs)
	{
		float cd = 0;
		if (child->IntersectionAABB(ray, Pos, cd))
		{
			child->dist += Math::Random(-0.001f, 0.001f);
			hittedChilds.emplace_back(child);
			
		}
	}

	if (hittedChilds.empty())
	{
		return false;
	}

	if(hittedChilds.size()>1)
	sort(hittedChilds.begin(), hittedChilds.end(), Compare);
	
	
	const Vector3& bestHit = ray.org + 1000 * ray.dir;

	for (auto& p : hittedChilds)
	{
		Vector3 thisHit;
		bool wasHit = p->Intersection(ray, thisHit,ID);
		if (!wasHit)
		{
			//cout << "Not was Hit" << endl;
			continue;
		}
		/*Vector3 nor;
		D3DXVec3Normalize(&nor, &(thisHit - org));
		Vector3 dotDir = dir;
		float dot = D3DXVec3Dot(&nor,&dir);
		if (dot > 1.4f||dot<0.6f) {
			cout << "False due to dot" << endl;
			continue;
		}*/
		
		Vector3 delta = ray.org - thisHit;
		float l1 = delta.LengthSq();
		Vector3 bestDelta = ray.org - bestHit;
		float l2 = bestDelta.LengthSq();
		// check that the intersection is closer than the nearest intersection found thus far
		if (!(l1 < l2))
		{
			//cout << "check that the intersection LengthSquard" << endl;
			continue;
		}

	
		hitted = true;
		Pos = thisHit;
		if (hierarchyIndex == FindHierarchyIndex)
		{
			ID = p->ID;
		}
		return true;
		
	}

	return false;
}

bool QuadTreeNode::IntersectionAABB(const Ray & ray, Vector3 & Pos, float & d)
{
	float t_min = 0;
	float t_max = FLT_MAX;

	
	for (int i = 0; i < 3; i++)
	{
		Vector3 dir = ray.dir;
		Vector3 org = ray.org;
		if (abs(dir[i]) < Math::EPSILON)
		{
			
			if (org[i] < boundsMin[i] ||
				org[i] >boundsMax[i])
			{
				return false;
			}

		}
		else
		{
			float denom = 1.0f / dir[i];
			float t1 = (boundsMin[i] - org[i]) * denom;
			float t2 = (boundsMax[i] - org[i]) * denom;

			if (t1 > t2)
			{
				swap(t1, t2);
			}

			t_min = max(t_min, t1);
			t_max = min(t_max, t2);

			if (t_min > t_max)
			{
				return false;
			}


		}
	}
	
	//Vector3 hit =  org + t_min * dir;


	//Pos = (boundsMin + boundsMax)*0.5f;
	
	d = t_min;
	return true;
}
