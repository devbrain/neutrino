#include <doctest/doctest.h>

#include <neutrino/video/sprite/atlas_packer.hh>

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

namespace {
    using neutrino::rect;
    using neutrino::pack_atlas;

    // Half-open rectangle intersection (touching edges do NOT overlap).
    bool overlaps(const rect& a, const rect& b) noexcept {
        return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
                 a.y + a.h <= b.y || b.y + b.h <= a.y);
    }

    // Grow a rect by m on every side.
    rect inflate(const rect& r, int m) noexcept {
        return rect{r.x - m, r.y - m, r.w + 2 * m, r.h + 2 * m};
    }

    std::vector<rect> make_tiles(int n) {
        std::vector<rect> v;
        v.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            v.push_back(rect{0, 0, 12 + (i % 5) * 6, 10 + (i % 3) * 8});
        }
        return v;
    }

    bool color_eq(const sdlpp::color& a, const sdlpp::color& b) noexcept {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }

    sdlpp::surface solid(int w, int h, const sdlpp::color& c,
                         sdlpp::pixel_format_enum fmt = sdlpp::pixel_format_enum::RGBA8888) {
        auto s = sdlpp::surface::create_rgb(w, h, fmt);
        REQUIRE(s.has_value());
        REQUIRE(s->fill(c).has_value());
        return std::move(*s);
    }

    // Build an atlas whose surface holds one fw x fh frame per colour, laid out
    // in a horizontal strip, with a frame rect registered for each.
    neutrino::cpu_texture_atlas make_atlas(const std::vector<sdlpp::color>& frame_colors,
                                           int fw, int fh) {
        auto s = sdlpp::surface::create_rgb(fw * static_cast<int>(frame_colors.size()), fh,
                                            sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(s.has_value());
        std::vector<neutrino::cpu_texture_atlas_frame> frames;
        for (std::size_t i = 0; i < frame_colors.size(); ++i) {
            const rect fr{static_cast<int>(i) * fw, 0, fw, fh};
            REQUIRE(s->fill_rect(fr, frame_colors[i]).has_value());
            frames.emplace_back(fr);
        }
        return neutrino::cpu_texture_atlas(std::move(*s), std::move(frames));
    }
}

TEST_SUITE("neutrino::atlas_packer") {
    TEST_CASE("every input is placed exactly once at its original size") {
        const auto tiles = make_tiles(50);
        const auto [placements, pages] = pack_atlas(tiles, 256, 256, 1);

        CHECK(placements.size() == tiles.size());
        std::set<std::uint32_t> seen;
        for (const auto& [page, input_index, bounds] : placements) {
            REQUIRE(input_index < tiles.size());
            CHECK(page < pages.size());
            CHECK(bounds.w == tiles[input_index].w);   // reported un-inflated
            CHECK(bounds.h == tiles[input_index].h);
            seen.insert(input_index);
        }
        CHECK(seen.size() == tiles.size());                 // no duplicates, no gaps
    }

    TEST_CASE("no two tiles overlap and the gutter belt separates them") {
        const int margin = 2;
        const auto tiles = make_tiles(60);
        const auto res = pack_atlas(tiles, 128, 128, margin);

        for (std::size_t i = 0; i < res.placements.size(); ++i) {
            for (std::size_t j = i + 1; j < res.placements.size(); ++j) {
                const auto& a = res.placements[i];
                const auto& b = res.placements[j];
                if (a.page != b.page) {
                    continue;                               // different textures, can't bleed
                }
                CHECK_FALSE(overlaps(a.bounds, b.bounds));  // tiles disjoint
                // Inflating each tile by the margin recovers its packed slot;
                // slots must still be disjoint, i.e. a full gutter separates them.
                CHECK_FALSE(overlaps(inflate(a.bounds, margin), inflate(b.bounds, margin)));
            }
        }
    }

    TEST_CASE("tiles lie inside their page and the page is the trimmed extent") {
        constexpr int margin = 2;
        const auto tiles = make_tiles(40);
        const auto [placements, pages] = pack_atlas(tiles, 256, 256, margin);

        std::vector<int> exp_w(pages.size(), 0);
        std::vector<int> exp_h(pages.size(), 0);
        for (const auto& p : placements) {
            const auto& b = p.bounds;
            CHECK(b.x >= margin);                           // left/top gutter present
            CHECK(b.y >= margin);
            CHECK(b.x + b.w <= pages[p.page].width);    // inside the page
            CHECK(b.y + b.h <= pages[p.page].height);
            // Page extent = max tile extent + its right/bottom gutter belt.
            exp_w[p.page] = std::max(exp_w[p.page], b.x + b.w + margin);
            exp_h[p.page] = std::max(exp_h[p.page], b.y + b.h + margin);
        }
        for (std::size_t p = 0; p < pages.size(); ++p) {
            CHECK(pages[p].width == exp_w[p]);          // tight, not the cap
            CHECK(pages[p].height == exp_h[p]);
        }
    }

    TEST_CASE("packing spills onto multiple pages") {
        // 20 tiles of 60x60 into a 128x128 page: at most 2x2 = 4 per page -> >= 5 pages.
        const std::vector<rect> tiles(20, rect{0, 0, 60, 60});
        const auto [placements, pages] = pack_atlas(tiles, 128, 128, 0);

        CHECK(pages.size() > 1);
        CHECK(placements.size() == tiles.size());
        std::set<std::uint32_t> seen;
        for (const auto& p : placements) {
            seen.insert(p.input_index);
        }
        CHECK(seen.size() == tiles.size());                 // nothing dropped in the spill
    }

    TEST_CASE("placements are ordered by (page, input_index)") {
        const auto tiles = make_tiles(40);
        const auto [placements, pages] = pack_atlas(tiles, 96, 96, 1);      // small page -> several pages

        for (std::size_t i = 1; i < placements.size(); ++i) {
            const auto& prev = placements[i - 1];
            const auto& cur = placements[i];
            const bool ordered = prev.page < cur.page ||
                                 (prev.page == cur.page && prev.input_index < cur.input_index);
            CHECK(ordered);
        }
    }

    TEST_CASE("a rect larger than the page cap throws") {
        CHECK_THROWS((void)pack_atlas(std::vector<rect>{rect{0, 0, 300, 10}}, 256, 256, 0));
        // Fits at margin 0, but the gutter pushes it over the cap.
        CHECK_THROWS((void)pack_atlas(std::vector<rect>{rect{0, 0, 256, 10}}, 256, 256, 1));
    }

    TEST_CASE("invalid arguments throw") {
        const std::vector<rect> tiles{rect{0, 0, 8, 8}};
        CHECK_THROWS((void)pack_atlas(tiles, 0, 256, 0));         // non-positive cap
        CHECK_THROWS((void)pack_atlas(tiles, 256, -1, 0));
        CHECK_THROWS((void)pack_atlas(tiles, 256, 256, -1));      // negative margin
    }

    TEST_CASE("non-positive tile sizes throw") {
        CHECK_THROWS((void)pack_atlas(std::vector<rect>{rect{0, 0, 0, 8}}, 256, 256, 0));   // zero width
        CHECK_THROWS((void)pack_atlas(std::vector<rect>{rect{0, 0, 8, 0}}, 256, 256, 0));   // zero height
        CHECK_THROWS((void)pack_atlas(std::vector<rect>{rect{0, 0, -4, 8}}, 256, 256, 0));  // negative
    }

    TEST_CASE("empty input yields an empty result") {
        const auto res = pack_atlas(std::vector<rect>{}, 256, 256, 1);
        CHECK(res.placements.empty());
        CHECK(res.pages.empty());
    }

    TEST_CASE("packing is deterministic for identical input") {
        const auto tiles = make_tiles(35);
        const auto a = pack_atlas(tiles, 128, 128, 1);
        const auto b = pack_atlas(tiles, 128, 128, 1);

        REQUIRE(a.placements.size() == b.placements.size());
        for (std::size_t i = 0; i < a.placements.size(); ++i) {
            CHECK(a.placements[i].page == b.placements[i].page);
            CHECK(a.placements[i].input_index == b.placements[i].input_index);
            CHECK(a.placements[i].bounds.x == b.placements[i].bounds.x);
            CHECK(a.placements[i].bounds.y == b.placements[i].bounds.y);
        }
        REQUIRE(a.pages.size() == b.pages.size());
        for (std::size_t i = 0; i < a.pages.size(); ++i) {
            CHECK(a.pages[i].width == b.pages[i].width);
            CHECK(a.pages[i].height == b.pages[i].height);
        }
    }
}

TEST_SUITE("neutrino::pack_surfaces") {
    using neutrino::pack_surfaces;
    using sdlpp::pixel_format_enum;

    TEST_CASE("each source composites at its slot in the target format") {
        std::vector<sdlpp::surface> src;
        src.push_back(solid(8, 8, sdlpp::color{255, 0, 0, 255}));
        src.push_back(solid(6, 10, sdlpp::color{0, 255, 0, 255}));
        src.push_back(solid(12, 4, sdlpp::color{0, 0, 255, 255}));
        const std::vector<sdlpp::color> want{
            {255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

        const auto res = pack_surfaces(std::span<const sdlpp::surface>{src},
                                       pixel_format_enum::RGBA8888, 256, 256, 1);

        REQUIRE(res.placements.size() == src.size());
        for (const auto& p : res.placements) {
            const auto& surf = res.pages[p.page].surface();
            const auto px = surf.get_pixel(p.bounds.x + p.bounds.w / 2, p.bounds.y + p.bounds.h / 2);
            REQUIRE(px.has_value());
            CHECK(color_eq(*px, want[p.input_index]));   // right tile, right place
        }
    }

    TEST_CASE("the gutter carries the tile's edge colour (extrude)") {
        std::vector<sdlpp::surface> src;
        src.push_back(solid(8, 8, sdlpp::color{10, 20, 30, 255}));
        const auto res = pack_surfaces(std::span<const sdlpp::surface>{src},
                                       pixel_format_enum::RGBA8888, 64, 64, 2);

        REQUIRE(res.placements.size() == 1);
        const auto& b = res.placements[0].bounds;
        const auto& surf = res.pages[0].surface();
        const auto left = surf.get_pixel(b.x - 1, b.y + b.h / 2);   // one pixel into the belt
        REQUIRE(left.has_value());
        CHECK(color_eq(*left, sdlpp::color{10, 20, 30, 255}));
    }

    TEST_CASE("blit is a straight copy: translucent RGB is not premultiplied") {
        std::vector<sdlpp::surface> src;
        src.push_back(solid(4, 4, sdlpp::color{200, 100, 50, 128}));
        const auto res = pack_surfaces(std::span<const sdlpp::surface>{src},
                                       pixel_format_enum::RGBA8888, 64, 64, 0);

        const auto& b = res.placements[0].bounds;
        const auto px = res.pages[0].surface().get_pixel(b.x + 1, b.y + 1);
        REQUIRE(px.has_value());
        CHECK(color_eq(*px, sdlpp::color{200, 100, 50, 128}));       // exact, not RGB*0.5
    }

    TEST_CASE("sources of differing formats convert into the atlas format") {
        std::vector<sdlpp::surface> src;
        src.push_back(solid(6, 6, sdlpp::color{255, 0, 0, 255}, pixel_format_enum::RGB24));
        src.push_back(solid(6, 6, sdlpp::color{0, 0, 255, 255}, pixel_format_enum::RGBA8888));
        const std::vector<sdlpp::color> want{{255, 0, 0, 255}, {0, 0, 255, 255}};

        const auto res = pack_surfaces(std::span<const sdlpp::surface>{src},
                                       pixel_format_enum::RGBA8888, 64, 64, 1);

        REQUIRE(res.placements.size() == 2);
        for (const auto& p : res.placements) {
            const auto px = res.pages[p.page].surface().get_pixel(p.bounds.x + 2, p.bounds.y + 2);
            REQUIRE(px.has_value());
            CHECK(color_eq(*px, want[p.input_index]));
        }
    }

    TEST_CASE("empty input yields no pages") {
        const auto res = pack_surfaces(std::span<const sdlpp::surface>{},
                                       pixel_format_enum::RGBA8888, 64, 64, 1);
        CHECK(res.pages.empty());
        CHECK(res.placements.empty());
    }

    TEST_CASE("mask generation is opt-in and reflects source alpha") {
        auto s = sdlpp::surface::create_rgb(2, 1, pixel_format_enum::RGBA8888);
        REQUIRE(s.has_value());
        REQUIRE(s->put_pixel(0, 0, sdlpp::color{255, 0, 0, 255}).has_value()); // solid
        REQUIRE(s->put_pixel(1, 0, sdlpp::color{0, 255, 0, 0}).has_value());   // transparent
        std::vector<sdlpp::surface> src;
        src.push_back(std::move(*s));

        // No policy -> no mask.
        const auto plain = pack_surfaces(std::span<const sdlpp::surface>{src},
                                         pixel_format_enum::RGBA8888, 64, 64, 0);
        REQUIRE(plain.pages[0].frame_count() == 1);
        CHECK_FALSE(plain.pages[0].frame(0).mask.has_value());

        // Alpha policy (default options) -> mask from source alpha.
        const auto masked = pack_surfaces(std::span<const sdlpp::surface>{src},
                                          pixel_format_enum::RGBA8888, 64, 64, 0,
                                          neutrino::cpu_texture_atlas_mask_options{});
        const auto& f = masked.pages[0].frame(0);
        REQUIRE(f.mask.has_value());
        CHECK(f.mask->get(0, 0) == true);
        CHECK_FALSE(f.mask->get(1, 0));
    }

    TEST_CASE("convenience overload derives format and cap from the first image") {
        std::vector<sdlpp::surface> src;
        src.push_back(solid(8, 8, sdlpp::color{255, 0, 0, 255}));
        src.push_back(solid(6, 6, sdlpp::color{0, 255, 0, 255}));
        const std::vector<sdlpp::color> want{{255, 0, 0, 255}, {0, 255, 0, 255}};

        const auto res = pack_surfaces(std::span<const sdlpp::surface>{src}, 1); // no format/w/h

        REQUIRE(res.placements.size() == 2);
        for (const auto& p : res.placements) {
            const auto px = res.pages[p.page].surface().get_pixel(p.bounds.x + 2, p.bounds.y + 2);
            REQUIRE(px.has_value());
            CHECK(color_eq(*px, want[p.input_index]));
        }
    }

    TEST_CASE("convenience overload on empty input yields no pages") {
        const auto res = pack_surfaces(std::span<const sdlpp::surface>{}, 1);
        CHECK(res.pages.empty());
        CHECK(res.placements.empty());
    }
}

TEST_SUITE("neutrino::pack_atlases") {
    using neutrino::pack_atlases;
    using neutrino::cpu_texture_atlas;
    using sdlpp::pixel_format_enum;

    TEST_CASE("frames from several atlases repack into one texture") {
        std::vector<cpu_texture_atlas> atlases;
        atlases.push_back(make_atlas({{255, 0, 0, 255}, {0, 255, 0, 255}}, 8, 8)); // 2 frames
        atlases.push_back(make_atlas({{0, 0, 255, 255}}, 6, 10));                  // 1 frame
        // atlas-major frame order: atlas0.f0, atlas0.f1, atlas1.f0
        const std::vector<sdlpp::color> want{{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

        const auto res = pack_atlases(std::span<const cpu_texture_atlas>{atlases},
                                      pixel_format_enum::RGBA8888, 256, 256, 1);

        REQUIRE(res.placements.size() == 3);   // total frames across all atlases
        std::size_t total_out_frames = 0;
        for (const auto& page : res.pages) total_out_frames += page.frame_count();
        CHECK(total_out_frames == 3);
        for (const auto& p : res.placements) {
            const auto& surf = res.pages[p.page].surface();
            const auto px = surf.get_pixel(p.bounds.x + p.bounds.w / 2, p.bounds.y + p.bounds.h / 2);
            REQUIRE(px.has_value());
            CHECK(color_eq(*px, want[p.input_index]));   // each frame's pixels relocated intact
        }
    }

    TEST_CASE("frame masks are carried onto the repacked frames") {
        auto s = sdlpp::surface::create_rgb(8, 8, pixel_format_enum::RGBA8888);
        REQUIRE(s.has_value());
        REQUIRE(s->fill(sdlpp::color{10, 20, 30, 255}).has_value());
        neutrino::bitmask m(8, 8);
        m.set(0, 0, true);
        m.set(1, 1, false);
        std::vector<neutrino::cpu_texture_atlas_frame> frames;
        frames.emplace_back(rect{0, 0, 8, 8}, m);
        std::vector<cpu_texture_atlas> atlases;
        atlases.emplace_back(std::move(*s), std::move(frames));

        const auto res = pack_atlases(std::span<const cpu_texture_atlas>{atlases},
                                      pixel_format_enum::RGBA8888, 64, 64, 0);

        REQUIRE(res.pages.size() == 1);
        REQUIRE(res.pages[0].frame_count() == 1);
        const auto& of = res.pages[0].frame(0);
        REQUIRE(of.mask.has_value());
        CHECK(of.mask->get(0, 0) == true);
        CHECK(of.mask->get(1, 1) == false);
    }

    TEST_CASE("empty atlas set yields no pages") {
        const auto res = pack_atlases(std::span<const cpu_texture_atlas>{},
                                      pixel_format_enum::RGBA8888, 64, 64, 1);
        CHECK(res.pages.empty());
        CHECK(res.placements.empty());
    }

    TEST_CASE("convenience overload derives format and cap from the first atlas") {
        std::vector<cpu_texture_atlas> atlases;
        atlases.push_back(make_atlas({{255, 0, 0, 255}, {0, 0, 255, 255}}, 8, 8));
        const std::vector<sdlpp::color> want{{255, 0, 0, 255}, {0, 0, 255, 255}};

        const auto res = pack_atlases(std::span<const cpu_texture_atlas>{atlases}, 1); // no format/w/h

        REQUIRE(res.placements.size() == 2);
        for (const auto& p : res.placements) {
            const auto& surf = res.pages[p.page].surface();
            const auto px = surf.get_pixel(p.bounds.x + p.bounds.w / 2, p.bounds.y + p.bounds.h / 2);
            REQUIRE(px.has_value());
            CHECK(color_eq(*px, want[p.input_index]));
        }
    }
}
