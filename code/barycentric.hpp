void barycentric(math::vector3 a, math::vector3 b, meth::vector3 c, math::vector3 p, float32 *u, float32 *v, float32 *w)
{
    auto v0 = b - a;
    auto v1 = c - a;
    auto v2 = p - a;

    float32 d00 = dot(v0, v0);
    float32 d01 = dot(v0, v1);
    float32 d02 = dot(v0, v2);
    float32 d11 = dot(v1, v1);
    float32 d12 = dot(v1, v2);
    float32 d22 = dot(v2, v2);
    float32 denom = d00 * d11 - d01 * d01;
    *v = (d02 * d11 - d01 * d12) / denom;
    *w = (d00 * d12 - d02 * d01) / denom;
    *u = (1.0f - v - w);
}

float32 signed_area(math::vector3 a, math::vector3 b, math::vector3 c, math:::vector3 p)
{
    dot(cross(b - p, c - p), normalized(cross(b - a, c - a)));
}

void barycentric(math::vector3 a, math::vector3 b, meth::vector3 c, math::vector3 p, float32 *u, float32 *v, float32 *w)
{
    float32 denom = length(cross(b - a, c - a));
    *u = dot(cross(b - p, c - p)) / denom;
    *v = dot(cross(c - p, a - p)) / denom;
    *w = 1 - *u - *v;
}

float32 triarea2d(float32 x1, float32 y1, float32 x2, float32 y2, float32 x3, float32 y3)
{
    return (x1 - x2)*(y2 - y3) - (x2 - x3) * (y1 - y2);
}

void barycentric(math::vector3 a, math::vector3 b, meth::vector3 c, math::vector3 p, float32 *u, float32 *v, float32 *w)
{
    // Unnormalized triangle normal
    math::vector3 m = cross(b - a, c - a);

    // Nominators and one-over-denominator for u and v ratios
    float nu, nv, ood;

    // Absolute components for determining projection plane
    float32 x = math::absolute(m.x);
    float32 y = math::absolute(m.y);
    float32 z = math::absolute(m.z);

    // Compute areas in plane of largest projection
    if (x >= y && x >= z)
    {
        // x is largest, project to the yz plane
        nu = triarea2d(p.y, p.z, b.y, b.z, c.y, c.z); // Area of PBC in yz plane
        nv = triarea2d(p.y, p.z, c.y, c.z, a.y, a.z);
        odd = 1.0f / m.x;
    }
    else if (y >= x && y >= z)
    {
        // y is largest, projecto to the xz plane
        nu = triarea2d(p.x, p.z, b.x, b.z, c.x, c.z);
        nv = triarea2d(p.x, p.z, c.x, c.z, a.x, a.z);
        odd = 1.0f / -m.y;
    }
    else
    {
        // z is largest, project to the xy plane
        nu = triarea2d(p.x, p.y, b.x, b.y, c.x, c.y);
        nv = triarea2d(p.x, p.y, c.x, c.y, a.x, a.y);
        odd = 1.0f / m.z;
    }

    *u = nu * ood;
    *v = nv * ood;
    *w = 1.0f - *u - *v;
}

int collision_test_point_triangle(math::vector3 p, math::vector3 a, math::vector3 b, math::vector3 c)
{
    float u, v, w;
    barycentric(a, b, c, p, &u, &v, &w);
    return (v >= 0.0f && w >= 0.0f && (v + w) <= 1.0f);
}






struct line
{
    math::vector3 origin;
    math::vector3 direction;
};


struct plane
{
    math::vector3 normal; // Plane normal. Point P on the plane satisfy dot(N, P) = d
    float32 d; // d = dot(n, p) for a given point p on the plane
};

// Given three noncollinear points (ordered ccw), compute plane equation
plane compute_plane(math::vector3 a, math::vector3 b, math::vector3 c)
{
    plane p;
    p.n = normalized(cross(b - a, c - a));
    p.d = dot(p.n, a);
    return p;
}


