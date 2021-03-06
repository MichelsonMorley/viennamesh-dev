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

#include "viennamesh/viennamesh.hpp"
#include "viennamesh/core/sizing_function.hpp"
#include <tclap/CmdLine.h>

int main(int argc, char **argv)
{
  try
  {
    TCLAP::CmdLine cmd("Uses Triangle to generate a volume mesh out of a boundary representation geometry", ' ', "1.0");

    TCLAP::ValueArg<std::string> log_filename("l","logfile", "Log file name (default is generate_triangle.log)", false, "generate_triangle.log", "string");
    cmd.add( log_filename );


    TCLAP::ValueArg<std::string> input_filetype("","input_filetype", "Input file type. Can be\nauto - ViennaMesh automatically detects the file format (default)\nvtk - for VTK files\nmesh - for Netgen .mesh files\npoly - for Tetgen .poly files\ndeva - for GTS deva files", false, "auto", "string");
    cmd.add( input_filetype );

    TCLAP::ValueArg<std::string> output_filetype("","output_filetype", "Output file type. Can be\nauto - ViennaMesh automatically detects the file format (default)\nvtk - for VTK files\nvmesh - for Vienna vmesh files", false, "auto", "string");
    cmd.add( output_filetype );

    TCLAP::ValueArg<std::string> option_string("s","option_string", "Option string for Triangle", false, "", "string");
    cmd.add( option_string );

    TCLAP::ValueArg<double> cell_size("c","cell_size", "Maximum cell size", false, 0.0, "double");
    cmd.add( cell_size );

    TCLAP::ValueArg<double> min_angle("a","min_angle", "Minimum angle", false, 0.0, "double");
    cmd.add( min_angle );

    TCLAP::SwitchArg delaunay("d","delaunay","Use delaunay meshing", false);
    cmd.add( delaunay );

    TCLAP::SwitchArg dont_use_logger("","dont_use_logger","Don't use logger for Tetgen output", false);
    cmd.add( dont_use_logger );

    TCLAP::ValueArg<std::string> sizing_function_filename("f","sizing_function", "Filename for sizing function", false, "", "string");
    cmd.add( sizing_function_filename );


    TCLAP::UnlabeledValueArg<std::string> input_filename( "input_filename", "Input file name", true, "", "InputFile"  );
    cmd.add( input_filename );

    TCLAP::UnlabeledValueArg<std::string> output_filename( "output_filename", "Output file name", true, "", "OutputFile"  );
    cmd.add( output_filename );


    cmd.parse( argc, argv );


    viennamesh::logger().register_callback( new viennamesh::FileStreamCallback<viennamesh::FileStreamFormater>( log_filename.getValue() ) );

    viennamesh::algorithm_handle reader( new viennamesh::io::mesh_reader() );
    reader->set_input( "filename", input_filename.getValue() );
    if (input_filetype.isSet() && (input_filetype.getValue() != "auto"))
      reader->set_input( "file_type", input_filetype.getValue() );
    reader->run();



    viennamesh::algorithm_handle mesher( new viennamesh::triangle::make_mesh() );

    if (sizing_function_filename.isSet())
    {
      typedef viennagrid::triangular_2d_mesh MeshType;
      typedef viennagrid::triangular_2d_segmentation SegmentationType;

      typedef viennagrid::segmented_mesh<MeshType, SegmentationType> SegmentedMeshType;
      viennamesh::result_of::parameter_handle<SegmentedMeshType>::type simple_mesh = viennamesh::make_parameter<SegmentedMeshType>();

      viennamesh::algorithm_handle simple_mesher( new viennamesh::triangle::make_mesh() );
      simple_mesher->set_output( "mesh", simple_mesh() );
      simple_mesher->set_input( "mesh", reader->get_output("mesh") );
      simple_mesher->run();

      mesher->set_input( "sizing_function", viennamesh::sizing_function::from_xmlfile(sizing_function_filename.getValue(), simple_mesh) );
    }


    mesher->set_input( "mesh", reader->get_output("mesh") );
    mesher->set_input( "seed_points", reader->get_output("seed_points") );
    mesher->set_input( "hole_points", reader->get_output("hole_points") );

    if (option_string.isSet())
      mesher->set_input( "option_string", option_string.getValue() );

    if (cell_size.isSet())
      mesher->set_input( "cell_size", cell_size.getValue() );

    if (min_angle.isSet())
      mesher->set_input( "min_angle", min_angle.getValue() );

    if (delaunay.isSet() && delaunay.getValue())
      mesher->set_input( "delaunay", true );

    if (dont_use_logger.isSet() && dont_use_logger.getValue())
      mesher->set_input( "use_logger", false );

    mesher->run();


    viennamesh::algorithm_handle writer( new viennamesh::io::mesh_writer() );
    writer->set_input( "mesh", mesher->get_output("mesh") );
    writer->set_input( "filename", output_filename.getValue() );
    if (output_filetype.isSet() && (output_filetype.getValue() != "auto"))
      writer->set_input( "file_type", output_filetype.getValue() );
    writer->run();
  }
  catch (TCLAP::ArgException &e)  // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  return 0;
}
