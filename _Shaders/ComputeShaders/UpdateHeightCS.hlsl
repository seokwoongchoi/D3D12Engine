#define Width 512
#define EPSILON  0.000001f
cbuffer UpdateDesc : register(b0)
{
    
    float3 Location : packoffset(c0.x);
    uint Range: packoffset(c0.w);
    
    uint BrushShape : packoffset(c1.x);
    float RaiseSpeed : packoffset(c1.y);
};

RWTexture2D<float4> Update : register(u0);


void Square(uint4 box,uint dispatch)
{
    
    uint3 CurPixel = uint3(box.x + dispatch % (box.y - box.x), box.w + dispatch / box.z, 0);
    if (CurPixel.y < box.z)
    {
        Update[CurPixel.xy] += float4(0, 0, 0, RaiseSpeed);
    }
}

void Circle(uint4 box, uint dispatch)
{
       
    uint3 CurPixel = uint3(box.x + dispatch % (box.y - box.x), box.w + dispatch / box.z,0);
    if (CurPixel.y < box.z)
    {
        float dx = CurPixel.x - (Location.x );
        float dy = CurPixel.y - (Width - Location.z);
        dx *= dx;
        dy *= dy;
        float distanceSquared = dx + dy;
        float radiusSquared = Range * Range;

        if (distanceSquared <= radiusSquared)
            Update[CurPixel.xy] += float4(0, 0, 0, RaiseSpeed);
    }
   
}

[numthreads(Width, 1, 1)]
void UpdateHeight(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 hit = 0;
  //  IntersectionAABB(Position, Direction, hit);
    
    //float3 Location = float3(hit);
   
    float l = (Location.x - Range) ;
    float r = (Location.x + Range) ;
    float b = ((Width - Location.z) + Range);
    float t = ((Width - Location.z) - Range);

    uint lb = clamp(l, 0, Width - 1);
    uint rb = clamp(r, 0, Width - 1);
    uint bb = clamp(b, 0, Width - 1);
    uint tb = clamp(t, 0, Width - 1);
       
    [branch]
        if (BrushShape==1)
        {
            Square(uint4(lb, rb, bb, tb), dispatchThreadId.x);

        }
        else if (BrushShape == 2)
        {
            Circle(uint4(lb, rb, bb, tb), dispatchThreadId.x);

        }
 }  

[numthreads(Width, 1, 1)]
void Smoothing(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    
    if (BrushShape == 0)
        return;
    
    float l = (Location.x - Range);
    float r = (Location.x + Range);
    float b = ((Width - Location.z) + Range);
    float t = ((Width - Location.z) - Range);

    uint left = clamp(l, 0, Width-1);
    uint right = clamp(r, 0, Width - 1);
    uint bottom = clamp(b, 0, Width - 1);
    uint top = clamp(t, 0, Width - 1);
  
   
   uint4 box = uint4(left, right, bottom, top);
   
    uint3 CurPixel = uint3(box.x + dispatchThreadId.x % (box.y - box.x), box.w + dispatchThreadId.x / box.z, 0);
    if (CurPixel.y < box.z)
    {
        int adjacentSections = 0; // 인접하는곳의 갯수
        float sectionsTotal = 0.0f; // 자신과 인접하는곳(8군데)높이를 다 가져와서 더한걸 여기에 담아둘거임
	    
        sectionsTotal += Update[uint2(CurPixel.x - 1, CurPixel.y)].w;
        adjacentSections++;
       

        sectionsTotal += Update[uint2(CurPixel.x - 1, CurPixel.y - 1)].w;
        adjacentSections++;
      
        
        sectionsTotal += Update[uint2(CurPixel.x - 1, CurPixel.y + 1)].w;
        adjacentSections++;

        
        sectionsTotal += Update[uint2(CurPixel.x + 1, CurPixel.y)].w;
        adjacentSections++;
      

        sectionsTotal += Update[uint2(CurPixel.x + 1, CurPixel.y - 1)].w;
        adjacentSections++;
       
            
        sectionsTotal += Update[uint2(CurPixel.x + 1, CurPixel.y + 1)].w;
        adjacentSections++;
        
        sectionsTotal += Update[uint2(CurPixel.x, CurPixel.y - 1)].w;
        adjacentSections++;
       
        sectionsTotal += Update[uint2(CurPixel.x,  (CurPixel.y + 1))].w;
        adjacentSections++;
       
       
		//자신의 높이에다가, sectionsTotal로 구한 주변높이의 평균을 가감해줘서 부드럽게
       
       float factor = (Update[CurPixel.xy].w +  (sectionsTotal / adjacentSections)) * 0.5f;
        
        Update[CurPixel.xy] = float4(Update[CurPixel.xy].xyz,factor);

    }
  
  
 
}

