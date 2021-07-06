//
// Created by igor on 25/06/2021.
//

#include <neutrino/engine/application.hh>
#include <neutrino/demoscene/demoscene.hh>
#include <glm/ext/vector_int2.hpp>
#include <cmath>

#define BLOB_RADIUS 44
#define BLOB_DRADIUS (BLOB_RADIUS * 2)
#define BLOB_SRADIUS (BLOB_RADIUS * BLOB_RADIUS)
#define NUMBER_OF_BLOBS 40
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 360

using namespace neutrino;

struct blob_window : public demoscene::scene
{
public:
    blob_window()
    : demoscene::scene(SCREEN_WIDTH, SCREEN_HEIGHT)
    {
        int i, j;
        uint32_t distance_squared;
        float fraction;

        /* create blob */
        for (i = -BLOB_RADIUS; i < BLOB_RADIUS; ++i)
        {
            for (j = -BLOB_RADIUS; j < BLOB_RADIUS; ++j)
            {
                distance_squared = i * i + j * j;
                if (distance_squared <= BLOB_SRADIUS)
                {
                    /* compute density */
                    fraction = (float)distance_squared / (float)BLOB_SRADIUS;
                    blob[i + BLOB_RADIUS][j + BLOB_RADIUS] = pow((1.0 - (fraction * fraction)), 4.0) * 255.0;
                }
                else
                    blob[i + BLOB_RADIUS][j + BLOB_RADIUS] = 0;
            }
        }

        for (i = 0; i < NUMBER_OF_BLOBS; i++)
        {
            init_blob(blobs + i);
        }
    }

    void effect(demoscene::vga& vga) override
    {
        uint32_t start;
        int i, j;
        uint8_t k;
        vga.cls();
        uint8_t* image = vga.surface().data();

        for (i = 0; i < NUMBER_OF_BLOBS; i++)
        {
            blobs[i][0] += -2 + (int)(5.0 * (rand()/(RAND_MAX+1.0)));
            blobs[i][1] += -2 + (int)(5.0 * (rand()/(RAND_MAX+1.0)));
        }

        for (k = 0; k < NUMBER_OF_BLOBS; ++k)
        {
            if (blobs[k][0] > 0 && blobs[k][0] < SCREEN_WIDTH - BLOB_DRADIUS &&
                blobs[k][1] > 0 && blobs[k][1] < SCREEN_HEIGHT - BLOB_DRADIUS)
            {
                start = blobs[k][0] + blobs[k][1] * SCREEN_WIDTH;
                for (i = 0; i < BLOB_DRADIUS; ++i)
                {
                    for (j = 0; j < BLOB_DRADIUS; ++j)
                    {
                        if (image[start + j] + blob[i][j] > 255)
                            image[start + j] = 255;
                        else
                            image[start + j] += blob[i][j];
                    }
                    start += SCREEN_WIDTH;
                }
            }
            else
            {
                init_blob(blobs + k);
            }
        }

    }

    void init(demoscene::vga& vga) override
    {
        auto& colors = vga.palette();
        for (int i = 0; i < 256; ++i)
        {
            colors[i].r = i;
            colors[i].g = i;
            colors[i].b = i;
        }
    }

private:
    using vec2 = glm::ivec2;

    static void init_blob(vec2* blob)
    {
        (*blob)[0] =  (SCREEN_WIDTH >> 1) - BLOB_RADIUS;
        (*blob)[1] =  (SCREEN_HEIGHT >> 1) - BLOB_RADIUS;
    }

    uint8_t blob[BLOB_DRADIUS][BLOB_DRADIUS];
    vec2 blobs[NUMBER_OF_BLOBS];
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    engine::application app(nullptr);
    blob_window w;
    w.show();
    app.run(30);
    return 0;
}