#ifndef _C4_BITMASK_HPP_
#define _C4_BITMASK_HPP_

#include <cinttypes>
#include <cstring>
#include <type_traits>

#include "enum.hpp"

//-----------------------------------------------------------------------------
/** write a bitmask to a stream, formatted as a string */
template< class E, class Stream >
Stream& bm2stream(typename std::underlying_type<E>::type bits, Stream &s)
{
    using I = typename std::underlying_type<E>::type;
    bool written = false;
    for(auto const& p : esyms< E >())
    {
        I b(p.value);
        if((b != 0) && ((bits & b) == b))
        {
            if(written) s << '|'; // append bit-or character
            written = true;
            s << p.name; // append bit string
            bits &= ~b;
        }
    }
    if(!written)
    {
        s << 0;
    }
}

//-----------------------------------------------------------------------------
/** convert a bitmask to string.
 * return the number of characters written. To find the needed size,
 * call first with str=nullptr and sz=0 */
template< class E >
size_t bm2str(typename std::underlying_type<E>::type bits, char *str = nullptr, size_t sz = 0)
{
    using I = typename std::underlying_type<E>::type;
    C4_ASSERT((str == nullptr) == (sz == 0));

// this macro simplifies the code
#define _c4appendchars(code, num)               \
    if(str && (pos + num <= sz)) { code; }      \
    else if(str && sz) { C4_ERROR("cannot write to string pos=%d num=%d sz=%d", (int)pos, (int)num, (int)sz); } \
    pos += num

    size_t pos = 0;
    typename EnumSymbols< E >::Sym const* zero = nullptr;
    for(auto const& p : esyms< E >())
    {
        I b = static_cast< I >(p.value);
        if(b == 0)
        {
            zero = &p; // save this symbol for later
        }
        else if((bits & b) == b)
        {
            bits &= ~b;
            // append bit-or character
            if(pos > 0)
            {
                _c4appendchars(str[pos] = '|', 1);
            }
            // append bit string
            size_t len = strlen(p.name);
            _c4appendchars(strncpy(str+pos, p.name, len), len);
        }
    }
    C4_ASSERT(bits == 0);
    if(pos == 0) // make sure at least something is written
    {
        if(zero) // if we have a zero symbol, use that
        {
            size_t len = strlen(zero->name);
            _c4appendchars(strncpy(str+pos, zero->name, len), len);
        }
        else // otherwise just write an integer zero
        {
            _c4appendchars(str[pos] = '0', 1);
        }
    }
    _c4appendchars(str[pos] = '\0', 1);

    return pos;

// cleanup!
#undef _c4appendchars
}

//-----------------------------------------------------------------------------

namespace detail {
template< class I > struct scanftag;
template<> struct scanftag< uint8_t  > { static constexpr const char *tag = "%" SCNu8 ; };
template<> struct scanftag< uint16_t > { static constexpr const char *tag = "%" SCNu16; };
template<> struct scanftag< uint32_t > { static constexpr const char *tag = "%" SCNu32; };
template<> struct scanftag< uint64_t > { static constexpr const char *tag = "%" SCNu64; };
template<> struct scanftag<  int8_t  > { static constexpr const char *tag = "%" SCNd8 ; };
template<> struct scanftag<  int16_t > { static constexpr const char *tag = "%" SCNd16; };
template<> struct scanftag<  int32_t > { static constexpr const char *tag = "%" SCNd32; };
template<> struct scanftag<  int64_t > { static constexpr const char *tag = "%" SCNd64; };
} // end namespace detail

/** convert a string to a bitmask */
template< class E >
typename std::underlying_type<E>::type str2bm(const char *str, size_t sz)
{
    using I = typename std::underlying_type<E>::type;

    I val = 0;
    auto pairs = esyms< E >();
    bool started = false;
    bool alnum = false, num = false;
    const char *f = nullptr;
    size_t n = 0;
    for(const char *c = str; c <= str + sz; ++c, ++n)
    {
        if((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_')
        {
            C4_CHECK(!num);
            if(!started)
            {
                f = c;
                n = 0;
                alnum = started = true;
            }
        }
        else if(*c >= '0' && *c <= '9')
        {
            C4_CHECK(!alnum);
            if(!started)
            {
                f = c;
                n = 0;
                num = started = true;
            }
        }
        else if(*c == ':' || *c == ' ')
        {
            // skip this char
        }
        else if(*c == '|' || *c == '\0')
        {
            C4_ASSERT(num != alnum);
            if(num)
            {
                I tmp;
                auto numconv = sscanf(f, detail::scanftag<I>::tag, &tmp);
                C4_CHECK_MSG(numconv == 1, "could not read string as an int: '%.*s'", (int)n, f);
                val |= tmp;
            }
            else if(alnum)
            {
                auto *p = pairs.find(f, n);
                C4_CHECK_MSG(p != nullptr, "no valid enum pair name for '%.*s'", (int)n, f);
                val |= static_cast< I >(p->value);
            }
            started = num = alnum = false;
            if(*c == '\0')
            {
                break;
            }
        }
        else
        {
            C4_ERROR("bad character '%c' in bitmask string", *c);
        }
    }
    return val;
}

/** convert a string to a bitmask */
template< class E >
typename std::underlying_type<E>::type str2bm(const char *str)
{
    return str2bm<E>(str, strlen(str));
}

#endif // _C4_BITMASK_HPP_
