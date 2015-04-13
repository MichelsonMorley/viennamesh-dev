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

#include "mesh_reader.hpp"

#include <string>
#include <vector>
#include <boost/algorithm/string/split.hpp>

#include "viennagridpp/io/vtk_reader.hpp"
#include "viennagridpp/io/netgen_reader.hpp"
#include "viennagridpp/io/tetgen_poly_reader.hpp"
// #include "viennagrid/io/bnd_reader.hpp"
// #include "viennagrid/io/neper_tess_reader.hpp"
#include "viennagridpp/io/stl_reader.hpp"
#include "viennagridpp/io/gts_deva_reader.hpp"
#include "viennagridpp/io/dfise_text_reader.hpp"


// #include "viennamesh/algorithm/io/silvaco_str_reader.hpp"

#include "viennameshpp/core.hpp"

namespace viennamesh
{
  mesh_reader::mesh_reader() {}
  std::string mesh_reader::name() { return "mesh_reader"; }


//   template<int GeometricDimensionV>
//   bool mesh_reader::read_seed_points( pugi::xml_document const & xml )
//   {
//     typedef typename viennamesh::result_of::point<GeometricDimensionV>::type PointType;
//     typedef typename viennamesh::result_of::seed_point_container<PointType>::type SeedPointContainerType;
//
//     output_parameter_proxy<SeedPointContainerType> ospp(output_seed_points);
//
//     pugi::xpath_node_set xml_segments = xml.select_nodes( "/mesh/segment" );
//     for (pugi::xpath_node_set::const_iterator xsit = xml_segments.begin(); xsit != xml_segments.end(); ++xsit)
//     {
//       int segment_id;
//       pugi::xpath_node_set xml_segment_id = xsit->node().select_nodes( "/id" );
//       if (!xml_segment_id.empty())
//         segment_id = atoi( xml_segment_id.first().node().text().get() );
//       else
//         continue;
//
//       pugi::xpath_node_set xml_seed_points = xsit->node().select_nodes( "/seed_point" );
//       for (pugi::xpath_node_set::const_iterator xspit = xml_seed_points.begin(); xspit != xml_seed_points.end(); ++xspit)
//       {
//         std::stringstream ss( xspit->node().text().get() );
//         PointType point;
//         for (std::size_t i = 0; i < point.size(); ++i)
//           ss >> point[i];
//
//         info(5) << "Found seed point for segment " << segment_id << ": " << point << std::endl;
//
//         ospp().push_back( std::make_pair(point, segment_id) );
//       }
//     }
//
//     return true;
//   }




//   bool mesh_reader::read_vmesh( std::string const & filename )
//   {
//     int geometric_dimension = -1;
//     std::string mesh_filename;
//
//     pugi::xml_document xml;
//     xml.load_file( filename.c_str() );
//
//     {
//       pugi::xpath_node_set nodes = xml.select_nodes( "/mesh/dimension" );
//       if (!nodes.empty())
//       {
//         geometric_dimension = atoi( nodes.first().node().text().get() );
//         info(5) << "geometric dimension = " << geometric_dimension << std::endl;
//       }
//     }
//
//
//     {
//       pugi::xpath_node_set nodes = xml.select_nodes( "/mesh/file" );
//       if (!nodes.empty())
//       {
//         mesh_filename = nodes.first().node().text().get();
//         info(5) << "vmesh inner mesh filename = " << mesh_filename << std::endl;
//       }
//     }
//
//     FileType file_type = from_filename( mesh_filename );
//
//     load(mesh_filename, file_type);
//
//     if (geometric_dimension == 1)
//       read_seed_points<1>(xml);
//     else if (geometric_dimension == 2)
//       read_seed_points<2>(xml);
//     else if (geometric_dimension == 3)
//       read_seed_points<3>(xml);
//
//     return true;
//   }



  bool mesh_reader::load( std::string const & filename, FileType filetype )
  {
    std::string path = extract_path( filename );

    info(1) << "Reading mesh from file \"" << filename << "\"" << std::endl;

    viennagrid::mesh_t mesh;
    mesh_handle output_mesh = make_data<mesh_handle>();
    output_mesh.set(mesh);
    bool success = false;

    switch (filetype)
    {
//     case SYNOPSIS_BND:
//       {
//         info(5) << "Found .bnd extension, using ViennaGrid BND Reader" << std::endl;
//         typedef viennagrid::mesh_t MeshType;
//
//         output_parameter_proxy<MeshType> omp(output_mesh);
//
//         viennagrid::io::bnd_reader reader;
//         reader(omp().mesh, omp().segmentation, filename);
//
//         return true;
//       }
    case NETGEN_MESH:
      {
        info(5) << "Found .mesh extension, using ViennaGrid Netgen Reader" << std::endl;

        viennagrid::io::netgen_reader reader;
        reader(mesh, filename);

        success = true;
        break;
      }
    case TETGEN_POLY:
      {
        info(5) << "Found .poly extension, using ViennaGrid Tetgen poly Reader" << std::endl;

        point_container_t hole_points;
        seed_point_container_t seed_points;

        viennagrid::io::tetgen_poly_reader reader;
        reader(mesh, filename, hole_points, seed_points);

        if (!hole_points.empty())
        {
          point_handle output_hole_points = make_data<point_t>();
          output_hole_points.set( hole_points );
          set_output("hole_points", output_hole_points);
        }

        if (!seed_points.empty())
        {
          seed_point_handle output_seed_points = make_data<seed_point_t>();
          output_seed_points.set( seed_points );
          set_output("seed_points", output_seed_points);
        }

        success = true;
        break;
      }


//     case NEPER_TESS:
//       {
//         info(5) << "Found .tess extension, using ViennaGrid Neper tess Reader" << std::endl;
//         typedef viennagrid::brep_3d_mesh MeshType;
//
//         output_parameter_proxy<MeshType> omp(output_mesh);
//         output_parameter_proxy<seed_point_3d_container> ospp(output_seed_points);
//
//         seed_point_3d_container seed_points;
//
//         viennagrid::io::neper_tess_reader reader;
//         reader(omp(), filename, seed_points);
//
//         if (!seed_points.empty())
//         {
//           info(1) << "Found seed points (" << seed_points.size() << ")" << std::endl;
//           ospp() = seed_points;
//         }
//         else
//           unset_output("seed_points");
//
//
//         return true;
//       }

    case STL:
    case STL_ASCII:
    case STL_BINARY:
      {
        info(5) << "Found .stl extension, using ViennaGrid STL Reader" << std::endl;

        data_handle<double> vertex_tolerance = get_input<double>("vertex_tolerance");

        viennagrid::io::stl_reader<> reader;
        if (vertex_tolerance.valid())
          reader = viennagrid::io::stl_reader<>( vertex_tolerance() );

        if (filetype == STL)
          reader(mesh, filename);
        else if (filetype == STL_ASCII)
          reader.read_ascii(mesh, filename);
        else if (filetype == STL_BINARY)
          reader.read_binary(mesh, filename);

        success = true;
        break;
      }

//     case SILVACO_STR:
//       {
//         info(5) << "Found .str extension, using ViennaGrid Silvaco str Reader" << std::endl;
//         typedef viennagrid::triangular_3d_mesh MeshType;
//
//         output_parameter_proxy<MeshType> omp(output_mesh);
//
//         viennagrid::io::silvaco_str_reader reader;
//         reader(omp(), filename);
//
//         return true;
//       }

    case GTS_DEVA:
      {
        info(5) << "Found .deva extension, using ViennaMesh GTS deva Reader" << std::endl;

        data_handle<bool> input_load_geometry = get_input<bool>("load_geometry");
        bool load_geometry = false;

        if (input_load_geometry.valid())
          load_geometry = input_load_geometry();

        viennagrid::io::gts_deva_reader reader;
        reader(mesh, filename, load_geometry);

        success = true;
        break;
      }

    case VTK:
      {
        info(5) << "Found .vtu/.pvd extension, using ViennaGrid VTK Reader" << std::endl;

        viennagrid::io::vtk_reader<viennagrid::mesh_t> reader;

        data_handle<bool> use_local_points = get_input<bool>("use_local_points");
        if (use_local_points.valid())
          reader.set_use_local_points( use_local_points() );

        reader(mesh, filename);


        std::vector<viennagrid::quantity_field> quantity_fields = reader.quantity_fields();
        if (!quantity_fields.empty())
        {
          quantity_field_handle output_quantity_fields = make_data<viennagrid::quantity_field>();
          output_quantity_fields.set(quantity_fields);
          set_output( "quantities", output_quantity_fields );
        }

        success = true;
        break;
      }
    case GRD:
      {
        try
        {
          viennagrid::io::dfise_text_reader reader(filename);

          std::vector<viennagrid::quantity_field> quantity_fields;

          data_handle<viennamesh_string> datafiles = get_input<std::string>("datafiles");

          if (datafiles.valid())
          {
            std::vector<std::string> split_datafiles;
            std::string tmp_datafiles = datafiles();
            boost::algorithm::split(split_datafiles, tmp_datafiles, boost::is_any_of(","));
            for (unsigned int i = 0; i < split_datafiles.size(); ++i)
            {
            // future use:
            //  reader.read_dataset(datafiles(i));
              reader.read_dataset(split_datafiles[i]);
            }
          }

          data_handle<bool> extrude_contacts = get_input<bool>("extrude_contacts");
          //TODO make this uniform with TDR reader TODO
          reader.to_viennagrid( output_mesh(), quantity_fields, extrude_contacts.valid() ? extrude_contacts() : true );

          quantity_field_handle output_quantity_fields = make_data<viennagrid::quantity_field>();
          output_quantity_fields.set(quantity_fields);
          set_output( "quantities", output_quantity_fields );

          success = true;
        }
        catch(viennagrid::io::dfise_text_reader::error const & e)
        {
          error(1) << "GRID reader: got error: " << e.what() << std::endl;
        }
        break;
      }
    default:
      {
        error(1) << "Unsupported extension: " << lexical_cast<std::string>(filetype) << std::endl;
        return false;
      }
    }

    if (success)
    {
      info(1) << "Successfully read a mesh" << std::endl;
      info(1) << "  Geometric dimension = " << viennagrid::geometric_dimension(mesh) << std::endl;
      info(1) << "  Cell dimension = " << viennagrid::cell_dimension(mesh) << std::endl;
      info(1) << "  Vertex count =  " << viennagrid::vertices(mesh).size() << std::endl;
      info(1) << "  Cell count = " << viennagrid::cells(mesh).size() << std::endl;

      set_output("mesh", output_mesh);
    }

    return success;
  }




  bool mesh_reader::run(viennamesh::algorithm_handle &)
  {
    string_handle filename = get_required_input<string_handle>("filename");
    string_handle filetype = get_input<string_handle>("filetype");

    FileType ft;
    if (filetype.valid())
      ft = lexical_cast<FileType>( filetype() );
    else
      ft = from_filename( filename() );
    info(1) << "Using file type " << lexical_cast<std::string>(ft) << std::endl;

    std::string path = base_path();

    if (!path.empty())
    {
      info(1) << "Using base path: " << path << std::endl;
      return load( path + "/" + filename(), ft);
    }
    else
      return load(filename(), ft);
  }

}
