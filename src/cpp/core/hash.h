#pragma once

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class T>
inline void hash_combine_array(std::size_t& seed, const T& v)
{
    for (const auto& el : v)
        hash_combine(seed, el);
}

// Hash literal string using the string itself instead of the address
inline size_t hash_literal_string(LiteralString s)
{
    return std::hash<std::string_view>()(std::string_view(s, std::strlen(s)));
}
