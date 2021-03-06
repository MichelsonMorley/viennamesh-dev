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

#include "viennamesh/algorithm/line_coarsening.hpp"

#include "viennagrid/algorithm/angle.hpp"
#include "viennagrid/algorithm/norm.hpp"
#include "viennagrid/mesh/element_deletion.hpp"


namespace viennamesh
{
  template<typename MeshT, typename SegmentationT>
  void coarsen( MeshT & mesh, SegmentationT & segmentation, double angle )
  {
    typedef typename viennagrid::result_of::vertex_id<MeshT>::type VertexIDType;
    typedef typename viennagrid::result_of::vertex_handle<MeshT>::type VertexHandleType;

    typedef typename viennagrid::result_of::line<MeshT>::type LineType;
    typedef typename viennagrid::result_of::line_id<MeshT>::type LineIDType;
    typedef typename viennagrid::result_of::line_handle<MeshT>::type LineHandleType;

    typedef typename viennagrid::result_of::segment_id_range<SegmentationT, LineType>::type SegmentIDRangeType;
    typedef typename viennagrid::result_of::segment_id<SegmentationT>::type SegmentIDType;
    typedef typename viennagrid::result_of::iterator<SegmentIDRangeType>::type SegmentIDIteratorType;

    typedef std::vector< std::vector<SegmentIDType> > LineSegmentContainerType;
    typedef typename viennagrid::result_of::accessor<LineSegmentContainerType, LineType>::type LineSegmentAccessorType;

    LineSegmentContainerType line_segments_container;
    LineSegmentAccessorType line_segments(line_segments_container);

    typedef typename viennagrid::result_of::line_range<MeshT>::type LineRangeType;
    typedef typename viennagrid::result_of::iterator<LineRangeType>::type LineIteratorType;

    LineRangeType lines(mesh);
    for (LineIteratorType lit = lines.begin(); lit != lines.end(); ++lit)
    {
      SegmentIDRangeType segment_ids = viennagrid::segment_ids(segmentation, *lit);
      line_segments(*lit).resize( segment_ids.size() );
      std::copy( segment_ids.begin(), segment_ids.end(), line_segments(*lit).begin() );
    }
    segmentation.clear();


    typedef typename viennagrid::result_of::vertex_range<MeshT>::type VertexRangeType;
    typedef typename viennagrid::result_of::iterator<VertexRangeType>::type VertexIteratorType;

    VertexRangeType vertices(mesh);
    for (VertexIteratorType vit = vertices.begin(); vit != vertices.end();)
    {
      typedef typename viennagrid::result_of::coboundary_range<MeshT, viennagrid::vertex_tag, viennagrid::line_tag>::type CoboundaryVertexRangeType;
      typedef typename viennagrid::result_of::iterator<CoboundaryVertexRangeType>::type CoboundaryLineIteratorType;

      CoboundaryVertexRangeType coboundary_lines(mesh, vit.handle());
      if (coboundary_lines.size() != 2)
      {
        ++vit;
        continue;
      }

      if (line_segments(coboundary_lines[0]) != line_segments(coboundary_lines[1]))
      {
        ++vit;
        continue;
      }

      VertexHandleType middle = vit.handle();
      VertexHandleType first = viennagrid::vertices(coboundary_lines[0]).handle_at(0) == middle ?
                                viennagrid::vertices(coboundary_lines[0]).handle_at(1) :
                                viennagrid::vertices(coboundary_lines[0]).handle_at(0);
      VertexHandleType last = viennagrid::vertices(coboundary_lines[1]).handle_at(0) == middle ?
                                viennagrid::vertices(coboundary_lines[1]).handle_at(1) :
                                viennagrid::vertices(coboundary_lines[1]).handle_at(0);

      double current_angle = viennagrid::angle( viennagrid::point(mesh, first), viennagrid::point(mesh, last), viennagrid::point(mesh, middle) );
      if (current_angle > angle)
      {
        VertexIDType first_id = viennagrid::dereference_handle(mesh, first).id();
        VertexIDType last_id = viennagrid::dereference_handle(mesh, last).id();

        std::vector<SegmentIDType> segment_ids = line_segments(coboundary_lines[0]);

        viennagrid::erase_element(mesh, middle);
        LineHandleType new_line = viennagrid::make_line( mesh,
                                                          viennagrid::find(mesh, first_id).handle(),
                                                          viennagrid::find(mesh, last_id).handle() );

        LineType const & line = viennagrid::dereference_handle(mesh, new_line);
        line_segments(line) = segment_ids;

        vit = vertices.begin();
      }
      else
      {
        ++vit;
      }
    }

    for (LineIteratorType lit = lines.begin(); lit != lines.end(); ++lit)
    {
      std::vector<SegmentIDType> const & segment_ids = line_segments(*lit);
      for (typename std::vector<SegmentIDType>::const_iterator sit = segment_ids.begin(); sit != segment_ids.end(); ++sit)
      {
        viennagrid::add( segmentation.get_make_segment(*sit), *lit );
      }
    }
  }


  line_coarsening::line_coarsening() :
    input_mesh(*this, parameter_information("mesh","mesh","The input mesh, segmented line 2d mesh and segmented line 3d mesh supported")),
    angle(*this, parameter_information("angle","double","All lines which have an angle higher than this are merged"), 2.5),
    output_mesh(*this, parameter_information("mesh", "mesh", "The output mesh, same type of mesh as input mesh")) {}

  std::string line_coarsening::name() const { return "ViennaGrid Line Coarsing"; }
  std::string line_coarsening::id() const { return "line_coarsening"; }

  template<typename MeshT, typename SegmentationT>
  bool line_coarsening::generic_run()
  {
    typedef viennagrid::segmented_mesh<MeshT, SegmentationT> SegmentedMeshType;

    typedef typename viennamesh::result_of::point< viennagrid::result_of::geometric_dimension<MeshT>::value >::type PointType;
    typedef typename viennamesh::result_of::seed_point_container<PointType>::type SeedPointContainerType;

    {
      typename viennamesh::result_of::const_parameter_handle<SegmentedMeshType>::type imp = input_mesh.get<SegmentedMeshType>();
      if (imp)
      {
        output_parameter_proxy<SegmentedMeshType> omp(output_mesh);

        if (omp != imp)
          omp() = imp();

        coarsen(omp().mesh, omp().segmentation, angle());

        return true;
      }
    }

    {
//           typename viennamesh::result_of::const_parameter_handle<MeshT>::type input_mesh = get_input<MeshT>("default");
//           if (input_mesh)
//           {
//             output_parameter_proxy<MeshT> output_mesh = output_proxy<MeshT>( "default" );
//             output_mesh() = input_mesh();
//             coarsen(output_mesh(), angle);
//
//             return true;
//           }
    }

    return false;
  }

  bool line_coarsening::run_impl()
  {
    if (generic_run<viennagrid::line_2d_mesh, viennagrid::line_2d_segmentation>())
      return true;

    if (generic_run<viennagrid::line_3d_mesh, viennagrid::line_3d_segmentation>())
      return true;


    error(1) << "Input Parameter 'default' (type: mesh) is missing or of non-convertable type" << std::endl;
    return false;
  }
}
