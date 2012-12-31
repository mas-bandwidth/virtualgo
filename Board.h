#ifndef BOARD_H
#define BOARD_H

/*
    Go board.

    We model the go board as an axis aligned rectangular prism.
    The top surface of the go board is along the y = 0 plane for simplicity.
    Collision with the top surface is the common case, however we also need
    to consider collisions with the side planes, the top edges and the top
    corners.
*/

class Board
{
public:

    Board( float width, float height, float thickness )
    {
        this->width = width;
        this->height = height;
        this->thickness = thickness;
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

    float GetThickness() const
    {
        return thickness;
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

    float width;                            // width of board (along x-axis)
    float height;                           // depth of board (along z-axis)
    float thickness;                        // thickness of board (along y-axis)

    float halfWidth;
    float halfHeight;
};

#endif
