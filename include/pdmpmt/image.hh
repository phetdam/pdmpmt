/**
 * @file image.hh
 * @author Derek Huang
 * @brief C++ header for RGB[A] image manipulation
 * @copyright MIT License
 */

#ifndef PDMPMT_IMAGE_HH_
#define PDMPMT_IMAGE_HH_

#include <cctype>
#include <cstddef>
#include <cstring>
#include <istream>
#include <ostream>
#include <utility>

namespace pdmpmt {

/**
 * RGBA pixel type.
 *
 * This is backed by 4 characters that represents the RGBA channels in network
 * byte order supporting at least 8-bit color on all platforms.
 *
 * @note We define the channel values in terms of bytes instead of fixed-width
 *  types as some exotic platforms have addressable units != 8 bits and the C++
 *  standard guarantees type-accessibility as `char` or `unsigned char`.
 */
class pixel {
public:
  using byte = unsigned char;

  /**
   * RGBA pixel view type.
   *
   * This holds a pointer to the backing 4-char buffer to allow modification.
   */
  class view {
  public:
    /**
     * Ctor.
     *
     * @param rgba RGBA buffer
     */
    view(byte* rgba) noexcept : rgba_{rgba} {}

    /**
     * Deleted copy ctor.
     *
     * We don't allow copying the `view` to prevent initializing an `auto`
     * variable with the result of `image::operator()`, which returns a
     * `pixel::view` pointing to the `image` data instead of a `pixel`.
     */
    view(const view&) = delete;

    /**
     * Assign the `pixel` values to the underlying 4-char buffer.
     *
     * This enables assignment to the `image::operator()` return value.
     *
     * @param px RGBA pixel
     */
    view& operator=(pixel px) noexcept
    {
      r() = px.r();
      g() = px.g();
      b() = px.b();
      a() = px.a();
      return *this;
    }

    /**
     * Return a pointer to the 4-char buffer.
     */
    auto data() const noexcept { return rgba_; }

    /**
     * Return a reference to the red value.
     */
    byte& r() const noexcept { return rgba_[0]; }

    /**
     * Return a reference to the green value.
     */
    byte& g() const noexcept { return rgba_[1]; }

    /**
     * Return a reference to the blue value.
     */
    byte& b() const noexcept { return rgba_[2]; }

    /**
     * Return a reference to the alpha value.
     */
    byte& a() const noexcept { return rgba_[3]; }

    /**
     * Test for equality against a pixel view.
     *
     * The equality is done by checking the actual RGBA values.
     */
    bool operator==(const view& v) const noexcept
    {
      return equal(rgba_, v.rgba_);
    }

    /**
     * Test for inequality against a pixel view by negating `operator==`.
     */
    bool operator!=(const view& v) const noexcept
    {
      return !(*this == v);
    }

    /**
     * Test for equality against a pixel.
     *
     * The equality is done by checking the actual RGBA values.
     */
    bool operator==(const pixel& v) const noexcept
    {
      return equal(rgba_, v.rgba_);
    }

    /**
     * Test for inequality against a pixel by negating `operator==`.
     */
    bool operator!=(const pixel& v) const noexcept
    {
      return !(*this == v);
    }

  private:
    byte* rgba_;
  };

  /**
   * Default ctor.
   *
   * Constructs a black pixel with alpha set to `0xFF`.
   */
  pixel() noexcept : pixel{0, 0, 0} {}

  /**
   * Ctor.
   *
   * Construct from 4 contiguous RGBA byte values.
   *
   * @param rgba RGBA values
   */
  pixel(const byte* rgba) noexcept : pixel{rgba[0], rgba[1], rgba[2], rgba[3]}
  {}

  /**
   * Ctor.
   *
   * Constructs from individual RGBA bytes.
   *
   * @param rv Red value
   * @param gv Green value
   * @param bv Blue value
   * @param av Alpha value
   */
  pixel(byte rv, byte gv, byte bv, byte av = 0xFF) noexcept
  {
    r() = rv;
    g() = gv;
    b() = bv;
    a() = av;
  }

  /**
   * Ctor.
   *
   * Constructs from a pixel view to enable copy semantics.
   *
   * @param rgba RGBA pixel view
   */
  pixel(const view& rgba) noexcept
  {
    r() = rgba.r();
    g() = rgba.g();
    b() = rgba.b();
    a() = rgba.a();
  }

  /**
   * Returns a red pixel.
   *
   * @param a Alpha value
   */
  static pixel red(byte a = 0xFF) noexcept
  {
    return {0xFF, 0, 0, a};
  }

  /**
   * Returns a green pixel.
   *
   * @param a Alpha value
   */
  static pixel green(byte a = 0xFF) noexcept
  {
    return {0, 0xFF, 0, a};
  }

  /**
   * Returns a blue pixel.
   *
   * @param a Alpha value
   */
  static pixel blue(byte a = 0xFF) noexcept
  {
    return {0, 0, 0xFF, a};
  }

  /**
   * Returns a black pixel.
   *
   * @param a Alpha value
   */
  static pixel black(byte a = 0xFF) noexcept
  {
    return {0, 0, 0, a};
  }

  /**
   * Returns a white pixel.
   *
   * @param a Alpha value
   */
  static pixel white(byte a = 0xFF) noexcept
  {
    return {0xFF, 0xFF, 0xFF, a};
  }

  /**
   * Return a pointer to the 4-char buffer.
   */
  auto data() const noexcept { return rgba_; }

  /**
   * Return a reference to the red value.
   */
  byte& r() noexcept { return rgba_[0]; }

  /**
   * Return a const reference to the red value.
   */
  auto& r() const noexcept { return rgba_[0]; }

  /**
   * Return a reference to the green value.
   */
  byte& g() noexcept { return rgba_[1]; }

  /**
   * Return a const reference to the green value.
   */
  auto& g() const noexcept { return rgba_[1]; }

  /**
   * Return a reference to the blue value.
   */
  byte& b() noexcept { return rgba_[2]; }

  /**
   * Return a const reference to the blue value.
   */
  auto& b() const noexcept { return rgba_[2]; }

  /**
   * Return a reference to the alpha value.
   */
  byte& a() noexcept { return rgba_[3]; }

  /**
   * Return a const reference to the alpha value.
   */
  auto& a() const noexcept { return rgba_[3]; }

  /**
   * Test for equality against a pixel.
   *
   * The equality is done by checking the actual RGBA values.
   */
  bool operator==(const pixel& v) const noexcept
  {
    return equal(rgba_, v.rgba_);
  }

  /**
   * Test for inequality against a pixel by negating `operator==`.
   */
  bool operator!=(const pixel& v) const noexcept
  {
    return !(*this == v);
  }

  /**
   * Test for equality against a pixel view.
   *
   * The equality is done by checking the actual RGBA values.
   */
  bool operator==(const view& v) const noexcept
  {
    return equal(rgba_, v.data());
  }

  /**
   * Test for inequality against a pixel view by negating `operator==`.
   */
  bool operator!=(const view& v) const noexcept
  {
    return !(*this == v);
  }

private:
  byte rgba_[4];

  /**
   * Check two RGBA byte buffers for equality.
   *
   * @param a RGBA buffer
   * @param b RGBA buffer
   */
  static bool equal(const byte* a, const byte* b) noexcept
  {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
  }
};

namespace detail {

/**
 * Stream an RGBA buffer to the output stream in hexadecimal.
 *
 * All RGBA channels will be represented using 2 uppercase hex characters.
 *
 * @param out Output stream
 * @param rgba RGBA buffer
 */
inline auto& write_rgba_hex(std::ostream& out, const pixel::byte* rgba)
{
  // hex characters
  constexpr const char hex_chars[] = "0123456789ABCDEF";
  // format bytes to buffer
  char buf[8];
  for (auto i = 0u; i < 4u; i++) {
    buf[2u * i] = hex_chars[(rgba[i] >> 4) & 0xF];
    buf[2u * i + 1u] = hex_chars[rgba[i] & 0xF];
  }
  // write to stream
  return out.write(buf, sizeof buf);
}

}  // namespace detail

/**
 * Write a `pixel` to the output stream in hexadecimal.
 *
 * All RGBA channels will be represented using 2 uppercase hex characters.
 *
 * @param out Output stream
 * @param px Pixel to write
 */
inline auto& operator<<(std::ostream& out, const pixel& px)
{
  return detail::write_rgba_hex(out, px.data());
}

/**
 * Write a `pixel::view` to the output stream in hexdecimal.
 *
 * All RGBA channels will be represented using 2 uppercase hex characters.
 *
 * @note Although the `pixel::view` can be converted into a `pixel` we provide
 *  this extra overload to support template argument deduction.
 *
 * @param out Output stream
 * @param pv Pixel view to write
 */
inline auto& operator<<(std::ostream& out, const pixel::view& pv)
{
  return detail::write_rgba_hex(out, pv.data());
}

/**
 * Tag type to indicate an `image` should be handled in PPM format.
 */
struct ppm_tag {};

/**
 * Tag global to indicate an `image` should be handled in PPM format.
 */
inline constexpr ppm_tag ppm{};

namespace detail {

/**
 * Skip whitespace and comments in a PPM file input stream.
 *
 * We first trim any whitespace and as long as comment, which is `#` up to but
 * not including the trailing CR or LF character, is successfully skipped, we
 * repeat the whitespace trimming step until we have no comments.
 *
 * @param in Input stream
 */
inline void skip_spaces_comments(std::istream& in, ppm_tag)
{
  // helper for skipping a comment. returns true if comment was skipped
  auto skip_comment = [&in]
  {
    // no comment
    if (in.peek() != '#')
      return false;
    // skip '#'
    in.get();
    // skip contents until CR or LF
    while (in.peek() != '\r' && in.peek() != '\n')
      in.get();
    return true;
  };
  // note: always skip whitespace at least once
  do {
    while (std::isspace(in.peek()))
      in.get();
  }
  while (skip_comment());
  // done. after last round of whitespace skipping the next char is not '#'
}

}  // namespace detail

/**
 * RGBA image type.
 *
 * The image's backing buffer consists of character data to enable easy
 * manipulation of the underlying addressable units.
 */
class image {
public:
  using byte = unsigned char;

  /**
   * View type over the image bytes.
   */
  class bytes_view {
  public:
    /**
     * Ctor.
     *
     * @param data `image` data buffer
     * @param size `image` data buffer size
     */
    bytes_view(byte* data, std::size_t size) noexcept
      : data_{data}, size_{size}
    {}

    /**
     * Return the image data buffer.
     */
    auto data() const noexcept { return data_; }

    /**
     * Return the image data buffer size in bytes.
     */
    auto size() const noexcept { return size_; }

    /**
     * Return a reference to byte `i` in the image.
     */
    auto& operator[](std::size_t i) const noexcept
    {
      return data_[i];
    }

    /**
     * Return an iterator to the first byte in the image buffer.
     */
    auto begin() const noexcept
    {
      return data_;
    }

    /**
     * Return an interator to one past the last byte in the image buffer.
     */
    auto end() const noexcept
    {
      return data_ + size_;
    }

  private:
    byte* data_;
    std::size_t size_;
  };

  /**
   * Ctor.
   *
   * Creates an image of size zero with `nullptr` pixel buffer.
   */
  image() noexcept : width_{}, height_{} {}

  /**
   * Ctor.
   *
   * Allocates memory for the pixel buffer without initializing values unless
   * `width` or `height` are zero, in which the buffer is left as `nullptr`.
   *
   * @param width Number of pixels per row
   * @param height Number of pixels per column
   */
  image(std::size_t width, std::size_t height) : width_{width}, height_{height}
  {
    if (size())
      alloc();
  }

  /**
   * Ctor.
   *
   * Read bytes from an input stream opened in binary mode in the PPM format
   * described in https://netpbm.sourceforge.net/doc/ppm.html. The max color
   * value is required to be `0xFF` and so each RGB triplet is 3 bytes.
   *
   * All alpha values are set to `0xFF` as PPM supports only RGB, not RGBA.
   *
   * @param in Input stream
   */
  image(std::istream& in, ppm_tag)
  {
    // check the PPM magic
    {
      char magic[2];
      in.read(magic, sizeof magic);
      if (magic[0] != 'P' && magic[1] != '6')
        throw std::runtime_error{"invalid PPM file magic"};
    }
    // skip whitespace + comments
    detail::skip_spaces_comments(in, ppm);
    // get image width and height
    // note: we allow these to be zero
    in >> width_;
    detail::skip_spaces_comments(in, ppm);
    in >> height_;
    detail::skip_spaces_comments(in, ppm);
    // get max color value (must be 0xFF)
    {
      auto max_value = 0u;
      in >> max_value;
      if (max_value != 0xFF)
        throw std::runtime_error{"max color value must be 255 (0xFF)"};
    }
    // skip spaces + comments for the last time before reading RGB triples
    detail::skip_spaces_comments(in, ppm);
    // allocate image buffer + read RGB triples
    alloc();
    for (auto i = 0u; i < size(); i++) {
      in.read(reinterpret_cast<char*>(&(*this)(i).r()), 3);
      // note: alpha set to max value since PPM is an RGB format
      (*this)(i).a() = 0xFF;
    }
  }

  /**
   * Copy ctor.
   */
  image(const image& other)
  {
    from(other);
  }

  /**
   * Copy assignment operator.
   */
  image& operator=(const image& other)
  {
    destroy();
    from(other);
    return *this;
  }

  /**
   * Move ctor.
   */
  image(image&& other) noexcept
  {
    from(std::move(other));
  }

  /**
   * Move assignment operator.
   */
  image& operator=(image&& other) noexcept
  {
    destroy();
    from(std::move(other));
    return *this;
  }

  /**
   * Dtor.
   *
   * If the image has `nullptr` buffer then nothing is done.
   */
  ~image()
  {
    destroy();
  }

  /**
   * Return the number of pixels in row.
   */
  auto width() const noexcept { return width_; }

  /**
   * Return the number of pixels in a column.
   */
  auto height() const noexcept { return height_; }

  /**
   * Return the number of pixels in the image.
   */
  std::size_t size() const noexcept
  {
    return width_ * height_;
  }

  /**
   * Return a view over the image's data buffer.
   */
  bytes_view bytes() const noexcept
  {
    return {buf_, 4u * size()};
  }

  /**
   * Indicate if the image is nonempty and has allocated storage.
   */
  operator bool() const noexcept
  {
    return !!buf_;
  }

  /**
   * Return a reference to the specified pixel.
   *
   * @param i Pixel index
   */
  pixel::view operator()(std::size_t i) noexcept
  {
    return &buf_[4u * i];
  }

  /**
   * Return the specified pixel.
   *
   * @param i Pixel index
   */
  pixel operator()(std::size_t i) const noexcept
  {
    return &buf_[4u * i];
  }

  /**
   * Return a reference to the specified pixel.
   *
   * @param i Row index
   * @param j Column index
   */
  pixel::view operator()(std::size_t i, std::size_t j) noexcept
  {
    return &buf_[4u * (i * width_ + j)];
  }

  /**
   * Return the specified pixel.
   *
   * @param i Row index
   * @param j Column index
   */
  pixel operator()(std::size_t i, std::size_t j) const noexcept
  {
    return &buf_[4u * (i * width_ + j)];
  }

private:
  byte* buf_{};         // pixel buffer
  std::size_t width_;   // pixels per row
  std::size_t height_;  // pixels per column

  /**
   * Allocates storage for the image buffer based on the value of `size()`.
   */
  void alloc()
  {
    buf_ = new byte[4u * size()];
  }

  /**
   * Deallocates storage if the image buffer is not `nullptr`.
   */
  void destroy() noexcept
  {
    if (buf_)
      delete[] buf_;
  }

  /**
   * Copy-initialize from the `image`.
   *
   * If the other image is itself not empty buffer allocation is performed and
   * the contents of the other image's buffer are copied over.
   */
  void from(const image& other)
  {
    width_ = other.width_;
    height_ = other.height_;
    if (size()) {
      alloc();
      std::memcpy(buf_, other.buf_, 4u * size());
    }
  }

  /**
   * Move-initialize from the `image`.
   *
   * On return the moved-from `image` will have size zero and `nullptr` buffer.
   */
  void from(image&& other)
  {
    buf_ = other.buf_;
    width_ = other.width_;
    height_ = other.height_;
    other.buf_ = nullptr;
    other.width_ = 0u;
    other.height_ = 0u;
  }
};

/**
 * Stream wrapper for disambiguating PPM image `operator>` overloads.
 */
class ppm_istream_wrapper {
public:
  /**
   * Ctor.
   *
   * @param in Input stream
   */
  ppm_istream_wrapper(std::istream& in) noexcept : in_{&in} {}

  /**
   * Return the input stream.
   */
  auto& in() const noexcept { return *in_; }

private:
  std::istream* in_;  // trivially-copyable
};

/**
 * Return a `ppm_istream_wrapper` to enable reading an `image` in PPM format.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @param in Input stream
 */
inline ppm_istream_wrapper operator>>(std::istream& in, const ppm_tag&)
{
  return in;
}

/**
 * Read an `image` in PPM format from an input stream opened in binary mode.
 *
 * See https://netpbm.sourceforge.net/doc/ppm.html for the PPM format.
 *
 * @note The maximum color value for the PPM is hardcoded to `0xFF`.
 *
 * @par
 *
 * @note PPM does not support alpha so all alpha values will be set fo `0xFF`.
 *
 * @param in Input stream
 * @param res Image to [re-]initialize
 */
inline auto& operator>>(const ppm_istream_wrapper& in, image& res)
{
  image img{in.in(), ppm};
  res = std::move(img);
  return in.in();
}

/**
 * Stream wrapper for disambiguating PPM image `operator<<` overloads.
 */
class ppm_ostream_wrapper {
public:
  /**
   * Ctor.
   *
   * @param out Output stream
   */
  ppm_ostream_wrapper(std::ostream& out) noexcept : out_{&out} {}

  /**
   * Return the output stream.
   */
  auto& out() const noexcept { return *out_; }

private:
  std::ostream* out_;  // trivially-copyable
};

/**
 * Return a `ppm_ostream_wrapper` to enable writing an `image` in PPM format.
 *
 * @note All arguments are taken by reference to avoid MSVC emitting C4866.
 *
 * @param out Output stream
 */
inline ppm_ostream_wrapper operator<<(std::ostream& out, const ppm_tag&)
{
  return out;
}

/**
 * Write an `image` in PPM format to an output stream opened in binary mode.
 *
 * See https://netpbm.sourceforge.net/doc/ppm.html for the PPM format.
 *
 * @note The maximum color value for the PPM is hardcoded to `0xFF`.
 *
 * @par
 *
 * @note PPM does not support alpha values so only the RGB values are written.
 *
 * @param out Output stream
 * @param img Image to write
 */
inline auto& operator<<(const ppm_ostream_wrapper& out, const image& img)
{
  auto& os = out.out();
  // PPM magic
  os << "P6\n";
  // image width + height
  os << img.width() << ' ' << img.height() << '\n';
  // max color value
  os << 0xFF << '\n';
  // write pixel values + return
  for (auto i = 0u; i < img.size(); i++)
    os << img(i).r() << img(i).g() << img(i).b();
  return os;
}

}  // namespace pdmpmt

#endif  // PDMPMT_IMAGE_HH_
