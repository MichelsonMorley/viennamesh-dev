/* ============================================================================
   Copyright (c) 2011-2014, Institute for Microelectronics,
                            Institute for Analysis and Scientific Computing,
                            TU Wien.

                            -----------------
                ViennaMesh - The Vienna Meshing Framework
                            -----------------

                    http://viennamesh.sourceforge.net/

   License:         MIT (X11), see file LICENSE in the base directory
=============================================================================== */

#include <set>
#include <map>
#include <iterator>

#include "recombine_slice.hpp"
#include "geometry.hpp"

#include "viennagridpp/algorithm/distance.hpp"
#include "viennagridpp/algorithm/centroid.hpp"


namespace viennamesh
{


  void reflect(viennagrid::mesh_t const & input,
               viennagrid::mesh_t const & output,
               viennagrid::point_t const & centroid,
               viennagrid::point_t const & normal,
               double tol)
  {
    typedef viennagrid::mesh_t MeshType;

    typedef viennagrid::result_of::point<MeshType>::type PointType;
    typedef viennagrid::result_of::element<MeshType>::type ElementType;

    typedef viennagrid::result_of::const_element_range<MeshType>::type ConstElementRangeType;
    typedef viennagrid::result_of::iterator<ConstElementRangeType>::type ConstElementIteratorType;

    std::map< ElementType, std::pair<ElementType,ElementType> > vertex_map;

    ConstElementRangeType vertices(input, 0);
    for (ConstElementIteratorType vit = vertices.begin(); vit != vertices.end(); ++vit)
    {
      PointType point = viennagrid::get_point( *vit );

      double d = viennagrid::inner_prod( point-centroid, normal );
      if (std::abs(d) < tol)
      {
        ElementType nv = viennagrid::make_vertex( output, point );
        vertex_map[*vit] = std::make_pair(nv, nv);
      }
      else
      {
        assert( d > 0 );
        PointType reflected = point - 2 * normal * d;

        ElementType nv0 = viennagrid::make_vertex( output, point );
        ElementType nv1 = viennagrid::make_vertex( output, reflected );

//         std::cout << "Reflecting " << nv0 << " to " << nv1 << std::endl;

        vertex_map[*vit] = std::make_pair(nv0, nv1);
      }
    }


    ConstElementRangeType cells(input, viennagrid::cell_dimension(input));
    for (ConstElementIteratorType cit = cells.begin(); cit != cells.end(); ++cit)
    {
      typedef viennagrid::result_of::const_element_range<ElementType>::type ConstBoundaryRangeType;
      typedef viennagrid::result_of::iterator<ConstBoundaryRangeType>::type ConstBoundaryIteratorType;

      ConstBoundaryRangeType boundary_vertices(*cit, 0);

      std::vector<ElementType> vertices[2];

      int i = 0;
      for (ConstBoundaryIteratorType bvit = boundary_vertices.begin(); bvit != boundary_vertices.end(); ++bvit, ++i)
      {
        vertices[0].push_back( vertex_map[*bvit].first );
        vertices[1].push_back( vertex_map[*bvit].second );
      }

      for (int i = 0; i != 2; ++i)
      {
        ElementType cell = viennagrid::make_element( output, (*cit).tag(), vertices[i].begin(), vertices[i].end() );
        viennagrid::copy_region_information(*cit, cell);
      }
    }
  }


//   template<typename MeshT, typename PointT, typename NumericConfigT>
//   void slice_normals(MeshT const & mesh, PointT axis, NumericConfigT tol,
//                      PointT & N0, PointT & N1, int & rotational_frequency)
//   {
//     typedef typename viennagrid::result_of::point<MeshT>::type PointType;
//     typedef typename viennagrid::result_of::coord<MeshT>::type CoordType;
//
//     axis.normalize();
//     PointType X;
//     PointType Y;
//
//     X = viennagrid::make_point(0,1,0);
//     if ( std::abs(viennagrid::inner_prod(axis,X)) > 1.0- viennagrid::detail::absolute_tolerance<CoordType>(tol) )
//       X = viennagrid::make_point(-1,0,0);
//     X -= axis * viennagrid::inner_prod(axis,X);
//     X.normalize();
//
//     Y = viennagrid::cross_prod(axis, X);
//
//     typedef typename viennagrid::result_of::const_element_range<MeshT>::type ConstElementRangeType;
//     typedef typename viennagrid::result_of::iterator<ConstElementRangeType>::type ConstElementIteratorType;
//
//
//     double min_angle = 10.0;
//     double max_angle = -10.0;
//
//     ConstElementRangeType vertices(mesh, 0);
//     for (ConstElementIteratorType vit = vertices.begin(); vit != vertices.end(); ++vit)
//     {
//       PointT point = viennagrid::get_point(*vit);
//
//       CoordType x = viennagrid::inner_prod(point, X);
//       CoordType y = viennagrid::inner_prod(point, Y);
//       CoordType current_angle = std::atan2(y,x);
//
//       std::cout << "Vertex " << *vit << " has angle " << current_angle << std::endl;
//
//       min_angle = std::min(min_angle, current_angle);
//       max_angle = std::max(max_angle, current_angle);
//     }
//
//
//     rotational_frequency = static_cast<int>(2*M_PI/(max_angle-min_angle)+0.5);
//     N0 = rotate(X, axis, min_angle + M_PI/2);
//     N1 = -rotate(X, axis, max_angle + M_PI/2);
//
//
//     std::cout << "axis = " << axis << std::endl;
//     std::cout << "X = " << X << std::endl;
//     std::cout << "Y = " << Y << std::endl;
//
//     std::cout << "Found min_angle = " << min_angle << " normal = " << N0 << std::endl;
//     std::cout << "Found max_angle = " << max_angle << " normal = " << N1 << std::endl;
//     std::cout << "rotational_frequency = " << rotational_frequency << std::endl;
//
//
// //     return std::make_pair(N0, N1);
//   }



  recombine_symmetric_slice::recombine_symmetric_slice() {}
  std::string recombine_symmetric_slice::name() { return "recombine_symmetric_slice"; }

  bool recombine_symmetric_slice::run(viennamesh::algorithm_handle &)
  {
    typedef viennagrid::mesh_t MeshType;
    typedef viennagrid::result_of::region<MeshType>::type RegionType;

    typedef viennagrid::result_of::point<MeshType>::type PointType;
    typedef viennagrid::result_of::element<MeshType>::type ElementType;

    typedef viennagrid::result_of::const_element_range<MeshType>::type ConstElementRangeType;
    typedef viennagrid::result_of::iterator<ConstElementRangeType>::type ConstElementIteratorType;

    typedef viennagrid::result_of::const_element_range<ElementType>::type ConstBoundaryElementRangeType;
    typedef viennagrid::result_of::iterator<ConstBoundaryElementRangeType>::type ConstBoundaryElementIteratorType;

    double tol = 1e-6;
    if (get_input<double>("tolerance").valid())
      tol = get_input<double>("tolerance")();

    bool mirror_if_even = true;
    if (get_input<bool>("mirror_if_even").valid())
      mirror_if_even = get_input<bool>("mirror_if_even")();

    mesh_handle input_mesh = get_required_input<mesh_handle>("mesh");

    int geometric_dimension = viennagrid::geometric_dimension(input_mesh());



    PointType axis = get_required_input<point_t>("axis")();
    axis.normalize();


    int rotational_frequency = get_required_input<int>("rotational_frequency")();
    double angle = 2*M_PI/rotational_frequency;

    PointType N[2];
    N[0] = viennagrid::make_point(0,1,0);
    if ( std::abs(viennagrid::inner_prod(axis,N[0])) > 1.0-tol )
      N[0] = viennagrid::make_point(-1,0,0);
    N[0] -= axis * viennagrid::inner_prod(axis,N[0]);
    N[0].normalize();

    N[1] = -rotate(N[0], axis, angle);




    MeshType tmp;
    if ( (rotational_frequency%2==0) && mirror_if_even )
    {
//       mesh_handle output_mesh = make_data<mesh_handle>();
      reflect(input_mesh(), tmp, PointType(geometric_dimension), N[1], tol);
      rotational_frequency /= 2;
      angle *= 2;
      N[1] = -rotate(N[0], axis, angle);

//       set_output("mesh", output_mesh);
//       return true;
    }
    else
      tmp = input_mesh();




    info(1) << "Using rotational frequency " << rotational_frequency << std::endl;
    info(1) << "Angle = " << angle << std::endl;
    info(1) << "Axis = " << axis << std::endl;
    info(1) << "Normal[0] = " << N[0] << std::endl;
    info(1) << "Normal[1] = " << N[1] << std::endl;
//     info(1) << "Centroid = " << centroid << std::endl;

//     slice_normals(tmp, axis, tol, N[0], N[1], rotational_frequency);


    ConstElementRangeType vertices( tmp, 0 );

    std::vector<ElementType> vertices_on_both_planes;
    std::vector<ElementType> vertices_on_plane0;
    std::vector<ElementType> vertices_on_plane1;
    std::vector<ElementType> vertices_on_no_plane;

    for (ConstElementIteratorType vit = vertices.begin(); vit != vertices.end(); ++vit)
    {
      PointType p = viennagrid::get_point(*vit);

      double dp0 = std::abs(viennagrid::inner_prod(p, N[0]));
      double dp1 = std::abs(viennagrid::inner_prod(p, N[1]));

      if ( (dp0 < tol) && (dp1 < tol) )
        vertices_on_both_planes.push_back(*vit);
      else if ( dp0 < tol )
        vertices_on_plane0.push_back(*vit);
      else if ( dp1 < tol )
        vertices_on_plane1.push_back(*vit);
      else
        vertices_on_no_plane.push_back(*vit);
    }
    assert( vertices_on_plane0.size() == vertices_on_plane1.size() );
    if ( vertices_on_plane0.size() != vertices_on_plane1.size() )
    {
      std::cout << "ERROR!! vertices_on_plane0.size() != vertices_on_plane1.size() " << vertices_on_plane0.size() << " " << vertices_on_plane1.size() << std::endl;
    }


    {
      std::vector<ElementType> reordered_vertices_on_plane1( vertices_on_plane1.size() );
      for (std::size_t i0 = 0; i0 != vertices_on_plane0.size(); ++i0)
      {
        for (std::size_t i1 = 0; i1 != vertices_on_plane1.size(); ++i1)
        {
          PointType p1 = viennagrid::get_point( vertices_on_plane1[i1] );
          PointType p0 = viennagrid::get_point( vertices_on_plane0[i0] );

          if (viennagrid::detail::is_equal(tol,
                                          rotate(p0, PointType(geometric_dimension), axis, angle),
                                          p1) )
          {
            reordered_vertices_on_plane1[i0] = vertices_on_plane1[i1];
            break;
          }
        }
      }

      vertices_on_plane1 = reordered_vertices_on_plane1;
    }



    std::size_t shared_vertex_count = vertices_on_both_planes.size();
    std::size_t non_shared_vertex_count = vertices_on_plane0.size() + vertices_on_no_plane.size();

    int offset = 0;
    std::vector<int> vertex_mapping( vertices.size() );
    for (std::size_t i = 0; i != vertices_on_both_planes.size(); ++i)
      vertex_mapping[vertices_on_both_planes[i].id()] = i+offset;
    offset += vertices_on_both_planes.size();

    for (std::size_t i = 0; i != vertices_on_plane0.size(); ++i)
      vertex_mapping[vertices_on_plane0[i].id()] = i+offset;
    offset += vertices_on_plane0.size();

    for (std::size_t i = 0; i != vertices_on_no_plane.size(); ++i)
      vertex_mapping[vertices_on_no_plane[i].id()] = i+offset;
    offset += vertices_on_no_plane.size();

    for (std::size_t i = 0; i != vertices_on_plane1.size(); ++i)
      vertex_mapping[vertices_on_plane1[i].id()] = i+offset;


    std::vector<ElementType> new_vertices;

    mesh_handle output_mesh = make_data<mesh_handle>();



    typedef viennagrid::result_of::const_region_range<MeshType>::type RegionRangeType;
    typedef viennagrid::result_of::iterator<RegionRangeType>::type RegionIteratorType;

    RegionRangeType regions( tmp );
    std::vector<RegionType> new_regions;
    for (RegionIteratorType rit = regions.begin(); rit != regions.end(); ++rit)
    {
      RegionType region = output_mesh().get_make_region( (*rit).id() );
      region.set_name( (*rit).name() );
      new_regions.push_back(region);
    }




    for (std::size_t i = 0; i != vertices_on_both_planes.size(); ++i)
      new_vertices.push_back( viennagrid::make_vertex(output_mesh(), viennagrid::get_point(vertices_on_both_planes[i])) );

    for (int rf = 0; rf != rotational_frequency; ++rf)
    {
      double current_angle = angle * rf;

      for (std::size_t i = 0; i != vertices_on_plane0.size(); ++i)
        new_vertices.push_back( viennagrid::make_vertex(output_mesh(), rotate(viennagrid::get_point(vertices_on_plane0[i]), PointType(geometric_dimension), axis, current_angle)) );

      for (std::size_t i = 0; i != vertices_on_no_plane.size(); ++i)
        new_vertices.push_back( viennagrid::make_vertex(output_mesh(), rotate(viennagrid::get_point(vertices_on_no_plane[i]), PointType(geometric_dimension), axis, current_angle)) );
    }

    info(1) << "New mesh has " << new_vertices.size() << " vertices (old had " << vertices.size() << ")" << std::endl;
    info(1) << "    shared vertex count = " << shared_vertex_count << std::endl;
    info(1) << "    on plane count = " << vertices_on_plane0.size() << std::endl;
    info(1) << "    on no plane count = " << vertices_on_no_plane.size() << std::endl;



    ConstElementRangeType cells( tmp, viennagrid::cell_dimension(tmp) );
    for (ConstElementIteratorType cit = cells.begin(); cit != cells.end(); ++cit)
    {
      std::vector<int> vertex_ids;
      ConstBoundaryElementRangeType vertices_on_cell(*cit, 0);

      for (ConstBoundaryElementIteratorType vcit = vertices_on_cell.begin(); vcit != vertices_on_cell.end(); ++vcit)
        vertex_ids.push_back( vertex_mapping[(*vcit).id()] );


      for (int rf = 0; rf != rotational_frequency; ++rf)
      {
        std::vector<ElementType> local_vertices(vertex_ids.size());
        for (std::size_t i = 0; i != vertex_ids.size(); ++i)
        {
          if (vertex_ids[i] < static_cast<int>(shared_vertex_count))
            local_vertices[i] = new_vertices[vertex_ids[i]];
          else
          {
            int index = rf*non_shared_vertex_count + vertex_ids[i];
            if (index >= static_cast<int>(new_vertices.size()))
              index -= rotational_frequency*non_shared_vertex_count;

            local_vertices[i] = new_vertices[index];
          }
        }

        ElementType new_cell = viennagrid::make_element( output_mesh(), (*cit).tag(), local_vertices.begin(), local_vertices.end() );
        viennagrid::copy_region_information(*cit, new_cell);
      }
    }

    set_output( "mesh", output_mesh );


    return true;
  }

}