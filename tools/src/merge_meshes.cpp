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
#include <tclap/CmdLine.h>

int main(int argc, char **argv)
{
  try
  {
    TCLAP::CmdLine cmd("Merge meshes", ' ', "1.0");

    TCLAP::ValueArg<std::string> log_filename("l","logfile", "Log file name (default is convert.log)", false, "convert.log", "string");
    cmd.add( log_filename );

//     TCLAP::ValueArg<std::string> input_filetype("i","input0type", "Input file type. Can be\nauto - ViennaMesh automatically detects the file format (default)\nvtk - for VTK files\nmesh - for Netgen .mesh files\npoly - for Tetgen .poly files\ndeva - for GTS deva files", false, "auto", "string");
//     cmd.add( input_filetype );
//
//     TCLAP::ValueArg<std::string> output_filetype("o","outputtype", "Output file type. Can be\nauto - ViennaMesh automatically detects the file format (default)\nvtk - for VTK files\nvmesh - for Vienna vmesh files", false, "auto", "string");
//     cmd.add( output_filetype );


    TCLAP::UnlabeledValueArg<std::string> input0_filename( "input0-filename", "Input file name", true, "", "InputFile"  );
    cmd.add( input0_filename );

    TCLAP::UnlabeledValueArg<std::string> input1_filename( "input1-filename", "Input file name", true, "", "InputFile"  );
    cmd.add( input1_filename );

    TCLAP::UnlabeledValueArg<std::string> output_filename( "output-filename", "Output file name", true, "", "OutputFile"  );
    cmd.add( output_filename );

    cmd.parse( argc, argv );


    viennamesh::logger().register_callback( new viennamesh::FileStreamCallback<viennamesh::FileStreamFormater>( log_filename.getValue() ) );

    viennamesh::algorithm_handle reader0( new viennamesh::io::mesh_reader() );
    reader0->set_input( "filename", input0_filename.getValue() );
    reader0->run();

    viennamesh::algorithm_handle reader1( new viennamesh::io::mesh_reader() );
    reader1->set_input( "filename", input1_filename.getValue() );
    reader1->run();


    viennamesh::algorithm_handle merge_meshes( new viennamesh::merge_meshes() );
    merge_meshes->set_input( "mesh0", reader0->get_output("mesh") );
    merge_meshes->set_input( "mesh1", reader1->get_output("mesh") );
    merge_meshes->run();

    viennamesh::algorithm_handle writer( new viennamesh::io::mesh_writer() );
    writer->set_input( "default", merge_meshes->get_output("mesh") );
    writer->set_input( "filename", output_filename.getValue() );
//     if (output_filetype.isSet() && (output_filetype.getValue() != "auto"))
//       writer->set_input( "file_type", output_filetype.getValue() );
    writer->run();
  }
  catch (TCLAP::ArgException &e)  // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  return 0;
}
