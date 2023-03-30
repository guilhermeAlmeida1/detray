
/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/definitions/detail/bit_encoder.hpp"
#include "detray/definitions/geometry.hpp"
#include "detray/definitions/qualifiers.hpp"

// System include(s)
#include <cstdint>
#include <ostream>

namespace detray::geometry {

/// @brief Unique identifier for geometry objects, based on the ACTS
/// GeometryIdentifier
///
/// Encodes the volume index, the type of surface (portal, sensitive, passive
/// etc), an index to identify a surface in a geometry accelerator structure,
/// as well as two extra bytes that can be used to tag surfaces arbitrarily.
///
/// @note the detray barcode is not compatible with the ACTS GeometryIdentifier!
///
/// @see
/// https://github.com/acts-project/acts/blob/main/Core/include/Acts/Geometry/GeometryIdentifier.hpp
class barcode {

    public:
    using value_t = uint64_t;
    using encoder = detail::bit_encoder<value_t>;

    /// Construct from an already encoded value.
    DETRAY_HOST_DEVICE
    constexpr explicit barcode(value_t encoded) : m_value(encoded) {}

    /// Construct default barcode with all values set to 1.
    constexpr barcode() = default;

    /// @returns the encoded value.
    DETRAY_HOST_DEVICE
    constexpr value_t value() const noexcept { return m_value; }

    /// @returns the volume index.
    DETRAY_HOST_DEVICE
    constexpr value_t volume() const {
        return encoder::template get_bits<k_volume_mask>(m_value);
    }

    /// @returns the surface id.
    DETRAY_HOST_DEVICE
    constexpr surface_id id() const {
        return static_cast<surface_id>(
            encoder::template get_bits<k_id_mask>(m_value));
    }

    /// @returns the surface index.
    DETRAY_HOST_DEVICE
    constexpr value_t index() const {
        return encoder::template get_bits<k_index_mask>(m_value);
    }

    /// @returns the extra identifier
    DETRAY_HOST_DEVICE
    constexpr value_t extra() const {
        return encoder::template get_bits<k_extra_mask>(m_value);
    }

    /// Set the volume index.
    DETRAY_HOST_DEVICE
    constexpr barcode& set_volume(value_t volume) {
        encoder::template set_bits<k_volume_mask>(m_value, volume);
        return *this;
    }

    /// Set the surface id.
    DETRAY_HOST_DEVICE
    constexpr barcode& set_id(surface_id id) {
        encoder::template set_bits<k_id_mask>(m_value,
                                              static_cast<value_t>(id));
        return *this;
    }

    /// Set the surface index.
    DETRAY_HOST_DEVICE
    constexpr barcode& set_index(value_t index) {
        encoder::template set_bits<k_index_mask>(m_value, index);
        return *this;
    }

    /// Set the extra identifier.
    DETRAY_HOST_DEVICE
    constexpr barcode& set_extra(value_t extra) {
        encoder::template set_bits<k_extra_mask>(m_value, extra);
        return *this;
    }

    /// Check wether the barcode is valid to use.
    /// @note The extra bits are allowed to be invalid and will not be checked
    DETRAY_HOST_DEVICE
    constexpr bool is_invalid() const noexcept {
        return encoder::template is_invalid<k_volume_mask, k_id_mask,
                                            k_index_mask>(m_value);
    }

    private:
    // clang-format off
    static constexpr value_t k_volume_mask = 0xfff0000000000000; // (2^12)-1 = 4095 volumes 
    static constexpr value_t k_id_mask     = 0x000f000000000000; // (2^4)-1 = 15 surface categories 
    static constexpr value_t k_index_mask  = 0x0000ffffffffff00; // (2^40)-1 = 1099511627775 surfaces (per volume and id)
    static constexpr value_t k_extra_mask  = 0x00000000000000ff; // (2^8)-1 = 255 extra values
    // clang-format on

    /// The encoded value. Default: All bits set to 1 (invalid)
    value_t m_value{~static_cast<value_t>(0)};

    DETRAY_HOST_DEVICE
    friend constexpr bool operator==(barcode lhs, barcode rhs) noexcept {
        return lhs.m_value == rhs.m_value;
    }

    DETRAY_HOST_DEVICE
    friend constexpr bool operator!=(barcode lhs, barcode rhs) noexcept {
        return lhs.m_value != rhs.m_value;
    }

    DETRAY_HOST_DEVICE
    friend constexpr bool operator<(barcode lhs, barcode rhs) noexcept {
        return lhs.m_value < rhs.m_value;
    }

    DETRAY_HOST
    friend std::ostream& operator<<(std::ostream& os, const barcode c) {
        if (c.is_invalid()) {
            return (os << "undefined");
        }

        static const char* const names[] = {
            "vol = ", "id = ", "index = ", "extra = "};
        const value_t levels[] = {c.volume(), static_cast<value_t>(c.id()),
                                  c.index(), c.extra()};

        bool writeSeparator = false;
        for (auto i = 0u; i < 4u; ++i) {
            if (writeSeparator) {
                os << " | ";
            }
            os << names[i] << levels[i];
            writeSeparator = true;
        }
        return os;
    }
};

}  // namespace detray::geometry

namespace std {

// specialize std::hash so barcode can be used e.g. in an unordered_map
template <>
struct hash<detray::geometry::barcode> {
    auto operator()(detray::geometry::barcode gid) const noexcept {
        return std::hash<detray::geometry::barcode::value_t>()(gid.value());
    }
};

}  // namespace std