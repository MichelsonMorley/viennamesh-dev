#include "viennamesh/algorithm/io.hpp"
#include "viennamesh/algorithm/viennagrid.hpp"
#include "viennamesh/core/algorithm_pipeline.hpp"
#include <tclap/CmdLine.h>

int main(int argc, char **argv)
{
  try
  {
    TCLAP::CmdLine cmd("Extracts a hull of a mesh", ' ', "1.0");

    TCLAP::ValueArg<std::string> log_filename("l","logfile", "Log file name (default is convert.log)", false, "convert.log", "string");
    cmd.add( log_filename );


    TCLAP::UnlabeledValueArg<std::string> pipeline_filename( "filename", "Pipeline file name", true, "", "PipelineFile"  );
    cmd.add( pipeline_filename );

    cmd.parse( argc, argv );


    viennamesh::logger().register_callback( new viennamesh::FileStreamCallback<viennamesh::FileStreamFormater>( log_filename.getValue() ) );



    pugi::xml_document pipeline_xml;
    pipeline_xml.load_file( pipeline_filename.getValue().c_str() );


    viennamesh::algorithm_pipeline pipeline;
    pipeline.from_xml( pipeline_xml );


    pipeline.run();
  }
  catch (TCLAP::ArgException &e)  // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  return 0;
}