// This code is in the public domain -- Ignacio Casta�o <castano@gmail.com>

#pragma once
#ifndef NV_MESH_TRIMESH_H
#define NV_MESH_TRIMESH_H

#include "nvcore/Array.h"
#include "nvmath/Vector.inl"
#include "nvmesh/nvmesh.h"
#include "nvmesh/BaseMesh.h"

namespace nv
{
    /// Triangle mesh.
    class TriMesh : public BaseMesh
    {
    public:
        struct Face;
        typedef BaseMesh::Vertex Vertex;

        TriMesh(uint faceCount, uint vertexCount) : BaseMesh(vertexCount), m_faceArray(faceCount) {}

        // Face methods.
        uint faceCount() const { return m_faceArray.count(); }
        const Face & faceAt(uint i) const { return m_faceArray[i]; }
        Face & faceAt(uint i) { return m_faceArray[i]; }
        const Array<Face> & faces() const { return m_faceArray; }
        Array<Face> & faces() { return m_faceArray; }

        NVMESH_API Vector3 faceNormal(uint f) const;
        NVMESH_API const Vertex & faceVertex(uint f, uint v) const;

        friend Stream & operator<< (Stream & s, BaseMesh & obj);

    private:

        Array<Face> m_faceArray;

    };


    /// TriMesh face.
    struct TriMesh::Face
    {
        uint id;
        uint v[3];
    };

} // nv namespace

#endif // NV_MESH_TRIMESH_H
