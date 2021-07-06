//
// Created by igor on 24/06/2021.
//

#include <neutrino/engine/application.hh>
#include <neutrino/demoscene/demoscene.hh>
#include <cmath>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 360

using namespace neutrino;

struct plasma : public demoscene::scene
{
public:
    plasma()
            : demoscene::scene(SCREEN_WIDTH, SCREEN_HEIGHT)
    {
        /*create sin lookup table */
        for (int i = 0; i < 512; i++)
        {
            auto rad =  ((float)i * 0.703125) * 0.0174532; /* 360 / 512 * degree to rad, 360 degrees spread over 512 values to be able to use AND 512-1 instead of using modulo 360*/
            aSin[i] = sin(rad) * 1024; /*using fixed point math with 1024 as base*/
        }
    }

    void effect(demoscene::vga& vga) override
    {
        uint16_t i,j;
        uint8_t index;
        int x;
        uint8_t* image = vga.surface().data();

        tpos4 = pos4;
        tpos3 = pos3;

        for (i = 0; i < SCREEN_HEIGHT; ++i)
        {
            tpos1 = pos1 + 5;
            tpos2 = pos2 + 3;

            tpos3 &= 511;
            tpos4 &= 511;

            for (j = 0; j < SCREEN_WIDTH; ++j)
            {
                tpos1 &= 511;
                tpos2 &= 511;

                x = aSin[tpos1] + aSin[tpos2] + aSin[tpos3] + aSin[tpos4]; /*actual plasma calculation*/

                index = 128 + (x >> 4); /*fixed point multiplication but optimized so basically it says (x * (64 * 1024) / (1024 * 1024)), x is already multiplied by 1024*/

                *image++ = index;

                tpos1 += 5;
                tpos2 += 3;
            }

            tpos4 += 3;
            tpos3 += 1;
        }

        /* move plasma */

        pos1 +=9;
        pos3 +=8;
    }

    void init(demoscene::vga& vga) override
    {
        auto& colors = vga.palette();
        /* create palette */
        for (int i = 0; i < 64; ++i)
        {
            colors[i].r = i << 2;
            colors[i].g = 255 - ((i << 2) + 1);
            colors[i+64].r = 255;
            colors[i+64].g = (i << 2) + 1;
            colors[i+128].r = 255 - ((i << 2) + 1);
            colors[i+128].g = 255 - ((i << 2) + 1);
            colors[i+192].g = (i << 2) + 1;
        }
    }


private:
    uint16_t pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0, tpos1, tpos2, tpos3, tpos4;
    int aSin[512];
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    engine::application app(nullptr);
    plasma window;
    window.show();

    app.run(30);

    return 0;
}