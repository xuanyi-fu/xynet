#include <span>
#include <vector>
#include <streambuf>

namespace xynet
{

class stream_buffer : public std::streambuf
{
public:
  inline static constexpr std::size_t DELTA = 128;

  stream_buffer()
  :m_buffer(DELTA)
  {
    auto base = m_buffer.data();
    setg(base, base, base);
    setp(base, base + m_buffer.size());
  }

  // readable size
  [[nodiscard]]
  std::size_t size() const noexcept
  {
    return pptr() - gptr();
  }

  [[nodiscard]]
  auto data() const noexcept
  {
    return std::as_bytes(std::span{gptr(), pptr()});
  }

  [[nodiscard]]
  auto prepare(std::size_t n)
  {
    reserve(n);
    return std::as_writable_bytes(std::span{pptr(), n});
  }

  void commit(std::size_t n) noexcept
  {
    n = std::min<std::size_t>(n, epptr() - pptr());
    pbump(n);
    setg(eback(), gptr(), pptr());
  }

  void consume(std::size_t n)
  {
    if(egptr() < pptr())
    {
      setg(m_buffer.data(), gptr(), pptr());
    }

    if(pptr() < gptr() + n)
    {
      n = pptr() - gptr();
    }

    gbump(static_cast<int>(n));
  }

protected:

  void reserve(std::size_t n)
  {
    if(n == 0 || epptr() - pptr() >= n)
    {
      return;
    }

    auto gptr_pos = gptr() - m_buffer.data();
    auto pptr_pos = pptr() - m_buffer.data();
    auto epptr_pos = epptr() - m_buffer.data();

    // gptr is not aligned with the beginning of the data
    // we have to shift all the data back
    if (gptr_pos > 0)
    {
      pptr_pos -= gptr_pos;
      std::memmove(m_buffer.data(),
        m_buffer.data() + gptr_pos,
        pptr_pos);
    }

    if(n > epptr_pos - pptr_pos)
    {
      epptr_pos = pptr_pos + n;
      m_buffer.resize(epptr_pos);
    }

    setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + pptr_pos);
    setp(m_buffer.data() + pptr_pos, m_buffer.data() + epptr_pos);
  }

  int_type overflow(int_type c) override
  {
    if(traits_type::eq_int_type(c, traits_type::eof()))
    {
      return traits_type::not_eof(c);
    }

    if(pptr() == epptr())
    {
      reserve(DELTA);
    }

    *pptr() = traits_type::to_char_type(c);
    pbump(1);
    return c;
  }

  int_type underflow() override
  {
    if (gptr() < pptr())
    {
      setg(m_buffer.data(), gptr(), pptr());
      return traits_type::to_int_type(*gptr());
    }
    else
    {
      return traits_type::eof();
    }
  }

private:
  std::vector<char> m_buffer;
};

}
