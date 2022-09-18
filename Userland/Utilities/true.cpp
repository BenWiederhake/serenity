/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Platform.h>
#include <AK/String.h>
#include <LibMain/Main.h>

// #define MAGIC_VARIANT_OLD
// → 0.25user 0.01system 0:00.26elapsed 99%CPU (0avgtext+0avgdata 64356maxresident)k
//   0inputs+552outputs (0major+7620minor)pagefaults 0swaps
// #define MAGIC_VARIANT_NEW
// → 0.26user 0.00system 0:00.27elapsed 99%CPU (0avgtext+0avgdata 63260maxresident)k
//   0inputs+568outputs (0major+10478minor)pagefaults 0swaps
// #define MAGIC_VARIANT_NONE
// → 0.07user 0.00system 0:00.08elapsed 98%CPU (0avgtext+0avgdata 43196maxresident)k
//   0inputs+56outputs (0major+4877minor)pagefaults 0swaps
#define MAGIC_VARIANT_FAKE
// → 0.12user 0.01system 0:00.13elapsed 99%CPU (0avgtext+0avgdata 49864maxresident)k
//   0inputs+176outputs (0major+6796minor)pagefaults 0swaps

#ifdef MAGIC_VARIANT_NEW
class VariantSpecial {
private:
    static constexpr u8 invalid_index = 255;

public:
    template<typename>
    static constexpr bool can_contain();

    // template<typename... NewTs>
    // Variant(Variant<NewTs...>&& old) requires((can_contain<NewTs>() && ...))
    //     : Variant(move(old).template downcast<Ts...>())
    //{
    // }

    // template<typename... NewTs>
    // Variant(Variant<NewTs...> const& old) requires((can_contain<NewTs>() && ...))
    //     : Variant(old.template downcast<Ts...>())
    //{
    // }

    VariantSpecial()
        : m_index(0) // FIXME: "0 = Empty"
    {
    }

    //    #ifdef AK_HAS_CONDITIONALLY_TRIVIAL
    //        Variant(Variant const&) requires(!(IsCopyConstructible<Ts> && ...)) = delete;
    //        Variant(Variant const&) = default;
    //
    //        Variant(Variant&&) requires(!(IsMoveConstructible<Ts> && ...)) = delete;
    //        Variant(Variant&&) = default;
    //
    //        ~Variant() requires(!(IsDestructible<Ts> && ...)) = delete;
    //        ~Variant() = default;
    //
    //        Variant& operator=(Variant const&) requires(!(IsCopyConstructible<Ts> && ...) || !(IsDestructible<Ts> && ...)) = delete;
    //        Variant& operator=(Variant const&) = default;
    //
    //        Variant& operator=(Variant&&) requires(!(IsMoveConstructible<Ts> && ...) || !(IsDestructible<Ts> && ...)) = delete;
    //        Variant& operator=(Variant&&) = default;
    //    #endif

    //        ALWAYS_INLINE Variant(Variant const& old)
    //    #ifdef AK_HAS_CONDITIONALLY_TRIVIAL
    //            requires(!(IsTriviallyCopyConstructible<Ts> && ...))
    //    #endif
    //            : Detail::MergeAndDeduplicatePacks<Detail::VariantConstructors<Ts, Variant<Ts...>>...>()
    //            , m_data {}
    //            , m_index(old.m_index)
    //        {
    //            Helper::copy_(old.m_index, old.m_data, m_data);
    //        }

    // Note: A moved-from variant emulates the state of the object it contains
    //       so if a variant containing an int is moved from, it will still contain that int
    //       and if a variant with a nontrivial move ctor is moved from, it may or may not be valid
    //       but it will still contain the "moved-from" state of the object it previously contained.
    //        ALWAYS_INLINE Variant(Variant&& old)
    //    #ifdef AK_HAS_CONDITIONALLY_TRIVIAL
    //            requires(!(IsTriviallyMoveConstructible<Ts> && ...))
    //    #endif
    //            : Detail::MergeAndDeduplicatePacks<Detail::VariantConstructors<Ts, Variant<Ts...>>...>()
    //            , m_index(old.m_index)
    //        {
    //            Helper::move_(old.m_index, old.m_data, m_data);
    //        }

    ALWAYS_INLINE ~VariantSpecial()
    //    #ifdef AK_HAS_CONDITIONALLY_TRIVIAL
    //            requires(!(IsTriviallyDestructible<Ts> && ...))
    //    #endif
    {
        // Helper::delete_(m_index, m_data);
        //  FIXME: Leak
    }

    //    ALWAYS_INLINE Variant& operator=(Variant const& other)
    //#    ifdef AK_HAS_CONDITIONALLY_TRIVIAL
    //        requires(!(IsTriviallyCopyConstructible<Ts> && ...) || !(IsTriviallyDestructible<Ts> && ...))
    //#    endif
    //    {
    //        if (this != &other) {
    //            if constexpr (!(IsTriviallyDestructible<Ts> && ...)) {
    //                Helper::delete_(m_index, m_data);
    //            }
    //            m_index = other.m_index;
    //            Helper::copy_(other.m_index, other.m_data, m_data);
    //        }
    //        return *this;
    //    }

    //    ALWAYS_INLINE Variant& operator=(Variant&& other)
    //#    ifdef AK_HAS_CONDITIONALLY_TRIVIAL
    //        requires(!(IsTriviallyMoveConstructible<Ts> && ...) || !(IsTriviallyDestructible<Ts> && ...))
    //#    endif
    //    {
    //        if (this != &other) {
    //            if constexpr (!(IsTriviallyDestructible<Ts> && ...)) {
    //                Helper::delete_(m_index, m_data);
    //            }
    //            m_index = other.m_index;
    //            Helper::move_(other.m_index, other.m_data, m_data);
    //        }
    //        return *this;
    //    }

    // using Detail::MergeAndDeduplicatePacks<Detail::VariantConstructors<Ts, Variant<Ts...>>...>::MergeAndDeduplicatePacks;

    template<typename T>
    void set(T&& t);

    // template<typename T, typename StrippedT = RemoveCVReference<T>>
    // void set(T&& t) requires(can_contain<StrippedT>() && requires { StrippedT(forward<T>(t)); })
    //{
    //     constexpr auto new_index = index_of<StrippedT>();
    //     // FIXME: Helper::delete_(m_index, m_data);
    //     new (m_data) StrippedT(forward<T>(t));
    //     m_index = new_index;
    // }

    // template<typename T, typename StrippedT = RemoveCVReference<T>>
    // void set(T&& t, Detail::VariantNoClearTag) requires(can_contain<StrippedT>() && requires { StrippedT(forward<T>(t)); })
    //{
    //     constexpr auto new_index = index_of<StrippedT>();
    //     new (m_data) StrippedT(forward<T>(t));
    //     m_index = new_index;
    // }

    template<typename T>
    T* get_pointer();

    // template<typename T>
    // T& get() requires(can_contain<T>())
    //{
    //     VERIFY(has<T>());
    //     return *bit_cast<T*>(&m_data);
    // }

    template<typename T>
    T const* get_pointer() const; // TODO: Implement

    // template<typename T>
    // const T& get() const requires(can_contain<T>())
    //{
    //     VERIFY(has<T>());
    //     return *bit_cast<const T*>(&m_data);
    // }

    // template<typename T>
    //[[nodiscard]] bool has() const requires(can_contain<T>())
    //{
    //     return index_of<T>() == m_index;
    // }

    // template<typename... Fs>
    // ALWAYS_INLINE decltype(auto) visit(Fs&&... functions)
    //{
    //     Visitor<Fs...> visitor { forward<Fs>(functions)... };
    //     return VisitHelper::visit(*this, m_index, m_data, move(visitor));
    // }

    // template<typename... Fs>
    // ALWAYS_INLINE decltype(auto) visit(Fs&&... functions) const
    //{
    //     Visitor<Fs...> visitor { forward<Fs>(functions)... };
    //     return VisitHelper::visit(*this, m_index, m_data, move(visitor));
    // }

    // template<typename... NewTs>
    // Variant<NewTs...> downcast() &&
    //{
    //     Variant<NewTs...> instance { Variant<NewTs...>::invalid_index, Detail::VariantConstructTag {} };
    //     visit([&](auto& value) {
    //         if constexpr (Variant<NewTs...>::template can_contain<RemoveCVReference<decltype(value)>>())
    //             instance.set(move(value), Detail::VariantNoClearTag {});
    //     });
    //     VERIFY(instance.m_index != instance.invalid_index);
    //     return instance;
    // }

    // template<typename... NewTs>
    // Variant<NewTs...> downcast() const&
    //{
    //     Variant<NewTs...> instance { Variant<NewTs...>::invalid_index, Detail::VariantConstructTag {} };
    //     visit([&](auto const& value) {
    //         if constexpr (Variant<NewTs...>::template can_contain<RemoveCVReference<decltype(value)>>())
    //             instance.set(value, Detail::VariantNoClearTag {});
    //     });
    //     VERIFY(instance.m_index != instance.invalid_index);
    //     return instance;
    // }

private:
    static constexpr auto data_size = 32;
    static constexpr auto data_alignment = 32;
    // using Helper = Detail::Variant<IndexType, 0, Ts...>;
    // using VisitHelper = Detail::VisitImpl<IndexType, Ts...>;

    // template<typename T_, typename U_>
    // friend struct Detail::VariantConstructors;

    // explicit Variant(IndexType index, Detail::VariantConstructTag)
    //     : Detail::MergeAndDeduplicatePacks<Detail::VariantConstructors<Ts, Variant<Ts...>>...>()
    //     , m_index(index)
    //{
    // }

    // ALWAYS_INLINE void clear_without_destruction()
    //{
    //     __builtin_memset(m_data, 0, data_size);
    //     m_index = invalid_index;
    // }

    // template<typename... Fs>
    // struct Visitor : Fs... {
    //     using Types = TypeList<Fs...>;

    //    Visitor(Fs&&... args)
    //        : Fs(forward<Fs>(args))...
    //    {
    //    }

    //    using Fs::operator()...;
    //};

    // Note: Make sure not to default-initialize!
    //       VariantConstructors::VariantConstructors(T) will set this to the correct value
    //       So default-constructing to anything will leave the first initialization with that value instead of the correct one.
    alignas(32) u8 m_data[32];
    u8 m_index;
};

template<typename T>
constexpr bool VariantSpecial::can_contain() { return false; }
template<>
constexpr bool VariantSpecial::can_contain<int>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<bool>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<String>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<char>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<unsigned int>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<Empty>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<char*>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<ErrorOr<int>>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<ErrorOr<char>>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<ErrorOr<char*>>() { return true; }
template<>
constexpr bool VariantSpecial::can_contain<ErrorOr<char**>>() { return true; }

template<>
constexpr int* VariantSpecial::get_pointer<int>() { return (m_index == 1) ? bit_cast<int*>(&m_data) : nullptr; }
template<>
constexpr bool* VariantSpecial::get_pointer<bool>() { return (m_index == 2) ? bit_cast<bool*>(&m_data) : nullptr; }
template<>
constexpr String* VariantSpecial::get_pointer<String>() { return (m_index == 3) ? bit_cast<String*>(&m_data) : nullptr; }
template<>
constexpr char* VariantSpecial::get_pointer<char>() { return (m_index == 4) ? bit_cast<char*>(&m_data) : nullptr; }
template<>
constexpr unsigned int* VariantSpecial::get_pointer<unsigned int>() { return (m_index == 5) ? bit_cast<unsigned int*>(&m_data) : nullptr; }
template<>
constexpr Empty* VariantSpecial::get_pointer<Empty>() { return (m_index == 0) ? bit_cast<Empty*>(&m_data) : nullptr; }
template<>
constexpr char** VariantSpecial::get_pointer<char*>() { return (m_index == 6) ? bit_cast<char**>(&m_data) : nullptr; }
template<>
constexpr ErrorOr<int>* VariantSpecial::get_pointer<ErrorOr<int>>() { return (m_index == 7) ? bit_cast<ErrorOr<int>*>(&m_data) : nullptr; }
template<>
constexpr ErrorOr<char>* VariantSpecial::get_pointer<ErrorOr<char>>() { return (m_index == 8) ? bit_cast<ErrorOr<char>*>(&m_data) : nullptr; }
template<>
constexpr ErrorOr<char*>* VariantSpecial::get_pointer<ErrorOr<char*>>() { return (m_index == 9) ? bit_cast<ErrorOr<char*>*>(&m_data) : nullptr; }
template<>
constexpr ErrorOr<char**>* VariantSpecial::get_pointer<ErrorOr<char**>>() { return (m_index == 10) ? bit_cast<ErrorOr<char**>*>(&m_data) : nullptr; }

template<>
void VariantSpecial::set<int>(int&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) int(forward<int>(value));
    m_index = 1;
}
template<>
void VariantSpecial::set<bool>(bool&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) bool(forward<bool>(value));
    m_index = 2;
}
template<>
void VariantSpecial::set<String>(String&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) String(forward<String>(value));
    m_index = 3;
}
template<>
void VariantSpecial::set<char>(char&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) char(forward<char>(value));
    m_index = 4;
}
template<>
void VariantSpecial::set<unsigned int>(unsigned int&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) unsigned int(forward<unsigned int>(value));
    m_index = 5;
}
template<>
void VariantSpecial::set<Empty>(Empty&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) Empty(forward<Empty>(value));
    m_index = 0;
}
template<>
void VariantSpecial::set<char*>(char*&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) char*(forward<char*>(value));
    m_index = 6;
}
template<>
void VariantSpecial::set<ErrorOr<int>>(ErrorOr<int>&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) ErrorOr<int>(forward<ErrorOr<int>>(value));
    m_index = 7;
}
template<>
void VariantSpecial::set<ErrorOr<char>>(ErrorOr<char>&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) ErrorOr<char>(forward<ErrorOr<char>>(value));
    m_index = 8;
}
template<>
void VariantSpecial::set<ErrorOr<char*>>(ErrorOr<char*>&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) ErrorOr<char*>(forward<ErrorOr<char*>>(value));
    m_index = 9;
}
template<>
void VariantSpecial::set<ErrorOr<char**>>(ErrorOr<char**>&& value)
{
    // FIXME: Helper::delete_(m_index, m_data);
    new (m_data) ErrorOr<char**>(forward<ErrorOr<char**>>(value));
    m_index = 10;
}

Variant<int, bool, String, char, unsigned int, Empty, char*, ErrorOr<int>, ErrorOr<char>, ErrorOr<char*>, ErrorOr<char**>> variant;

#endif

ErrorOr<int> serenity_main(Main::Arguments args)
{
#ifdef MAGIC_VARIANT_NONE
    (void)args;
    return 0;
#endif
#ifdef MAGIC_VARIANT_OLD
    if (args.strings.size() >= 2 && args.strings[1] == "weirdmagic"sv) {
        Variant<int, bool, String, char, unsigned int, Empty, char*, ErrorOr<int>, ErrorOr<char>, ErrorOr<char*>, ErrorOr<char**>> variant;
        switch (args.argc) {
        case 2:
            variant.set((int)42);
            break;
        case 3:
            variant.set((bool)true);
            break;
        case 4:
            variant.set((String) "asdf");
            break;
        case 5:
            variant.set((char)'x');
            break;
        case 6:
            variant.set((unsigned int)42);
            break;
        case 7:
            variant.set(const_cast<char*>("forty-two"));
            break;
        case 8:
            variant.set((ErrorOr<int>)42);
            break;
        case 9:
            variant.set((ErrorOr<char>)42);
            break;
        case 10:
            variant.set((ErrorOr<char*>)args.argv[0]);
            break;
        case 11:
            variant.set((ErrorOr<char**>)args.argv);
            break;
        default:
            /* no-op */
            break;
        }
        dbgln("int ptr: {:p}", variant.get_pointer<int>());
        dbgln("bool ptr: {:p}", variant.get_pointer<bool>());
        dbgln("String ptr: {:p}", variant.get_pointer<String>());
        dbgln("char ptr: {:p}", variant.get_pointer<char>());
        dbgln("unsigned int ptr: {:p}", variant.get_pointer<unsigned int>());
        dbgln("Empty ptr: {:p}", variant.get_pointer<Empty>());
        dbgln("char* ptr: {:p}", variant.get_pointer<char*>());
        dbgln("E int ptr: {:p}", variant.get_pointer<ErrorOr<int>>());
        dbgln("E char ptr: {:p}", variant.get_pointer<ErrorOr<char>>());
        dbgln("E char* ptr: {:p}", variant.get_pointer<ErrorOr<char*>>());
        dbgln("E char** ptr: {:p}", variant.get_pointer<ErrorOr<char**>>());
    }
    return 0;
#endif
#ifdef MAGIC_VARIANT_NEW
    if (args.strings.size() >= 2 && args.strings[1] == "weirdmagic"sv) {
        VariantSpecial variant;
        switch (args.argc) {
        case 2:
            variant.set<int>(42);
            break;
        case 3:
            variant.set<bool>(true);
            break;
        case 4:
            variant.set((String) "asdf");
            break;
        case 5:
            variant.set((char)'x');
            break;
        case 6:
            variant.set((unsigned int)42);
            break;
        case 7:
            variant.set(const_cast<char*>("forty-two"));
            break;
        case 8:
            variant.set((ErrorOr<int>)42);
            break;
        case 9:
            variant.set((ErrorOr<char>)42);
            break;
        case 10:
            variant.set((ErrorOr<char*>)args.argv[0]);
            break;
        case 11:
            variant.set((ErrorOr<char**>)args.argv);
            break;
        default:
            /* no-op */
            break;
        }
        dbgln("(NEW!) int ptr: {:p}", variant.get_pointer<int>());
        dbgln("(NEW!) bool ptr: {:p}", variant.get_pointer<bool>());
        dbgln("(NEW!) String ptr: {:p}", variant.get_pointer<String>());
        dbgln("(NEW!) char ptr: {:p}", variant.get_pointer<char>());
        dbgln("(NEW!) unsigned int ptr: {:p}", variant.get_pointer<unsigned int>());
        dbgln("(NEW!) Empty ptr: {:p}", variant.get_pointer<Empty>());
        dbgln("(NEW!) char* ptr: {:p}", variant.get_pointer<char*>());
        dbgln("(NEW!) E int ptr: {:p}", variant.get_pointer<ErrorOr<int>>());
        dbgln("(NEW!) E char ptr: {:p}", variant.get_pointer<ErrorOr<char>>());
        dbgln("(NEW!) E char* ptr: {:p}", variant.get_pointer<ErrorOr<char*>>());
        dbgln("(NEW!) E char** ptr: {:p}", variant.get_pointer<ErrorOr<char**>>());
    }
    return 0;
#endif
#ifdef MAGIC_VARIANT_FAKE
    if (args.strings.size() >= 2 && args.strings[1] == "weirdmagic"sv) {
        int value = 42;
        switch (args.argc) {
        case 2:
            value = 1337;
            break;
        case 3:
            value = 1186568939;
            break;
        case 4:
            value = 67348714;
            break;
        case 5:
            value = 215815582;
            break;
        case 6:
            value = 1769168898;
            break;
        case 7:
            value = 2003018508;
            break;
        case 8:
            value = 1665004825;
            break;
        case 9:
            value = 2137986064;
            break;
        case 10:
            value = 138135437;
            break;
        case 11:
            value = 1774614015;
            break;
        default:
            /* no-op */
            break;
        }
        dbgln("(FAKE!) int ptr: {:p}", &value);
        dbgln("(FAKE!) bool ptr: {:p}", &value);
        dbgln("(FAKE!) String ptr: {:p}", &value);
        dbgln("(FAKE!) char ptr: {:p}", &value);
        dbgln("(FAKE!) unsigned int ptr: {:p}", &value);
        dbgln("(FAKE!) Empty ptr: {:p}", &value);
        dbgln("(FAKE!) char* ptr: {:p}", &value);
        dbgln("(FAKE!) E int ptr: {:p}", &value);
        dbgln("(FAKE!) E char ptr: {:p}", &value);
        dbgln("(FAKE!) E char* ptr: {:p}", &value);
        dbgln("(FAKE!) E char** ptr: {:p}", &value);
    }
    return 0;
#endif
}
