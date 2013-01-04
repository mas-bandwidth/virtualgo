#ifndef MESH_H
#define MESH_H

#include <vector>
#include <list>
#include <assert.h>

struct Vertex
{
    vec3f position;
    vec3f normal;
};

// TODO: grid cell is an intermediary between vertex and bucket
// it is necessary to first check what grid cell the current vertex
// is in, and if this grid cell 

class Mesh
{
public:

    Mesh( int numBuckets = 1024 )
    {
        this->numBuckets = numBuckets;
        buckets = new std::list<int>[numBuckets];
    }

    ~Mesh()
    {
        delete [] buckets;
        buckets = NULL;
    }

    void AddTriangle( const Vertex & a, const Vertex & b, const Vertex & c )
    {
        indexBuffer.push_back( AddVertex( a ) );
        indexBuffer.push_back( AddVertex( b ) );
        indexBuffer.push_back( AddVertex( c ) );
    }

    Vertex * GetVertexBuffer() { assert( vertexBuffer.size() ); return &vertexBuffer[0]; }

    int * GetIndexBuffer() { assert( indexBuffer.size() ); return &indexBuffer[0]; }

protected:

    uint32_t GetGridCellBucket( int x, int y, int z )
    {
        const unsigned int magic1 = 0x8da6b343;
        const unsigned int magic2 = 0xd8163841;
        const unsigned int magic3 = 0x7fffffff;
        const uint32_t hash = magic1 * x + magic2 * y + magic3 * z;
        return hash % numBuckets;
    }

    int AddVertex( const Vertex & vertex, const float tolerance = 0.001f )
    {
        int index = -1;
        // todo: find bucket for vertex
        if ( index != -1 )
            return index;
        indexBuffer.push_back( index );
        return indexBuffer.size() - 1;
    }

private:

    std::vector<Vertex> vertexBuffer;
    
    std::vector<int> indexBuffer;

    int numBuckets;
    std::list<int> * buckets;
};

#endif