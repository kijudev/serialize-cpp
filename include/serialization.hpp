#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>

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

enum class ArchiveMode { Read, Write };

template <typename ArchiveT>
concept IsArchiveT =
    requires(ArchiveT archive, U32& value) { archive.property("property_name", value); };

template <typename ArchiveT, typename ValueT>
concept HasMemberSerialize =
    requires(ArchiveT& archive, ValueT& value) { value.serialize(archive); };

template <typename ArchiveT, typename ValueT>
concept HasGlobalSerialize =
    requires(ArchiveT& archive, ValueT& value) { serialize(archive, value); };

template <typename ArchiveT, typename ValueT>
concept IsSerializable = HasGlobalSerialize<ArchiveT, ValueT>;

class DebugWriter {
    U32 m_depth { 0 };

   public:
    template <typename ValueT>
    void property(std::string_view name, ValueT& value) {
        if constexpr (HasMemberSerialize<DebugWriter, ValueT>) {
            std::println("{}{}:", std::string(m_depth, '\t'), name);
            ++m_depth;
            value.serialize(*this);
            --m_depth;
        } else if constexpr (std::is_same_v<ValueT, U32>) {
            std::println("{}{}: {}", std::string(m_depth, '\t'), name, value);
        } else if constexpr (std::is_same_v<ValueT, F32>) {
            std::println("{}{}: {}", std::string(m_depth, '\t'), name, value);
        }
    }
};

class DebugReader {};
}  // namespace ser
