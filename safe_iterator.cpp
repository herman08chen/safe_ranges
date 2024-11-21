#include <atomic>
#include <ranges>
#include "control_block.cpp"

namespace safe_ranges {
    template<class R>
    class iterator_t {
    public:
        typedef std::ranges::iterator_t<R> iterator_type;
        explicit iterator_t(iterator_type it, R& Range, control_block* Control) : it{it}, Range{Range}, Control{Control}, Gen{Control->gen_count} {
            Control->ref_count++;
        }
        iterator_t(const iterator_t& other) : iterator_t {other.it, other.Range, other.Control} {}

        ~iterator_t() {
            Control->release();
        }

        explicit operator iterator_type() { return it; }

        iterator_t& operator++() {
            if(it == std::ranges::end(Range)) [[unlikely]] throw std::out_of_range("incremented past end");
            ++it;
            return *this;
        }
        iterator_t operator++(int) {
            iterator_t old = *this;
            operator++();
            return old;
        }
        iterator_t& operator--() {
            if(it == std::ranges::begin(Range)) [[unlikely]] throw std::out_of_range("decremented begin");
            --it;
            return *this;
        }
        iterator_t operator--(int) {
            iterator_t old = *this;
            operator--();
            return old;
        }

        template<class U = void>
        requires std::ranges::random_access_range<R>
        friend iterator_t operator+(iterator_t lhs, const int& i) {
            const auto current_index = std::distance(std::ranges::begin(lhs.Range), lhs.it);
            if( current_index + i >= std::ranges::size(lhs.Range) | current_index + i < 0) [[unlikely]] throw std::out_of_range("out of range");
            return iterator_t{lhs.it + i, lhs.Range};
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        friend iterator_t operator-(iterator_t lhs, const int& i) {
            return operator+(lhs, -i);
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        iterator_t& operator+=(const int& i) {
            *this = *this + i;
            return *this;
        }
        template<class U = void>
        requires std::ranges::random_access_range<R>
        iterator_t& operator-=(const int& i) {
            return operator+=(-i);
        }
        bool operator==(const iterator_t& rhs) const { return it == rhs.it; };
        bool operator!=(const iterator_t& rhs) const { return it != rhs.it; };

        auto& operator*() {
            if(Gen != Control->gen_count) [[unlikely]] throw std::out_of_range("iterator invalidated");
            if(it == std::ranges::end(Range)) [[unlikely]] throw std::out_of_range("dereferenced past end");
            return *it;
        }
        auto& operator[](std::size_t pos) {
            return *(*this + pos);
        }

    private:
        iterator_type it;
        R& Range;
        control_block* Control;
        std::size_t Gen;
    };

    template<class R>
    class sentinel_t {
    public:
        typedef std::ranges::sentinel_t<R> iterator_type;
        explicit sentinel_t(iterator_type it, R& Range) : it {it}, Range{Range} {}
        sentinel_t(const sentinel_t& other) = default;
        ~sentinel_t() = default;

        friend bool operator==(iterator_t<R> lhs, sentinel_t<R> rhs) {
            return lhs.it == rhs.it && &lhs.Range == &rhs.Range;
        }
        friend bool operator!=(iterator_t<R> lhs, sentinel_t<R> rhs) {
            return !operator==(lhs, rhs);
        }
    private:
        iterator_type it;
        R& Range;
    };
}
