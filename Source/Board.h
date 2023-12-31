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

class Board
{
public:

    Board()
    {
        size = 0;
        width = 0;
        height = 0;
        halfWidth = 0;
        halfHeight = 0;
    }

    void Initialize( int size, const BoardParams & params = BoardParams() )
    {
        this->size = size;
        this->params = params;

        this->width = ( size - 1 ) * params.cellWidth + params.border * 2;
        this->height = ( size - 1 ) * params.cellHeight + params.border * 2;
        
        halfWidth = width / 2;
        halfHeight = height / 2;
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

    vec3f GetPointPosition( int row, int column )
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

    const BoardParams & GetParams() const
    {
        return params;
    }

private:

    int size;

    BoardParams params;

    float width;                            // width of board (along x-axis)
    float height;                           // height of board (along y-axis)

    float halfWidth;
    float halfHeight;
};

#endif
