#pragma once
#define terrain_gridpoints					512
#define QuadTreeSize 1.0f
struct Ray
{
	Vector3 org = Vector3(0, 0, 0);
	Vector3 dir = Vector3(0, 0, 0);
};
struct QuadTreeNode
{
	bool Intersection(const Ray& ray, Vector3& Pos,uint& ID, const uint& FindHierarchyIndex = 5);
	bool IntersectionAABB(const Ray& ray, Vector3& Pos, float& d);

	Vector3 boundsMin;
	Vector3 boundsMax;
	
	vector< shared_ptr<QuadTreeNode>>childs;
	vector<shared_ptr<QuadTreeNode>> hittedChilds;
	

	int hierarchyIndex = -1;
	uint ID = 0;
	bool hitted = false;
	float dist = 0.0f;

	

	
};


class QuadTree
{
public:
	QuadTree();
	~QuadTree();


	shared_ptr<QuadTreeNode> FindNode(shared_ptr<QuadTreeNode> node, const uint& id);
	bool Intersection(uint& id);
	bool Intersection(Vector3& Pos);
	void Intersection(Matrix *const matrix, uint& ID);
	bool InBounds(uint row, uint col);
	void CreateQuadTree(Vector2 lt,Vector2 rb, float** heightMap = nullptr);

	void UpdateHeight( Vector2 leftTop, Vector2 rightBottom, float** heightMap = nullptr);
	void UpdateHeight(Vector3 min,Vector3 max);
	void GetRay(Ray* ray,  const Matrix& v, const Matrix& p);
	shared_ptr<QuadTreeNode> GetRoot(){	return root;}


private:
	void CreateQuadTree(shared_ptr< QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom, int hierarchyIndex , float** heightMap=nullptr);

	void UpdateHeight(shared_ptr< QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom, float** heightMap = nullptr);
	void UpdateHeight(shared_ptr< QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom, Vector3 min,Vector3 max);
private:
	
	shared_ptr< QuadTreeNode> root;
private:
	Ray ray;


	Vector3 pos;

	float height;
	float width;

	uint ID = 0;


};

