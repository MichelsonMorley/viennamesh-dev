#ifndef VIENNAMESH_ALGORITHM_IO_MESH_WRITER_HPP
#define VIENNAMESH_ALGORITHM_IO_MESH_WRITER_HPP

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

#include "viennamesh/core/algorithm.hpp"
#include "viennamesh/algorithm/io/common.hpp"

namespace viennamesh
{
  namespace io
  {

    class mesh_writer : public base_algorithm
    {
      friend struct vmesh_writer_proxy;

    public:

      mesh_writer();
      std::string name() const;
      std::string id() const;
      bool run_impl();

    private:

      dynamic_required_input_parameter_interface                    input_mesh;
      required_input_parameter_interface<std::string>               filename;
      optional_input_parameter_interface<std::string>               filetype;
      dynamic_optional_input_parameter_interface                    input_seed_points;
//       optional_input_parameter_interface<point_1d_container>        input_hole_points;
      dynamic_optional_input_parameter_interface                    quantities;



      template<typename WriterProxyT>
      bool write_all( const_parameter_handle const & mesh, std::string const & filename,
                      int geometric_dimension, std::string cell_type, bool is_segmented );

      bool write_mphtxt( const_parameter_handle const & mesh, std::string const & filename,
                         int geometric_dimension, std::string cell_type );


      template<typename WriterProxyT, typename MeshT>
      bool basic_nonsegmented_write( const_parameter_handle const & mesh, std::string const & filename );

      template<typename WriterProxyT, typename MeshT, typename SegmentationT>
      bool basic_segmented_write( const_parameter_handle const & mesh, std::string const & filename );

      template<typename WriterProxyT, typename TagT, int DimensionV>
      bool generic_nonsegmented_write( const_parameter_handle const & mesh, std::string const & filename );

      template<typename WriterProxyT, typename TagT, int DimensionV>
      bool generic_segmented_write( const_parameter_handle const & mesh, std::string const & filename );

      template<typename WriterProxyT, typename TagT, int DimensionV>
      bool generic_write( const_parameter_handle const & mesh, std::string const & filename, bool is_segmented );
    };

  }

}



#endif
