#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ser {

using U8  = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

using S8  = std::int8_t;
using S16 = std::int16_t;
using S32 = std::int32_t;
using S64 = std::int64_t;

using USize = std::size_t;

using F32 = float;
using F64 = double;

using Byte = std::byte;
using Bool = bool;

enum class ArchiveMode { Read, Write };

template <typename ArchiveT>
concept IsArchiveT = requires(ArchiveT archive, U32& value) {
    { ArchiveT::mode } -> std::same_as<const ArchiveMode&>;
    { archive.property("property_name", value) } -> std::same_as<void>;
};

template <typename ArchiveT, typename ValueT>
concept HasMemberSerializeT =
    requires(ArchiveT& archive, ValueT& value) { value.serialize(archive); };

template <typename ArchiveT, typename ValueT>
concept HasGlobalSerializeT =
    requires(ArchiveT& archive, ValueT& value) { serialize_global(archive, value); };

template <typename ArchiveT, typename ValueT>
concept IsSerializableT =
    HasMemberSerializeT<ArchiveT, ValueT> || HasGlobalSerializeT<ArchiveT, ValueT>;

// WARNING:
// This one was written by a Clanker. It was not tested on windows with MSVC.
template <typename T>
consteval std::string_view compt_get_type_name() {
#if defined(__clang__) || defined(__GNUC__)
    std::string_view name = __PRETTY_FUNCTION__;
    // Extract the type from consteval std::string_view
    // "compt_get_type_name() [with T = TypeName]"
    size_t start = name.find("T = ") + 4;
    size_t end   = name.find_last_of(']');
    return name.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    // Extract the type from
    // "class std::basic_string_view<char,struct std::char_traits<char>>
    //  __cdecl compt_get_type_name<TypeName>(void)"
    size_t start = name.find("compt_get_type_name<") + 12;
    size_t end   = name.find_last_of('>');
    return name.substr(start, end - start);
#else
    return "UNKNOWN";
#endif
}

class DebugStdoutWriter {
   public:
    static constexpr ArchiveMode mode = ArchiveMode::Write;
    static constexpr U32 tab_width   = 4;

    template <typename ValueT>
    void property(std::string_view name, ValueT& value) {
        constexpr std::string_view compt_type_name = compt_get_type_name<ValueT>();

        if constexpr (IsSerializableT<DebugStdoutWriter, ValueT>) {
            std::println("{}[{}] {}:", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name);
            ++m_depth;

            if constexpr (HasMemberSerializeT<DebugStdoutWriter, ValueT>) {
                value.serialize(*this);
            } else {
                serialize_global(*this, value);
            }

            --m_depth;
        } else if constexpr (std::is_same_v<ValueT, U8>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, static_cast<U32>(value));
        } else if constexpr (std::is_same_v<ValueT, U16>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, U32>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, U64>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, S8>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, static_cast<S32>(value));
        } else if constexpr (std::is_same_v<ValueT, S16>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, S32>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, S64>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, USize>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, F32>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, F64>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, Bool>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value ? "true" : "false");
        } else if constexpr (std::is_same_v<ValueT, std::string>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, std::string_view>) {
            std::println("{}[{}] {}: {}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, value);
        } else if constexpr (std::is_same_v<ValueT, Byte>) {
            std::println("{}[{}] {}: 0x{:02X}", std::string(m_depth * tab_width, ' '),
                         compt_type_name, name, std::to_integer<U32>(value));
        }
    }

   private:
    U32 m_depth { 0 };
};

template <IsArchiveT ArchiveT, typename ValueT>
void serialize_global(ArchiveT& ar, std::vector<ValueT>& vec) {
    USize size = vec.size();

    ar.property("size", size);

    if constexpr (ArchiveT::mode == ArchiveMode::Read) {
        vec.resize(size);
    }

    for (USize i = 0; i < size; ++i) {
        ar.property("item." + std::to_string(i), vec[i]);
    }
}

}  // namespace ser
