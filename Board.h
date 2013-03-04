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

    void SetThickness( float thickness )
    {
        this->thickness = thickness;
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
