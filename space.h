#include <ratio>
#include <type_traits>

template<typename Rep, typename Period = std::ratio<1>>
class Space;

template<typename ToSpace, typename Rep, typename Period>
ToSpace space_cast(const Space<Rep, Period>& other) {
    using to_period = typename ToSpace::period;
    using to_rep = typename ToSpace::rep;
    using from_period = Period;

    using ratio = std::ratio_divide<from_period, to_period>;
    return ToSpace(static_cast<to_rep>(other.count() * ratio::num / ratio::den));
}

template<typename Rep, typename Period>
class Space {
public:
    using rep = Rep;
    using period = Period;

    Space() : rep_(0){}

    explicit Space(const Rep& r) : rep_(r){}

    template<typename Rep2, typename Period2>
    explicit Space(const Space<Rep2, Period2>& other) {
        rep_ = space_cast<Space<Rep, Period>>(other).count();
    }

    constexpr rep count() const {
        return rep_;
    }

    Space& operator++() {
        rep_++;
        return *this;
    }
    Space operator++(int) {
        Space temp = *this;
        rep_++;
        return temp;
    }

    Space& operator--() {
        rep_--;
        return *this;
    }
    Space operator--(int) {
        Space temp = *this;
        rep_--;
        return temp;
    }
    template<typename Rep2, typename Period2>
    Space& operator+=(const Space<Rep2, Period2>& other) {
        rep_ += space_cast<Space>(other).count();
        return *this;
    }

    template<typename Rep2, typename Period2>
    Space& operator-=(const Space<Rep2, Period2>& other) {
        rep_ -= space_cast<Space>(other).count();
        return *this;
    }

private:
    Rep rep_;
};

template<typename Rep, typename Period, typename Rep2, typename Period2>
auto operator+(const Space<Rep, Period>& x, const Space<Rep2, Period2>& y) {
    using need_rep = std::common_type_t<Rep, Rep2>;
    using need_period = std::conditional_t<std::ratio_less_v<Period, Period2>, Period, Period2>;
    using need_space = Space<need_rep, need_period>;

    auto left_count = space_cast<need_space>(x).count();
    auto right_count = space_cast<need_space>(y).count();
    return need_space(left_count + right_count);
}

template<typename Rep, typename Period, typename Rep2, typename Period2>
auto operator-(const Space<Rep, Period>& x, const Space<Rep2, Period2>& y) {
    using need_rep = std::common_type_t<Rep, Rep2>;
    using need_period = std::conditional_t<std::ratio_less_v<Period, Period2>, Period, Period2>;
    using need_space = Space<need_rep, need_period>;

    auto left_count = space_cast<need_space>(x).count();
    auto right_count = space_cast<need_space>(y).count();
    return need_space(left_count - right_count);
}



using kilo = std::ratio<1024L, 1>;
using mega = std::ratio<1024L * 1024, 1>;
using giga = std::ratio<1024L * 1024 * 1024, 1>;
using tera = std::ratio<1024L * 1024 * 1024 * 1024, 1>;

using bytes = Space<uint64_t>;
using kilobytes = Space<uint64_t, kilo>;
using megabytes = Space<uint64_t, mega>;
using gigabytes = Space<uint64_t, giga>;
using terabytes = Space<uint64_t, tera>;
