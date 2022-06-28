/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/definitions/qualifiers.hpp"
#include "detray/intersection/detail/trajectories.hpp"
#include "detray/intersection/intersection.hpp"

// System include(s)
#include <cmath>
#include <type_traits>

namespace detray {

/// @brief Intersection implementation for cylinder surfaces using helical
/// trajectories.
///
/// The algorithm uses the Newton-Raphson method to find an intersection on
/// the unbounded surface and then applies the mask. On the @c cylinder3
/// mask, it switches on the check of the radial distance.
struct helix_cylinder_intersector {

    using intersection_type = line_plane_intersection;
    using output_type = std::array<intersection_type, 2>;
    using point3 = __plugin::point3<detray::scalar>;
    using vector3 = __plugin::vector3<detray::scalar>;

    /// Operator function to find intersections between helix and cylinder mask
    ///
    /// @tparam mask_t is the input mask type
    /// @tparam transform_t is the input transform type
    ///
    /// @param h is the input helix trajectory
    /// @param mask is the input mask
    /// @param trf is the transform
    /// @param mask_tolerance is the tolerance for mask edges
    /// @param overstep_tolerance is the tolerance for track overstepping
    ///
    /// @return the intersection
    template <typename mask_t, typename transform_t>
    DETRAY_HOST_DEVICE inline output_type operator()(
        const detail::helix &h, const mask_t &mask, const transform_t &trf,
        const scalar mask_tolerance = 0) const {

        output_type ret;

        using local_frame = typename mask_t::local_type;

        // Guard against inifinite loops
        constexpr std::size_t max_n_tries{100};
        // Tolerance for convergence
        constexpr scalar tol{1e-3};

        // Get the surface placement
        const auto &sm = trf.matrix();
        // Cylinder z axis
        const vector3 sz = getter::vector<3>(sm, 0, 2);
        // Cylinder centre
        const point3 sc = getter::vector<3>(sm, 0, 3);

        // Starting point on the helix for the Newton iteration
        // The mask is a cylinder -> it provides its radius as the first value
        const scalar r{mask[0]};
        // Helix path length parameter
        scalar s{r * getter::perp(h.dir(tol))};
        // Path length in the previous iteration step
        scalar s_prev{s - scalar{0.1}};

        // f(s) = ((h.pos(s) - sc) x sz)^2 - r^2 == 0
        // Run the iteration on s
        std::size_t n_tries{0};
        while (std::abs(s - s_prev) > tol and n_tries < max_n_tries) {

            // f'(s) = 2 * ( (h.pos(s) - sc) x sz) * (h.dir(s) x sz) )
            const vector3 crp = vector::cross(h.pos(s) - sc, sz);
            const scalar denom{scalar{2.} *
                               vector::dot(crp, vector::cross(h.dir(s), sz))};
            // No intersection can be found if dividing by zero
            if (denom == scalar{0.}) {
                return ret;
            }
            // x_n+1 = x_n - f(s) / f'(s)
            s_prev = s;
            s -= (vector::dot(crp, crp) - r * r) / denom;

            ++n_tries;
        }
        // No intersection found within max number of trials
        if (n_tries == max_n_tries) {
            return ret;
        }

        // Build intersection struct from helix parameter s
        intersection_type &is = ret[0];
        const point3 helix_pos = h.pos(s);

        is.path = getter::norm(helix_pos);
        is.p3 = helix_pos;
        constexpr local_frame local_converter{};
        is.p2 = local_converter(trf, is.p3);

        auto local3 = trf.point_to_local(is.p3);
        // Explicitly check for radial match
        is.status =
            mask.template is_inside<local_frame, true>(local3, mask_tolerance);
        is.direction = vector::dot(is.p3, h.dir(s)) > scalar{0.}
                           ? intersection::direction::e_along
                           : intersection::direction::e_opposite;
        is.link = mask.volume_link();

        return ret;
    }
};

}  // namespace detray