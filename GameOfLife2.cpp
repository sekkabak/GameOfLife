#include <iostream>

#include <Windows.h>
#include "olcConsoleGameEngine.h"

#include<windows.h>
#include<iostream>
#include <cmath>

using namespace std;

#define PI 3.14

/*
class Demo : public olcConsoleGameEngine {
public:
    Demo() {
    }

protected:
    virtual bool OnUserCreate() {
        return true;
    }
    virtual bool OnUserUpdate(float fElapsedTime) {
        for (int x = 0; x < m_nScreenWidth; x++) {
            for (int y = 0; y < m_nScreenHeight; y++) {
                Draw(x, y, L'#', rand() % 16);
            }
        }

        return true;
    }
};
*/
int main1()
{
    /*
    Demo game;
    game.ConstructConsole(160, 100, 8, 8);
    game.Start();
    */


    //Get a console handle
    HWND myconsole = GetConsoleWindow();
    //Get a handle to device context
    HDC mydc = GetDC(myconsole);

    int pixel = 0;

    //Choose any color
    COLORREF COLOR = RGB(255, 255, 255);

    //Draw pixels
    for (double i = 0; i < PI * 4; i += 0.05)
    {
        SetPixel(mydc, pixel, (int)(50 + 25 * cos(i)), COLOR);
        pixel += 1;
    }

    ReleaseDC(myconsole, mydc);
    cin.ignore();
    return 0;
};