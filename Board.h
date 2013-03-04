#ifndef BOARD_H
#define BOARD_H

/*
    Go board.

    We model the go board as an axis aligned rectangular prism.

    Since the floor is the plane y = 0, the top surface of the board
    is the plane y = thickness.

    Go board dimensions:

        Board width                 424.2mm
        Board length                454.5mm
        Board thickness             151.5mm
        Line spacing width-wise     22mm
        Line spacing length-wise    23.7mm
        Line thickness              1mm
        Star point marker diameter  4mm

    https://en.wikipedia.org/wiki/Go_equipment#Board
*/

struct BoardParams
{
    BoardParams()
    {
        gridWidth = 2.2f;
        gridHeight = 2.37f;
        thickness = 0.5f;
    }

    float gridWidth;
    float gridHeight;
    float thickness;
};

class Board
{
public:

    Board( int size, const BoardParams & params = BoardParams() )
    {
        this->size = size;
        this->params = params;

        this->width = size * params.gridWidth;
        this->height = size * params.gridHeight;
        
        halfWidth = width / 2;
        halfHeight = height / 2;
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

private:

    int size;

    BoardParams params;

    float width;                            // width of board (along x-axis)
    float height;                           // depth of board (along z-axis)

    float halfWidth;
    float halfHeight;
};

#endif
