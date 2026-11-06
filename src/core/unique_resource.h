#pragma once

#include <utility>

namespace vp {

// Move-only RAII owner for handle-like values.
//
// Traits must provide:
//   using value_type = ...;
//   static value_type invalid() noexcept;
//   static bool isValid(value_type) noexcept;
//   static void close(value_type) noexcept;
template <typename Traits>
class UniqueResource {
public:
    using value_type = typename Traits::value_type;

    UniqueResource() noexcept = default;
    explicit UniqueResource(value_type value) noexcept : m_value(value) {}
    ~UniqueResource() { reset(); }

    UniqueResource(UniqueResource&& other) noexcept : m_value(other.release()) {}
    UniqueResource& operator=(UniqueResource&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator=(const UniqueResource&) = delete;

    value_type get() const noexcept { return m_value; }
    explicit operator bool() const noexcept { return Traits::isValid(m_value); }

    // Gives up ownership without closing.
    value_type release() noexcept { return std::exchange(m_value, Traits::invalid()); }

    // Closes the current value (if valid) and takes ownership of the new one.
    void reset(value_type value = Traits::invalid()) noexcept {
        const value_type old = std::exchange(m_value, value);
        if (Traits::isValid(old)) {
            Traits::close(old);
        }
    }

private:
    value_type m_value = Traits::invalid();
};

} // namespace vp
