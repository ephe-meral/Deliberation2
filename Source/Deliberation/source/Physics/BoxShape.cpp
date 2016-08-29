#include <Deliberation/Physics/BoxShape.h>

namespace deliberation
{

BoxShape::BoxShape(const glm::vec3 & halfExtent):
    m_halfExtent(halfExtent)
{

}

const glm::vec3 & BoxShape::halfExtent()
{
    return m_halfExtent;
}

void BoxShape::setHalfExtent(const glm::vec3 & halfExtent)
{
    m_halfExtent = halfExtent;
}

Box BoxShape::instanciate(const Transform3D & transform) const
{
    auto x = transform.directionLocalToWorld(glm::vec3(m_halfExtent.x, 0.0f, 0.0f));
    auto y = transform.directionLocalToWorld(glm::vec3(0.0f, m_halfExtent.y, 0.0f));
    auto z = transform.directionLocalToWorld(glm::vec3(0.0f, 0.0f, m_halfExtent.z));

    return Box(transform.position(), x, y, z);
}

AABB BoxShape::bounds(const Transform3D & transform) const
{
    return instanciate(transform).bounds();
}

}