//
// Created by igor on 08/06/2020.
//

#ifndef NEUTRINO_SDL_RENDER_HH
#define NEUTRINO_SDL_RENDER_HH

#include <hal/sdl/surface.hh>
#include <hal/sdl/texture.hh>
#include <neutrino/utils/array_view.hh>

namespace neutrino::sdl {
  class renderer : public object<SDL_Renderer> {
    public:
      enum class flags : uint32_t {
          NONE = 0,
          SOFTWARE = SDL_RENDERER_SOFTWARE,
          ACCELERATED = SDL_RENDERER_ACCELERATED,
          PRESENTVSYNC = SDL_RENDERER_PRESENTVSYNC,
          TARGETTEXTURE = SDL_RENDERER_TARGETTEXTURE
      };

      enum class flip : uint32_t {
          NONE = SDL_FLIP_NONE,
          HORIZONTAL = SDL_FLIP_HORIZONTAL,
          VERTICAL = SDL_FLIP_VERTICAL
      };

    public:
      renderer () = default;

      template <typename ... Args,
          typename std::enable_if<(std::is_same_v<flags, Args> && ...), int>::type = 0>
      explicit renderer (const object<SDL_Window>& w, Args...flags);
      explicit renderer (const object<SDL_Surface>& s);

      explicit renderer (object<SDL_Renderer>&& other);
      renderer& operator = (object<SDL_Renderer>&& other) noexcept;

      [[nodiscard]] blend_mode blend () const;
      void blend (blend_mode bm);

      [[nodiscard]] color active_color () const;
      void active_color (const color& c);

      [[nodiscard]] rect clip () const;
      void clip (const rect& area);
      void disable_clippping ();
      [[nodiscard]] bool clipping_enabled () const;

      [[nodiscard]] bool integer_scaling () const;
      void integer_scaling (bool enabled);

      [[nodiscard]] std::pair<unsigned, unsigned> logical_size () const;
      void logical_size (unsigned x, unsigned y);

      [[nodiscard]] std::pair<float, float> scaling () const;
      void scaling (float x, float y);

      [[nodiscard]] rect viewport () const;
      void viewport (const rect& area);
      void disable_viewport ();

      /**
       * This is a very slow operation, and should not be used frequently.
       *  pitch specifies the number of bytes between rows in the destination pixels data.
       *  This allows you to write to a subrectangle or have padded rows in the destination.
       *  Generally, pitch should equal the number of pixels per row in the pixels data times the number
       *  of bytes per pixel, but it might contain additional padding
       *  (for example, 24bit RGB Windows Bitmap data pads all rows to multiples of 4 bytes).
       */
      void read_pixels (const pixel_format& fmt, void* dst, std::size_t pitch) const;
      void read_pixels (const rect& area, const pixel_format& fmt, void* dst, std::size_t pitch) const;
      pixel_format get_pixel_format() const;

      [[nodiscard]] std::optional<texture> target () const;
      void target (texture& t);
      void restore_default_target ();

      [[nodiscard]] std::pair<unsigned, unsigned> output_size () const;

      void clear ();

      void copy (const texture& t, flip flip_ = flip::NONE);
      void copy (const texture& t, const rect& srcrect, flip flip_ = flip::NONE);
      void copy (const texture& t, const rect& srcrect, const rect& dstrect, flip flip_ = flip::NONE);
      void copy (const texture& t, const rect& srcrect, const rect& dstrect, double angle, flip flip_ = flip::NONE);
      void copy (const texture& t, const rect& srcrect, const rect& dstrect, double angle, const point& pt, flip flip_);

      void draw (int x1, int y1, int x2, int y2);
      void draw (const point& p1, const point& p2);
      void draw_connected_lines (const utils::array_view1d<point>& vertices);
      void draw (int x, int y);
      void draw (const point& p);
      void draw (const utils::array_view1d<point>& points);
      void draw (const rect& rec);
      void draw (const utils::array_view1d<rect>& rec);
      void draw_filled (const rect& rec);
      void draw_filled (const utils::array_view1d<rect>& rec);

      void present () noexcept;

  };
}

// ===========================================================================================================
// Implementation
// ===========================================================================================================

namespace neutrino::sdl {
  template <typename ... Args,
      typename std::enable_if<(std::is_same_v<renderer::flags, Args> && ...), int>::type>
  renderer::renderer (const object<SDL_Window>& w, Args...flags)
      : object<SDL_Renderer> (SAFE_SDL_CALL(SDL_CreateRenderer,
                                            const_cast<SDL_Window*>(w.handle ()),
                                            -1,
                                            (static_cast<std::uint32_t>(flags) | ... | 0u)
                              ), true) {

  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  renderer::renderer (const object<SDL_Surface>& s)
      : object<SDL_Renderer> (SAFE_SDL_CALL(SDL_CreateSoftwareRenderer, const_cast<SDL_Surface*>(s.handle ())), true) {

  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  renderer::renderer (object<SDL_Renderer>&& other)
      : object<SDL_Renderer> (std::move (other)) {

  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  renderer& renderer::operator = (object<SDL_Renderer>&& other) noexcept {
    object<SDL_Renderer>::operator = (std::move (other));
    return *this;
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  blend_mode renderer::blend () const {
    SDL_BlendMode m;
    SAFE_SDL_CALL(SDL_GetRenderDrawBlendMode, const_handle (), &m);
    return static_cast<blend_mode>(m);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::blend (blend_mode bm) {
    SAFE_SDL_CALL(SDL_SetRenderDrawBlendMode, handle (), static_cast<SDL_BlendMode>(bm));
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  color renderer::active_color () const {
    color c;
    SAFE_SDL_CALL(SDL_GetRenderDrawColor, const_handle (), &c.r, &c.g, &c.b, &c.a);
    return c;
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::active_color (const color& c) {
    SAFE_SDL_CALL(SDL_SetRenderDrawColor, handle (), c.r, c.g, c.b, c.a);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  rect renderer::clip () const {
    rect r;
    SDL_RenderGetClipRect (const_handle (), &r);
    return r;
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::clip (const rect& area) {
    SAFE_SDL_CALL(SDL_RenderSetClipRect, handle (), &area);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::disable_clippping () {
    SAFE_SDL_CALL(SDL_RenderSetClipRect, handle (), nullptr);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  bool renderer::clipping_enabled () const {
    return SDL_RenderIsClipEnabled (const_handle ()) == SDL_TRUE;
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  bool renderer::integer_scaling () const {
    return SDL_TRUE == SDL_RenderGetIntegerScale (const_handle ());
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::integer_scaling (bool enabled) {
    SAFE_SDL_CALL(SDL_RenderSetIntegerScale, handle (), enabled ? SDL_TRUE : SDL_FALSE);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  std::pair<unsigned, unsigned> renderer::logical_size () const {
    int w, h;
    SDL_RenderGetLogicalSize (const_handle (), &w, &h);
    return {static_cast<unsigned >(w), static_cast<unsigned >(h)};
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::logical_size (unsigned x, unsigned y) {
    SAFE_SDL_CALL(SDL_RenderSetLogicalSize, handle (), static_cast<int>(x), static_cast<int>(y));
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  std::pair<float, float> renderer::scaling () const {
    float x, y;
    SDL_RenderGetScale (const_handle (), &x, &y);
    return {x, y};
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::scaling (float x, float y) {
    SAFE_SDL_CALL(SDL_RenderSetScale, handle (), x, y);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  rect renderer::viewport () const {
    rect r;
    SDL_RenderGetViewport (const_handle (), &r);
    return r;
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::viewport (const rect& area) {
    SAFE_SDL_CALL(SDL_RenderSetViewport, handle (), &area);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::disable_viewport () {
    SAFE_SDL_CALL(SDL_RenderSetViewport, handle (), nullptr);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::read_pixels (const pixel_format& fmt, void* dst, std::size_t pitch) const {
    SAFE_SDL_CALL(SDL_RenderReadPixels,
                  const_handle (),
                  nullptr,
                  fmt.value (),
                  dst,
                  static_cast<int>(pitch)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  pixel_format renderer::get_pixel_format() const {
    auto window = SDL_RenderGetWindow(const_handle());
    if (window) {
      return pixel_format (SDL_GetWindowPixelFormat (window));
    } else {
      uint32_t format;
      auto t = target();
      if (t) {
        SAFE_SDL_CALL(SDL_QueryTexture, t->const_handle (), &format, nullptr, nullptr, nullptr);
        return pixel_format(format);
      }
      RAISE_EX("Can not determine pixel format");
    }
  }
  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::read_pixels (const rect& area, const pixel_format& fmt, void* dst, std::size_t pitch) const {
    SAFE_SDL_CALL(SDL_RenderReadPixels,
                  const_handle (),
                  &area,
                  fmt.value (),
                  dst,
                  static_cast<int>(pitch)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  std::optional<texture> renderer::target () const {
    SDL_Texture* t = SDL_GetRenderTarget (const_handle ());
    if (t) {
      return texture (object<SDL_Texture> (t, false));
    }
    return std::nullopt;
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::target (texture& t) {
    SAFE_SDL_CALL(SDL_SetRenderTarget, handle (), t.handle ());
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::restore_default_target () {
    SAFE_SDL_CALL(SDL_SetRenderTarget, handle (), nullptr);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  std::pair<unsigned, unsigned> renderer::output_size () const {
    int w, h;
    SAFE_SDL_CALL(SDL_GetRendererOutputSize, const_handle (), &w, &h);
    return {static_cast<unsigned>(w), static_cast<unsigned>(h)};
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::clear () {
    SAFE_SDL_CALL(SDL_RenderClear, handle ());
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::copy (const texture& t, flip flip_) {
    SAFE_SDL_CALL(SDL_RenderCopyEx,
                  handle (),
                  t.const_handle (),
                  nullptr,
                  nullptr,
                  0.0,
                  nullptr,
                  static_cast<SDL_RendererFlip>(flip_)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::copy (const texture& t, const rect& srcrect, flip flip_) {
    SAFE_SDL_CALL(SDL_RenderCopyEx,
                  handle (),
                  t.const_handle (),
                  &srcrect,
                  nullptr,
                  0.0,
                  nullptr,
                  static_cast<SDL_RendererFlip>(flip_)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::copy (const texture& t, const rect& srcrect, const rect& dstrect, flip flip_) {
    SAFE_SDL_CALL(SDL_RenderCopyEx,
                  handle (),
                  t.const_handle (),
                  &srcrect,
                  &dstrect,
                  0.0,
                  nullptr,
                  static_cast<SDL_RendererFlip>(flip_)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::copy (const texture& t, const rect& srcrect, const rect& dstrect, double angle, flip flip_) {
    SAFE_SDL_CALL(SDL_RenderCopyEx,
                  handle (),
                  t.const_handle (),
                  &srcrect,
                  &dstrect,
                  angle,
                  nullptr,
                  static_cast<SDL_RendererFlip>(flip_)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void
  renderer::copy (const texture& t, const rect& srcrect, const rect& dstrect, double angle, const point& pt,
                  flip flip_) {
    SAFE_SDL_CALL(SDL_RenderCopyEx,
                  handle (),
                  t.const_handle (),
                  &srcrect,
                  &dstrect,
                  angle,
                  &pt,
                  static_cast<SDL_RendererFlip>(flip_)
    );
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (int x1, int y1, int x2, int y2) {
    SAFE_SDL_CALL(SDL_RenderDrawLine, handle (), x1, y1, x2, y2);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (const point& p1, const point& p2) {
    draw (p1.x, p1.y, p2.x, p2.y);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw_connected_lines (const utils::array_view1d<point>& vertices) {
#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4267)
#endif
    SAFE_SDL_CALL(SDL_RenderDrawLines, handle (), vertices.data (), vertices.size ());
#if defined(_MSC_VER)
#pragma warning ( pop )
#endif
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (int x, int y) {
    SAFE_SDL_CALL(SDL_RenderDrawPoint, handle (), x, y);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (const point& p) {
    draw (p.x, p.y);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (const utils::array_view1d<point>& points) {
    SAFE_SDL_CALL(SDL_RenderDrawPoints, handle (), points.data (), points.size ());
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (const rect& rec) {
    SAFE_SDL_CALL(SDL_RenderDrawRect, handle (), &rec);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw (const utils::array_view1d<rect>& rec) {
#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4267)
#endif
    SAFE_SDL_CALL(SDL_RenderDrawRects, handle (), rec.data (), rec.size ());
#if defined(_MSC_VER)
#pragma warning ( pop )
#endif
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw_filled (const rect& rec) {
    SAFE_SDL_CALL(SDL_RenderFillRect, handle (), &rec);
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::draw_filled (const utils::array_view1d<rect>& rec) {
    SAFE_SDL_CALL(SDL_RenderFillRects, handle (), rec.data (), rec.size ());
  }

  // ----------------------------------------------------------------------------------------------------------------
  inline
  void renderer::present () noexcept {
    SDL_RenderPresent (const_handle ());
  }
}

#endif
