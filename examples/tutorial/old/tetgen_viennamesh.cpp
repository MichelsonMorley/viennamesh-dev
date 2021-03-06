#include <iostream>

#include "viennamesh/algorithm/tetgen_tetrahedron_mesher.hpp"

#include "viennagrid/mesh/element_creation.hpp"
#include "viennagrid/io/vtk_writer.hpp"





int main()
{
    typedef viennagrid::triangular_3d_mesh hull_mesh_type;

    typedef viennagrid::result_of::point<hull_mesh_type>::type point_type;

    typedef viennagrid::result_of::element<hull_mesh_type, viennagrid::vertex_tag>::type hull_vertex_type;
    typedef viennagrid::result_of::handle<hull_mesh_type, viennagrid::vertex_tag>::type hull_vertex_handle_type;

    typedef viennagrid::result_of::element<hull_mesh_type, viennagrid::triangle_tag>::type hull_triangle_type;


    hull_mesh_type hull_mesh;


    double s = 10.0;
    hull_vertex_handle_type vtx[8];

    vtx[0] = viennagrid::make_vertex( hull_mesh, point_type(0, 0, 0) );
    vtx[1] = viennagrid::make_vertex( hull_mesh, point_type(0, s, 0) );
    vtx[2] = viennagrid::make_vertex( hull_mesh, point_type(s, 0, 0) );
    vtx[3] = viennagrid::make_vertex( hull_mesh, point_type(s, s, 0) );

    vtx[4] = viennagrid::make_vertex( hull_mesh, point_type(0, 0, s) );
    vtx[5] = viennagrid::make_vertex( hull_mesh, point_type(0, s, s) );
    vtx[6] = viennagrid::make_vertex( hull_mesh, point_type(s, 0, s) );
    vtx[7] = viennagrid::make_vertex( hull_mesh, point_type(s, s, s) );


    viennagrid::make_triangle( hull_mesh, vtx[0], vtx[1], vtx[2] );
    viennagrid::make_triangle( hull_mesh, vtx[2], vtx[1], vtx[3] );

    viennagrid::make_triangle( hull_mesh, vtx[4], vtx[6], vtx[5] );
    viennagrid::make_triangle( hull_mesh, vtx[6], vtx[7], vtx[5] );

    viennagrid::make_triangle( hull_mesh, vtx[0], vtx[2], vtx[4] );
    viennagrid::make_triangle( hull_mesh, vtx[2], vtx[6], vtx[4] );

    viennagrid::make_triangle( hull_mesh, vtx[1], vtx[5], vtx[3] );
    viennagrid::make_triangle( hull_mesh, vtx[3], vtx[5], vtx[7] );

    viennagrid::make_triangle( hull_mesh, vtx[0], vtx[4], vtx[1] );
    viennagrid::make_triangle( hull_mesh, vtx[1], vtx[4], vtx[5] );

    viennagrid::make_triangle( hull_mesh, vtx[2], vtx[3], vtx[6] );
    viennagrid::make_triangle( hull_mesh, vtx[3], vtx[7], vtx[6] );



    viennamesh::ConstParameterSet settings;
    settings.set("cell_size", 1.0);

    viennagrid::tetrahedral_3d_mesh tet_mesh;
    viennamesh::run_algo<viennamesh::tetgen_tetrahedron_tag>(hull_mesh, tet_mesh, settings);


    viennagrid::io::vtk_writer<viennagrid::tetrahedral_3d_mesh> vtk_writer;
    vtk_writer(tet_mesh, "cube_meshed_tetgen");

}
