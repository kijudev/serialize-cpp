#pragma once

#include <yyjson.h>

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// =========================================================================================
// Global Typedefs
// =========================================================================================

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

namespace ser {

// =========================================================================================
// Concepts, Interfaces, Helpers
// =========================================================================================

enum class ArchiveMode { Read, Write };

template <typename ArchiveT>
concept IsArchiveT =
    requires(ArchiveT archive, U32& value) {
        { ArchiveT::mode } -> std::convertible_to<const ArchiveMode&>;
        { archive.property("property_name", value) } -> std::same_as<void>;
    } && !std::is_copy_constructible_v<ArchiveT>  // <-
    && !std::is_copy_assignable_v<ArchiveT>       // <-
    && !std::is_move_constructible_v<ArchiveT>    // <-
    && !std::is_move_assignable_v<ArchiveT>;      // <-

template <typename ArchiveT, typename ValueT>
concept HasMemberSerializeT =
    requires(ArchiveT& archive, ValueT& value) { value.serialize(archive); };

template <typename ArchiveT, typename ValueT>
concept HasGlobalSerializeT =
    requires(ArchiveT& archive, ValueT& value) { serialize_global(archive, value); };

// TODO: this should propably be removed
template <typename ArchiveT, typename ValueT>
concept IsSerializableT =
    HasMemberSerializeT<ArchiveT, ValueT> || HasGlobalSerializeT<ArchiveT, ValueT>;

namespace impl {

template <typename T>
concept IsIterableT = std::ranges::range<T> && !std::is_same_v<T, std::string> &&
                      !std::is_same_v<T, std::string_view>;

// WARNING:
// This one was written by a Clanker. It was not tested on windows with MSVC.
template <typename T>
consteval std::string_view compt_get_type_name() {
#if defined(__clang__) || defined(__GNUC__)
    std::string_view name = __PRETTY_FUNCTION__;
    // Extract the type from consteval std::string_view
    // "compt_get_type_name() [with T = TypeName]"
    USize start = name.find("T = ") + 4;
    USize end   = name.find_last_of(']');
    return name.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    // Extract the type from
    // "class std::basic_string_view<char,struct std::char_traits<char>>
    // __cdecl compt_get_type_name<TypeName>(void)"
    USize start = name.find("compt_get_type_name<") + 12;
    USize end   = name.find_last_of('>');
    return name.substr(start, end - start);
#else
    return "@unknown";
#endif
}
}  // namespace impl

// =========================================================================================
// IMPLEMENTATION: DebugStdout
// =========================================================================================

class DebugStdoutWriter {
   public:
    struct Options {
        U32 tab_width { 4 };
        bool print_types : 1 { true };
    };

    static constexpr ArchiveMode mode = ArchiveMode::Write;

    DebugStdoutWriter() = default;
    DebugStdoutWriter(Options options) : m_options(options) {}

    DebugStdoutWriter(const DebugStdoutWriter&)            = delete;
    DebugStdoutWriter& operator=(const DebugStdoutWriter&) = delete;
    DebugStdoutWriter(DebugStdoutWriter&&)                 = delete;
    DebugStdoutWriter& operator=(DebugStdoutWriter&&)      = delete;

    ~DebugStdoutWriter() = default;

   public:
    template <typename ValueT>
    void property(std::string_view name, ValueT& value) {
        constexpr std::string_view compt_type_name = impl::compt_get_type_name<ValueT>();

        const std::string indent      = impl_indent();
        const std::string type_prefix = impl_type_prefix(compt_type_name);

        // ==== (1) Primitive ====
        if constexpr (std::is_integral_v<ValueT>) {
            if constexpr (std::is_signed_v<ValueT>) {
                std::println("{}{} {}: {}", indent, type_prefix, name,
                             static_cast<S64>(value));
            } else {
                std::println("{}{} {}: {}", indent, type_prefix, name,
                             static_cast<U64>(value));
            }
        } else if constexpr (std::is_floating_point_v<ValueT>) {
            std::println("{}{} {}: {}", indent, type_prefix, name, static_cast<F64>(value));
        } else if constexpr (std::is_same_v<ValueT, std::string> ||
                             std::is_same_v<ValueT, std::string_view>) {
            std::println("{}{} {}:  {}", indent, type_prefix, name, value);
        }
        // ==== (2) Iterable ====
        else if constexpr (impl::IsIterableT<ValueT>) {
            std::println("{}{} {}:", indent, type_prefix, name);

            ++m_depth;

            USize idx = 0;

            for (auto& item : value) {
                property("[" + std::to_string(idx) + "]", item);
                ++idx;
            }

            --m_depth;
        }
        // ==== (3) Serializable ====
        else if constexpr (IsSerializableT<DebugStdoutWriter, ValueT>) {
            std::println("{}{} {}:", indent, type_prefix, name);
            ++m_depth;

            if constexpr (HasMemberSerializeT<DebugStdoutWriter, ValueT>) {
                value.serialize(*this);
            } else {
                serialize_global(*this, value);
            }

            --m_depth;
        }
        // ==== (3) Panic ====
        else {
            static_assert(
                false,
                "\n"
                "SER   | -> [DebugStdoutWriter :: property(std::string_view, ValueT&)]\n"
                "SER   | -> Unable to Serialize.\n"
                "SER   | -> Provided Type does not implement [template <typename "
                "ArchiveT>serialize(ArchiveT)].\n");
        }
    }

   private:
    std::string impl_indent() const {
        return std::string(m_depth * m_options.tab_width, ' ');
    }

    std::string impl_type_prefix(std::string_view type_name) const {
        if (!m_options.print_types) {
            return {};
        }

        std::string prefix;

        prefix.reserve(type_name.size() + 3);
        prefix.push_back('[');
        prefix.append(type_name);
        prefix.push_back(']');

        return prefix;
    }

   private:
    // ==== State ====
    U32 m_depth { 0 };
    const Options m_options;
};

// =========================================================================================
// IMPLEMENTATION: Json
// =========================================================================================

class JsonWriter {
   public:
    struct Options {
        bool include_type : 1 { true };
        bool pretty       : 1 { true };
    };

    enum class State : U8 { Init, Serialize, Done, Error };

    static constexpr ArchiveMode mode = ArchiveMode::Write;

    JsonWriter() : JsonWriter(Options {}) {};
    JsonWriter(Options options) : m_options(options) {
        m_state = State::Init;

        m_doc = yyjson_mut_doc_new(nullptr);
        if (!m_doc) {
            m_state = State::Error;
            return;
        }

        m_root = yyjson_mut_obj(m_doc);
        if (!m_root) {
            m_state = State::Error;
            return;
        }

        yyjson_mut_doc_set_root(m_doc, m_root);
        m_stack.push_back(m_root);
        m_state = State::Init;
    }

    JsonWriter(const JsonWriter&)            = delete;
    JsonWriter& operator=(const JsonWriter&) = delete;
    JsonWriter(JsonWriter&&)                 = delete;
    JsonWriter& operator=(JsonWriter&&)      = delete;

    ~JsonWriter() { impl_cleanup(); }

   public:
    std::string format() {
        m_output.clear();

        if (m_state == State::Error || !m_doc) {
            return m_output;
        }

        const yyjson_write_flag flags =
            m_options.pretty ? YYJSON_WRITE_PRETTY : YYJSON_WRITE_NOFLAG;

        USize len  = 0;
        char* data = yyjson_mut_write(m_doc, flags, &len);

        if (!data) {
            m_state = State::Error;
            return m_output;
        }

        m_output.assign(data, len);
        std::free(data);

        m_state = State::Done;
        return m_output;
    }

    void reset() {
        m_output.clear();
        m_stack.clear();
        m_state = State::Init;

        if (m_doc) {
            yyjson_mut_doc_free(m_doc);
        }

        m_doc = yyjson_mut_doc_new(nullptr);
        if (!m_doc) {
            m_root  = nullptr;
            m_state = State::Error;
            return;
        }

        m_root = yyjson_mut_obj(m_doc);
        if (!m_root) {
            m_state = State::Error;
            return;
        }

        yyjson_mut_doc_set_root(m_doc, m_root);
        m_stack.push_back(m_root);
        m_state = State::Init;
    }

    State state() const { return m_state; }

    template <typename ValueT>
    void property(std::string_view name, ValueT& value) {
        // SAFETY: If writer is in an incorrect state return.
        if (m_state == State::Error || m_state == State::Done) {
            return;
        }

        if (!m_doc || !m_root) {
            m_state = State::Error;
            return;
        }

        if (m_state == State::Init) {
            m_state = State::Serialize;
        }

        // ==== (1) Primitive ====
        if constexpr (std::is_integral_v<ValueT>) {
            if constexpr (std::is_signed_v<ValueT>) {
                impl_add_value(name, yyjson_mut_sint(m_doc, static_cast<S64>(value)));
            } else {
                impl_add_value(name, yyjson_mut_uint(m_doc, static_cast<U64>(value)));
            }
        } else if constexpr (std::is_floating_point_v<ValueT>) {
            impl_add_value(name, yyjson_mut_double(m_doc, static_cast<F64>(value)));
        } else if constexpr (std::is_same_v<ValueT, std::string> ||
                             std::is_same_v<ValueT, std::string_view>) {
            impl_add_value(name, yyjson_mut_strncpy(m_doc, value.data(), value.size()));
        }
        // ==== (2) Iterable ====
        else if constexpr (impl::IsIterableT<ValueT>) {
            yyjson_mut_val* arr = yyjson_mut_arr(m_doc);
            impl_add_value(name, arr);
            m_stack.push_back(arr);

            for (auto& item : value) {
                property("", item);
            }

            m_stack.pop_back();
        }
        // ==== (3) Serializable ====
        else if constexpr (IsSerializableT<JsonWriter, ValueT>) {
            yyjson_mut_val* obj = yyjson_mut_obj(m_doc);
            if (!obj) {
                m_state = State::Error;
                return;
            }

            impl_add_value(name, obj);
            m_stack.push_back(obj);

            constexpr std::string_view compt_type_name =
                impl::compt_get_type_name<ValueT>();

            if (m_options.include_type) {
                impl_add_value("@typename",
                               yyjson_mut_strncpy(m_doc, compt_type_name.data(),
                                                  compt_type_name.size()));
            }

            if constexpr (HasMemberSerializeT<JsonWriter, ValueT>) {
                value.serialize(*this);
            } else {
                serialize_global(*this, value);
            }

            m_stack.pop_back();
        }
        // ==== (4) Panic ====
        else {
            static_assert(false,
                          "\n"
                          "SER   | -> [JsonWriter :: property(std::string_view, ValueT&)]\n"
                          "SER   | -> Unable to Serialize.\n"
                          "SER   | -> Provided Type does not implement [template <typename "
                          "ArchiveT>serialize(ArchiveT)].\n");
        }
    }

   private:
    void impl_cleanup() {
        yyjson_mut_doc_free(m_doc);
        m_doc  = nullptr;
        m_root = nullptr;
        m_stack.clear();
    }

    void impl_add_value(std::string_view name, yyjson_mut_val* val) {
        if (!m_doc || !val) {
            m_state = State::Error;
            return;
        }

        yyjson_mut_val* parent = impl_curr_obj();
        if (!parent) {
            m_state = State::Error;
            return;
        }

        if (yyjson_mut_is_arr(parent)) {
            yyjson_mut_arr_append(parent, val);
        } else {
            yyjson_mut_val* key = yyjson_mut_strncpy(m_doc, name.data(), name.size());
            if (!key) {
                m_state = State::Error;
                return;
            }
            yyjson_mut_obj_add(parent, key, val);
        }
    }

    yyjson_mut_val* impl_curr_obj() { return m_stack.empty() ? m_root : m_stack.back(); }

   private:
    // ==== Json ====
    yyjson_mut_doc* m_doc { nullptr };
    yyjson_mut_val* m_root { nullptr };

    // ==== Stack ====
    std::vector<yyjson_mut_val*> m_stack {};

    // ==== State ===
    std::string m_output {};
    State m_state { State::Init };
    const Options m_options;
};

class JsonReader {
   public:
    // NOTE: For future use.
    struct Options {};

    enum class State : U8 { Init, Serialize, Done, Error };

    static constexpr ArchiveMode mode = ArchiveMode::Read;

    JsonReader(std::string_view json_string) : JsonReader(json_string, Options {}) {}
    JsonReader(std::string_view json_string, Options options) : m_options(options) {
        (void)m_options;

        m_state = State::Init;

        yyjson_read_flag flags = YYJSON_READ_NOFLAG;
        yyjson_read_err err;
        m_doc = yyjson_read_opts(const_cast<char*>(json_string.data()), json_string.size(),
                                 flags, nullptr, &err);

        if (!m_doc) {
            m_state = State::Error;
            return;
        }

        m_root = yyjson_doc_get_root(m_doc);
        if (!m_root) {
            m_state = State::Error;
            return;
        }

        m_stack.push_back(m_root);
        m_arr_indices.push_back(0);
        m_state = State::Init;
    }

    JsonReader(const JsonReader&)            = delete;
    JsonReader& operator=(const JsonReader&) = delete;
    JsonReader(JsonReader&&)                 = delete;
    JsonReader& operator=(JsonReader&&)      = delete;

    ~JsonReader() { impl_cleanup(); }

   public:
    void reset(std::string_view json_data) {
        impl_cleanup();
        m_state = State::Init;

        m_doc = yyjson_read_opts(const_cast<char*>(json_data.data()), json_data.size(),
                                 YYJSON_READ_NOFLAG, nullptr, nullptr);

        if (!m_doc) {
            m_state = State::Error;
            return;
        }

        m_root = yyjson_doc_get_root(m_doc);
        if (!m_root) {
            m_state = State::Error;
            return;
        }

        m_stack.push_back(m_root);
        m_arr_indices.push_back(0);
    }

    State state() const { return m_state; }

    // NOTE:
    template <typename ValueT>
    void property(std::string_view name, ValueT& value) {
        // SAFETY: If reader is in an incorrect state we return.
        if (m_state == State::Error || m_state == State::Done) {
            return;
        }

        if (!m_doc || !m_root) {
            m_state = State::Error;
            return;
        }

        if (m_state == State::Init) {
            m_state = State::Serialize;
        }

        yyjson_val* val = impl_get_value(name);

        // SAFETY: If val is NULL we return; the deafult C++ constructor/initializer is
        // called.
        if (!val) {
            return;
        }

        // ==== (1) Primitive ====
        // NOTE: Bool is separated from standard integral types because yyjson treats
        // differently.
        if constexpr (std::is_same_v<ValueT, bool>) {
            if (yyjson_is_bool(val)) {
                value = yyjson_get_bool(val);
            }
        } else if constexpr (std::is_integral_v<ValueT>) {
            if constexpr (std::is_signed_v<ValueT>) {
                if (yyjson_is_int(val)) {
                    value = static_cast<ValueT>(yyjson_get_sint(val));
                }
            } else {
                if (yyjson_is_uint(val)) {
                    value = static_cast<ValueT>(yyjson_get_uint(val));
                }
            }
        } else if constexpr (std::is_floating_point_v<ValueT>) {
            if (yyjson_is_num(val)) {
                value = static_cast<ValueT>(yyjson_get_num(val));
            }
        } else if constexpr (std::is_same_v<ValueT, std::string>) {
            if (yyjson_is_str(val)) {
                value.assign(yyjson_get_str(val), yyjson_get_len(val));
            }
        } else if constexpr (std::is_same_v<ValueT, std::string_view>) {
            // WARNING: std::string_view will dangle if the JsonReader is destroyed.
            if (yyjson_is_str(val)) {
                value = std::string_view(yyjson_get_str(val), yyjson_get_len(val));
            }
        }
        // ==== (2) Iterable ====
        else if constexpr (impl::IsIterableT<ValueT>) {
            if (!yyjson_is_arr(val)) {
                m_state = State::Error;
                return;
            }

            m_stack.push_back(val);
            m_arr_indices.push_back(0);

            USize size = yyjson_arr_size(val);

            // Allocate suffiecent memory before reading.
            if constexpr (requires { value.resize(size); }) {
                value.resize(size);
            }

            for (auto& item : value) {
                // TODO: Add some nice idiom, constepr value for name-less properties.
                property("", item);
            }

            m_stack.pop_back();
            m_arr_indices.pop_back();
        }
        // ==== (3) Serializable ===
        else if constexpr (IsSerializableT<JsonReader, ValueT>) {
            if (!yyjson_is_obj(val)) {
                m_state = State::Error;
                return;
            }

            m_stack.push_back(val);
            m_arr_indices.push_back(0);

            if constexpr (HasMemberSerializeT<JsonReader, ValueT>) {
                value.serialize(*this);
            } else {
                serialize_global(*this, value);
            }

            m_stack.pop_back();
            m_arr_indices.pop_back();

        }
        // ==== (4) Panic ===
        else {
            static_assert(false,
                          "\n"
                          "SER   | -> [JsonReader :: property(std::string_view, ValueT&)]\n"
                          "SER   | -> Unable to Serialize.\n"
                          "SER   | -> Provided Type does not implement [template <typename "
                          "ArchiveT>serialize(ArchiveT)].\n");
        }
    }

   private:
    void impl_cleanup() {
        if (m_doc) {
            yyjson_doc_free(m_doc);
            m_doc = nullptr;
        }

        m_root = nullptr;
        m_stack.clear();
        m_arr_indices.clear();
    }

    yyjson_val* impl_get_value(std::string_view name) {
        if (m_stack.empty()) return nullptr;

        yyjson_val* parent = m_stack.back();
        if (!parent) return nullptr;

        if (yyjson_is_arr(parent)) {
            // We are inside an array, ignore the property name.
            USize& idx      = m_arr_indices.back();
            yyjson_val* val = yyjson_arr_get(parent, idx);
            ++idx;

            return val;
        } else if (yyjson_is_obj(parent)) {
            // We are inside an object, get value by name.
            return yyjson_obj_getn(parent, name.data(), name.size());
        }

        return nullptr;
    }

   private:
    // ==== Json ====
    yyjson_doc* m_doc { nullptr };
    yyjson_val* m_root { nullptr };

    // ==== Stacks ====
    // For the nodes, like in JsonWriter.
    std::vector<yyjson_val*> m_stack {};
    // Used to keep track of the array indices currently being processed.
    std::vector<USize> m_arr_indices {};

    // ==== State ====
    State m_state { State::Init };
    const Options m_options;
};

// =========================================================================================
// Generic `serialize_global` implementations for common std containers.
// =========================================================================================

}  // namespace ser
