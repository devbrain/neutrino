//
// Created by igor on 9/14/24.
//

#ifndef CC_MAP_TILE_HH
#define CC_MAP_TILE_HH

enum TileFlags {
    TILE_SOLID = 0x01,
    TILE_SOLID_TOP = 0x02,
    TILE_DAMAGE = 0x04,
    TILE_DEATH = 0x08,
    TILE_ANIMATED = 0x10,
    TILE_RENDER_IN_FRONT = 0x20,
};

enum TileObject {
    TILE_SIMPLE,
    TILE_PLAYER_START,
    TILE_PLATFORM_H,
    TILE_PLATFORM_V,
    TILE_DOOR
};

class Tile {
    public:
        Tile()
            : valid_(false), sprite_(-1), sprite_count_(0), flags_(0), object_(TILE_SIMPLE) {
        }

        Tile(int sprite, int sprite_count, int flags)
            : valid_(true), sprite_(sprite), sprite_count_(sprite_count), flags_(flags), object_(TILE_SIMPLE) {
        }

        Tile(int sprite, int sprite_count, int flags, TileObject object)
            : valid_(true), sprite_(sprite), sprite_count_(sprite_count), flags_(flags), object_(object) {
        }

        [[nodiscard]] bool valid() const { return valid_; }

        [[nodiscard]] int get_sprite() const { return sprite_; }
        [[nodiscard]] int get_sprite_count() const { return sprite_count_; }

        [[nodiscard]] bool is_solid() const { return (flags_ & 0x01) != 0; }
        [[nodiscard]] bool is_solid_top() const { return (flags_ & 0x02) != 0; }
        [[nodiscard]] bool is_damage() const { return (flags_ & 0x04) != 0; }
        [[nodiscard]] bool is_death() const { return (flags_ & 0x08) != 0; }
        [[nodiscard]] bool is_animated() const { return (flags_ & 0x10) != 0; }
        [[nodiscard]] bool is_render_in_front() const { return (flags_ & 0x20) != 0; }

        [[nodiscard]] TileObject get_object() const { return object_; }

        static const Tile INVALID;

        int entrance_to {0};
    private:
        bool valid_;
        int sprite_;
        int sprite_count_;
        int flags_;
        TileObject object_;
};
#endif
