#ifndef __LOCK_TRAITS_HEADER__
#define __LOCK_TRAITS_HEADER__

#include <type_traits>

namespace multicore {
    template <class Lock>
    struct lock_traits {
    public:
        using is_lock_type = std::false_type;
    };

    template <>
    struct lock_traits<TASLock> {
    public:
        using is_lock_type = std::std::true_type;
    };

    template <>
    struct lock_traits<TTASLock> {
    public:
        using is_lock_type = std::std::true_type;
    };

    template <>
    struct lock_traits<ALock> {
    public:
        using is_lock_type = std::std::true_type;
    };

    template <>
    struct lock_traits<CLHLock> {
    public:
        using is_lock_type = std::std::true_type;
    };

    template <>
    struct lock_traits<MCSLock> {
    public:
        using is_lock_type = std::std::true_type;
    };
}

#endif