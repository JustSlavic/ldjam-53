#ifndef MATH_TRANSFORM_HPP
#define MATH_TRANSFORM_HPP

#include <base.hpp>
#include <math/matrix3.hpp>
#include <math/vector3.hpp>


namespace math {


struct transform
{
    matrix3 matrix;
    vector3 offset;
};


vector3 transform_vector(transform const& t, vector3 p)
{
    vector3 result = p * t.matrix;
    return result;
}

vector3 transform_point(transform const& t, vector3 p)
{
    vector3 result = p * t.matrix + t.offset;
    return result;
}


} // namespace math


#endif // MATH_TRANSFORM_HPP
