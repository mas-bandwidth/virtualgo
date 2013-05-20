#ifndef BOARD_H
#define BOARD_H

/*
    Go board.

    We model the go board as an axis aligned rectangular prism.

    Since the floor is the plane z = 0, the top surface of the board
    is the plane z = thickness.

    Go board dimensions:

        Board width                 424.2mm
        Board length                454.5mm
        Board thickness             151.5mm
        Line spacing width-wise     22mm
        Line spacing length-wise    23.7mm
        Line width                  1mm
        Star point marker diameter  4mm
        Border                      15mm

    https://en.wikipedia.org/wiki/Go_equipment#Board
*/

struct BoardParams
{
    BoardParams()
    {
        cellWidth = 2.2f;
        cellHeight = 2.37f;
        border = 1.5f;
        thickness = 1.0f;
        lineWidth = 0.1;
        starPointRadius = 0.2f;
    }

    float cellWidth;
    float cellHeight;
    float border;
    float thickness;
    float lineWidth;
    float starPointRadius;
};

enum PointState
{
    Empty,
    White,
    Black
};

class Board
{
    BoardParams params;

    int size;                               // size of board. eg size of 9 means 9x9 board
    
    float width;                            // width of board (along x-axis)
    float height;                           // height of board (along y-axis)

    float halfWidth;
    float halfHeight;

    PointState * pointState;
    uint16_t * pointStoneId;

public:

    Board()
    {
        size = 0;
        width = 0;
        height = 0;
        halfWidth = 0;
        halfHeight = 0;
        pointState = NULL;
        pointStoneId = NULL;
    }

    Board( int size, const BoardParams & params = BoardParams() )
    {
        Initialize( size, params );
    }

    ~Board()
    {
        Free();
    }

    void Initialize( int size, const BoardParams & params = BoardParams() )
    {
        this->size = size;
        this->params = params;

        this->width = ( size - 1 ) * params.cellWidth + params.border * 2;
        this->height = ( size - 1 ) * params.cellHeight + params.border * 2;
        
        halfWidth = width / 2;
        halfHeight = height / 2;

        const int numPoints = size * size;

        pointState = new PointState[numPoints];
        pointStoneId = new uint16_t[numPoints];

        memset( pointState, 0, numPoints * sizeof(PointState) );
        memset( pointStoneId, 0, numPoints * 2 );
    }

    void Free()
    {
        this->size = 0;
        delete [] pointState;
        delete [] pointStoneId;
        pointState = NULL;
        pointStoneId = NULL;
    }

    int GetSize() const
    {
        return size;
    }

    float GetWidth() const
    {
        return width;
    }

    float GetHeight() const
    {
        return height;
    }

    void SetThickness( float thickness )
    {
        this->params.thickness = thickness;
    }

    float GetThickness() const
    {
        return params.thickness;
    }
   
    float GetHalfWidth() const
    {
        return halfWidth;
    }

    float GetHalfHeight() const
    {
        return halfHeight;
    }

    float GetCellHeight() const
    {
        return params.cellHeight;
    }

    float GetCellWidth() const
    {
        return params.cellWidth;
    }

    void GetBounds( float & bx, float & by )
    {
        bx = halfWidth;
        by = halfHeight;
    }

    vec3f GetPointPosition( int row, int column ) const
    {
        assert( row >= 1 );
        assert( column >= 1 );
        assert( row <= size );
        assert( column <= size );

        const float w = params.cellWidth;
        const float h = params.cellHeight;
        const float z = params.thickness;

        const int n = ( GetSize() - 1 ) / 2;

        return vec3f( ( -n + column - 1 ) * w, ( -n + row - 1 ) * h, z );
    }

    void GetStarPoints( vec3f * pointPosition, int & numStarPoints ) const
    {
        // todo: generalize this such that it works with different board sizes
        assert( size == 9 );
        numStarPoints = 5;
        pointPosition[0] = GetPointPosition( 3, 3 );
        pointPosition[1] = GetPointPosition( 7, 3 );
        pointPosition[2] = GetPointPosition( 3, 7 );
        pointPosition[3] = GetPointPosition( 7, 7 );
        pointPosition[4] = GetPointPosition( 5, 5 );
    }

    PointState GetPointState( int row, int column ) const
    {
        assert( row >= 1 );
        assert( column >= 1 );
        assert( row <= size );
        assert( column <= size );
        return pointState[(column-1)+(row-1)*size];
    }

    void SetPointState( int row, int column, PointState state )
    {
        assert( row >= 1 );
        assert( column >= 1 );
        assert( row <= size );
        assert( column <= size );

        // IMPORTANT: this will fire if you try to add a stone 
        // to a point that is already occupied by another stone!
        assert( state == Empty || state != Empty && pointState[(column-1)+(row-1)*size] == Empty );

        pointState[(column-1)+(row-1)*size] = state;
    }

    uint16_t GetPointStoneId( int row, int column ) const
    {
        assert( row >= 1 );
        assert( column >= 1 );
        assert( row <= size );
        assert( column <= size );
        return pointStoneId[(column-1)+(row-1)*size];
    }

    void SetPointStoneId( int row, int column, uint16_t stoneId )
    {
        assert( row >= 1 );
        assert( column >= 1 );
        assert( row <= size );
        assert( column <= size );
        pointStoneId[(column-1)+(row-1)*size] = stoneId;
    }

    bool FindNearestPoint( const vec3f & position, int & row, int & column )
    {
        vec3f origin = GetPointPosition( 1, 1 );

        vec3f delta = position - origin;

        const float dx = delta.x() + params.cellWidth/2;
        const float dy = delta.y() + params.cellHeight/2;

        int ix = (int) floor( dx / params.cellWidth );
        int iy = (int) floor( dy / params.cellHeight );

        row = iy + 1;
        column = ix + 1;

        if ( row < 1 || row > size || column < 1 || column > size )
            return false;
                
        return true;
    }

    struct EmptyPointInfo
    {
        int row;
        int column;
        vec3f position;
    };

    void AddEmptyPoint( std::vector<EmptyPointInfo> & emptyPoints, int row, int column )
    {
        if ( row < 1 || row > size || column < 1 || column > size )
            return;

        if ( GetPointState( row, column ) == Empty )
        {
            EmptyPointInfo info;
            info.row = row;
            info.column = column;
            info.position = GetPointPosition( row, column );
            emptyPoints.push_back( info );
        }
    }

    bool FindNearestEmptyPoint( const vec3f & position, int & row, int & column )
    {
        vec3f origin = GetPointPosition( 1, 1 );

        vec3f delta = position - origin;

        const float dx = delta.x() + params.cellWidth/2;
        const float dy = delta.y() + params.cellHeight/2;

        int ix = (int) floor( dx / params.cellWidth );
        int iy = (int) floor( dy / params.cellHeight );

        row = iy + 1;
        column = ix + 1;

        std::vector<EmptyPointInfo> emptyPoints;

        AddEmptyPoint( emptyPoints, row - 1, column - 1 );
        AddEmptyPoint( emptyPoints, row,     column - 1 );
        AddEmptyPoint( emptyPoints, row + 1, column - 1 );

        AddEmptyPoint( emptyPoints, row - 1, column );
        AddEmptyPoint( emptyPoints, row,     column );
        AddEmptyPoint( emptyPoints, row + 1, column );

        AddEmptyPoint( emptyPoints, row - 1, column + 1 );
        AddEmptyPoint( emptyPoints, row,     column + 1 );
        AddEmptyPoint( emptyPoints, row + 1, column + 1 );

        if ( emptyPoints.size() == 0 )
            return false;

        float nearestDistance = FLT_MAX;
        EmptyPointInfo * nearestEmptyPoint = NULL;

        for ( int i = 0; i < emptyPoints.size(); ++i )
        {
            const float distance = length( emptyPoints[i].position - position );
            if ( distance < nearestDistance )
            {
                nearestDistance = distance;
                nearestEmptyPoint = &emptyPoints[i];
            }
        }

        assert( nearestEmptyPoint );

        row = nearestEmptyPoint->row;
        column = nearestEmptyPoint->column;

        return true;
    }

    const BoardParams & GetParams() const
    {
        return params;
    }
};

#endif
