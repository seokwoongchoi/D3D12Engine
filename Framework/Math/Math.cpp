#include "Framework.h"
#include "Math.h"
#include <random>

std::random_device rd;
std::mt19937 random_engine(rd());

auto Math::Random(const int & min, const int & max) -> const int
{
    //std::uniform_int_distribution<int> test(min, max);
    //int ramdom = test(random_engine);

    //Uniform Initialization
    return std::uniform_int_distribution<int>{min, max}(random_engine);
}

auto Math::Random(const float & min, const float & max) -> const float
{
    return std::uniform_real_distribution<float>{min, max}(random_engine);
}
