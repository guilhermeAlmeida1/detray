/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/core/detail/multi_store.hpp"
#include "detray/core/detail/single_store.hpp"
#include "detray/definitions/containers.hpp"
#include "detray/definitions/indexing.hpp"
#include "detray/geometry/surface.hpp"
#include "detray/intersection/cylinder_portal_intersector.hpp"
#include "detray/io/common/detail/type_traits.hpp"  // mask_info
#include "detray/masks/masks.hpp"
#include "detray/materials/material_slab.hpp"
#include "detray/surface_finders/accelerator_grid.hpp"
#include "detray/surface_finders/brute_force_finder.hpp"

// Linear algebra types
#include "detray/definitions/algebra.hpp"

// Covfie include(s)
#include <covfie/core/backend/primitive/constant.hpp>
#include <covfie/core/vector.hpp>

namespace detray {

// linear algebra types
using transform3 = __plugin::transform3<detray::scalar>;

//
// Detector
//

/// Defines a detector that contains squares, trapezoids and a bounding portal
/// box.
struct itk_metadata {

    /// Portal link type between volumes
    using nav_link = std::uint_least16_t;

    //
    // Surface Primitives
    //

    /// The mask types for the detector sensitive surfaces
    using annulus = mask<annulus2D<>, nav_link>;
    using rectangle = mask<rectangle2D<>, nav_link>;
    // Types for portals
    using cylinder_portal =
        mask<cylinder2D<false, cylinder_portal_intersector>, nav_link>;
    using disc_portal = mask<ring2D<>, nav_link>;

    //
    // Material Description
    //

    /// The material types to be mapped onto the surfaces: Here homogeneous
    /// material
    using slab = material_slab<detray::scalar>;

    /// Constant B-field
    using bfield_backend_t =
        covfie::backend::constant<covfie::vector::vector_d<scalar, 3>,
                                  covfie::vector::vector_d<scalar, 3>>;

    /// How to store and link transforms. The geometry context allows to resolve
    /// the conditions data for e.g. module alignment
    template <template <typename...> class vector_t = dvector>
    using transform_store =
        single_store<transform3, vector_t, geometry_context>;

    /// Assign the mask types to the mask tuple container entries. It may be a
    /// good idea to have the most common types in the first tuple entries, in
    /// order to minimize the depth of the 'unrolling' before a mask is found
    /// in the tuple
    enum class mask_ids {
        e_rectangle2 = 0,
        e_annulus2 = 1,
        e_portal_cylinder2 = 2,
        e_portal_ring2 = 3,
    };

    /// This is the mask collections tuple (in the detector called 'mask store')
    /// the @c regular_multi_store is a vecemem-ready tuple of vectors of
    /// the detector masks.
    template <template <typename...> class tuple_t = dtuple,
              template <typename...> class vector_t = dvector>
    using mask_store =
        regular_multi_store<mask_ids, empty_context, tuple_t, vector_t,
                            rectangle, annulus, cylinder_portal, disc_portal>;

    /// Similar to the mask store, there is a material store, which
    enum class material_ids {
        e_slab = 0,
        e_none = 1,
    };

    /// How to store and link materials. The material does not make use of
    /// conditions data ( @c empty_context )
    template <template <typename...> class tuple_t = dtuple,
              template <typename...> class vector_t = dvector>
    using material_store = regular_multi_store<material_ids, empty_context,
                                               tuple_t, vector_t, slab>;

    /// Surface descriptor type used for sensitives, passives and portals
    /// It holds the indices to the surface data in the detector data stores
    /// that were defined above
    using transform_link = typename transform_store<>::link_type;
    using mask_link = typename mask_store<>::single_link;
    using material_link = typename material_store<>::single_link;
    using source_link = dindex;
    using surface_type = surface<mask_link, material_link, transform_link,
                                 nav_link, source_link>;

    /// How to index the constituent objects in a volume
    /// If they share the same index value here, they will be added into the
    /// same acceleration data structure in every respective volume
    enum geo_objects : unsigned int {
        e_surface = 0u,  //< This detector keeps all surfaces in the same
        e_portal = 0u,   //  acceleration data structure (id 0)
        e_passive = 0u,
        e_size = 1u
    };

    /// The acceleration data structures live in another tuple that needs to be
    /// indexed correctly:
    enum class sf_finder_ids {
        e_brute_force = 0,  //< test all surfaces in a volume (brute force)
        e_default = e_brute_force,
    };

    /// How a volume finds its constituent objects in the detector containers
    /// In this case: One range for sensitive/passive surfaces, one for portals
    using object_link_type =
        dmulti_index<dtyped_index<sf_finder_ids, dindex>, geo_objects::e_size>;

    /// The tuple store that hold the acceleration data structures for all
    /// volumes. Every collection of accelerationdata structures defines its
    /// own container and view type. Does not make use of conditions data
    /// ( @c empty_context )
    template <template <typename...> class tuple_t = dtuple,
              typename container_t = host_container_types>
    using surface_finder_store =
        multi_store<sf_finder_ids, empty_context, tuple_t,
                    brute_force_collection<surface_type, container_t>>;

    /// Data structure that allows to find the current detector volume from a
    /// given position. Here: Uniform grid with a 3D cylindrical shape
    template <typename container_t = host_container_types>
    using volume_finder =
        grid<coordinate_axes<
                 cylinder3D::axes<n_axis::bounds::e_open, n_axis::irregular,
                                  n_axis::regular, n_axis::irregular>,
                 true, container_t>,
             dindex, simple_serializer, replacer>;
};

}  // namespace detray
