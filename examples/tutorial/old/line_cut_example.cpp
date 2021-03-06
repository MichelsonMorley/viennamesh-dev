#include <sstream>

#include "viennamesh/viennamesh.hpp"


int main()
{
  viennamesh::algorithm_handle reader( new viennamesh::io::mesh_reader() );
  reader->set_input( "filename", "/export/florian/work/projects/2014_01 Markus Mesh/imec44/coarsed_imec44_hull_main.pvd" );
  reader->run();

  viennamesh::algorithm_handle project( new viennamesh::project::algorithm() );
  project->set_default_source(reader);
  project->set_input( "target_dimension", 2 );
  project->run();

  viennamesh::algorithm_handle line_cut( new viennamesh::line_cut::algorithm() );
  line_cut->set_default_source(project);
  line_cut->set_input( "pt", viennamesh::point_2d(0,0) );
  line_cut->set_input( "dir", viennamesh::point_2d(0,1) );
  line_cut->run();

  viennamesh::algorithm_handle writer( new viennamesh::io::mesh_writer() );
  writer->set_default_source(line_cut);
  writer->set_input( "filename", "bla.vtu");
  writer->run();
}
