#ifndef SCENE_GRID_H
#define SCENE_GRID_H

#include <vector>
#include <algorithm>

struct Cell
{
	std::vector<uint16_t> objects;
    
    void clear()
    {
        objects.clear();
    }
};

class SceneGrid
{
	float res;							// grid resolution, eg. size of grid cube side

	float width;						// x dimension: [-width/2,+width/2]
	float height;						// y dimension: [-height/2,+height/2]
	float depth;						// z dimension: [0,depth]

	int nx,ny,nz;						// grid bounds in integer coordinates [0,nx-1]

	std::vector<Cell> cells;

public:

	void Initialize( float res, float width, float height, float depth )
	{
		this->res = res;
        
        this->width = width;
        this->height = height;
        this->depth = depth;

		nx = (int) ceil( width / res );
		ny = (int) ceil( height / res );
		nz = (int) ceil( depth / res );

        const int size = nx * ny * nz;
        assert( size > 0 );
		cells.resize( size );

		for ( int i = 0; i < cells.size(); ++i )
			cells[i].objects.reserve( 32 );
	}

	void GetCellCoordinates( const vec3f & position, int & x, int & y, int & z )
	{
		x = clamp( (int) floor( ( position.x() + width/2 ) / res ), 0, nx - 1 );
		y = clamp( (int) floor( ( position.y() + height/2 ) / res ), 0, ny - 1 );
		z = clamp( (int) floor( position.z() / res ), 0, nz - 1 );
	}

	int GetCellIndex( int x, int y, int z )
	{
		x = clamp( x, 0, nx - 1 );
		y = clamp( y, 0, ny - 1 );
		z = clamp( z, 0, nz - 1 );

		int index = x + y*nx + z*nx*ny;

		assert( index >= 0 );
		assert( index < cells.size() );

		return index;
	}

	Cell & GetCell( int index )
	{
		return cells[index];
	}

	Cell & GetCellAtIntCoords( int ix, int iy, int iz )
	{
		int index = GetCellIndex( ix, iy, iz );
		return cells[index];
	}

	void clear()
	{
		for ( int i = 0; i < cells.size(); ++i )
			cells[i].clear();
	}

	void AddObject( uint32_t id, const vec3f & position )
	{
		int x,y,z;
		GetCellCoordinates( position, x, y, z );
		const int index = GetCellIndex( x, y, z );
		Cell & cell = GetCell( index );
		#if VALIDATION
		assert( std::find( cell.objects.begin(), cell.objects.end(), id ) == cell.objects.end() );
		#endif
		cell.objects.push_back( id );
	}

	void MoveObject( uint32_t id, const vec3f & previousPosition, const vec3f & currentPosition )
	{
		int prev_x,prev_y,prev_z;
		int curr_x,curr_y,curr_z;
		GetCellCoordinates( previousPosition, prev_x, prev_y, prev_z );
		GetCellCoordinates( currentPosition, curr_x, curr_y, curr_z );
		const int prev_index = GetCellIndex( prev_x, prev_y, prev_z );
		const int curr_index = GetCellIndex( curr_x, curr_y, curr_z );
		if ( prev_index != curr_index )
        {
            Cell & prev = GetCell( prev_index );
            Cell & curr = GetCell( curr_index );
			prev.objects.erase( std::remove( prev.objects.begin(), prev.objects.end(), id ), prev.objects.end() );
			#if VALIDATION
            assert( std::find( curr.objects.begin(), curr.objects.end(), id ) == curr.objects.end() );
            #endif
            curr.objects.push_back( id );
        }
	}

	void RemoveObject( uint32_t id, const vec3f & position )
	{
		int x,y,z;
		GetCellCoordinates( position, x, y, z );
		const int index = GetCellIndex( x, y, z );
		Cell & cell = GetCell( index );
		cell.objects.erase( std::remove( cell.objects.begin(), cell.objects.end(), id ), cell.objects.end() );
	}

	void GetIntegerBounds( int & nx, int & ny, int & nz )
	{
		nx = this->nx;
		ny = this->ny;
		nz = this->nz;
	}
};

#endif
