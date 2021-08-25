

#define BLOCK_SIZE 256
#define LOOP_UNROLL 8

// Constants used by the compute shader
cbuffer SimulationParameters : register(b0)
{
    float timestep : packoffset(c0.x);
    float softeningSquared : packoffset(c0.y);
    uint numParticles : packoffset(c0.z);
    uint readOffset : packoffset(c0.w);
    
    uint writeOffset : packoffset(c1.x);
    float distance : packoffset(c1.y);
    float timer : packoffset(c1.z);
    float runningTime : packoffset(c1.w);
};	


RWStructuredBuffer<float4> particles : register(u0);

float3 bodyBodyInteraction(float4 bi, float4 bj) 
{
    // r_ij  [3 FLOPS]
    float3 r = bi - bj;

    // distSqr = dot(r_ij, r_ij) + EPS^2  [6 FLOPS]
    float distSqr = dot(r, r);
    distSqr += softeningSquared;

    // invDistCube =1/distSqr^(3/2)  [4 FLOPS (2 mul, 1 sqrt, 1 inv)]
    float invDist = 1.0f / sqrt(distSqr);
	float invDistCube =  invDist * invDist * invDist;

    // s = m_j * invDistCube [1 FLOP]
    float s = bj.w * invDistCube;

    // a_i =  a_i + s * r_ij [6 FLOPS]  
    return r*s;
}

groupshared float4 sharedPos[BLOCK_SIZE];


float3 gravitation(float4 myPos, float3 accel)
{
    unsigned int i = 0;

    [unroll]
    for (unsigned int counter = 0; counter < BLOCK_SIZE; ) 
    {
       accel += bodyBodyInteraction(sharedPos[i++], myPos);
        counter++;

    }

    return accel;
}


float3 computeBodyAccel(float4 bodyPos, uint threadId, uint blockId)
{
    float3 acceleration = {0.0f, 0.0f, 0.0f};
    
    uint p = BLOCK_SIZE;
    uint n = numParticles;
    uint numTiles = n / p;
    
    for (uint tile = 0; tile < numTiles; tile++) 
    {
        sharedPos[threadId] = particles[readOffset + tile * p + threadId];
       
        GroupMemoryBarrierWithGroupSync();

        acceleration = gravitation(bodyPos, acceleration);
        
        GroupMemoryBarrierWithGroupSync();
    }

    return acceleration;
}


[numthreads(BLOCK_SIZE,1,1)]
void CSMAIN(uint threadId : SV_GroupIndex,
                 uint3 groupId        : SV_GroupID,
                 uint3 globalThreadId : SV_DispatchThreadID)
{
	float3 pos = particles[readOffset + globalThreadId.x].xyz; 
    float4 vel = particles[2 * numParticles + globalThreadId.x]; 
   
	// compute acceleration
    float3 accel = computeBodyAccel(float4(pos, 1.0f), threadId, groupId.x);
	
	// Leapfrog-Verlet integration of velocity and position
    vel.xyz += accel * timestep;
    pos.xyz += vel * timestep;
   
    
     particles[writeOffset + globalThreadId.x] = float4(pos, 1.0f);
     particles[2 * numParticles + globalThreadId.x] = vel;
}
