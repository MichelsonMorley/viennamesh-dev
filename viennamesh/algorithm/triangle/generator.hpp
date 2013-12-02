#ifndef VIENNAMESH_ALGORITHM_TRIANGLE_GENERATOR_HPP
#define VIENNAMESH_ALGORITHM_TRIANGLE_GENERATOR_HPP

#include "viennamesh/core/algorithm.hpp"
#include "viennamesh/algorithm/triangle/mesh.hpp"
#include "viennamesh/algorithm/viennagrid/extract_seed_points.hpp"

namespace viennamesh
{
  namespace triangle
  {

    class algorithm : public base_algorithm
    {
    public:

      string name() const { return "Triangle 1.6 mesher"; }



      void extract_seed_points( triangle::input_segmentation const & segmentation, int num_hole_points, REAL * hole_points,
                                seed_point_2d_container & seed_points )
      {
        if (segmentation.segments.empty())
          return;

        LoggingStack stack( string("Extracting seed points from segments") );

        info(5) << "Extracting seed points from segments" << std::endl;

        string options = "zp";

        int highest_segment_id = -1;
        for (seed_point_2d_container::iterator spit = seed_points.begin(); spit != seed_points.end(); ++spit)
          highest_segment_id = std::max( highest_segment_id, spit->second );
        ++highest_segment_id;

        for ( std::list<triangle::input_mesh>::const_iterator sit = segmentation.segments.begin(); sit != segmentation.segments.end(); ++sit)
        {
          LoggingStack stack( string("Segment ") + boost::lexical_cast<string>(highest_segment_id) );

          triangulateio tmp = sit->triangle_mesh;
          triangle::output_mesh tmp_mesh;

          if (hole_points)
          {
            tmp.numberofholes = num_hole_points;
            tmp.holelist = hole_points;
          }

          char * buffer = new char[options.length()];
          std::strcpy(buffer, options.c_str());

          viennautils::StdCapture capture;
          capture.start();

          triangulate( buffer, &tmp, &tmp_mesh.triangle_mesh, NULL);

          capture.finish();
          info(5) << capture.get() << std::endl;


          viennagrid::triangular_2d_mesh viennagrid_mesh;
          viennamesh::triangle::convert( tmp_mesh, viennagrid_mesh );


          unsigned int i = seed_points.size();

          viennamesh::extract_seed_points::extract_seed_points( viennagrid_mesh, seed_points, highest_segment_id++ );

          for (; i < seed_points.size(); ++i)
            info(5) << "Found seed point: " << seed_points[i].first << std::endl;

          tmp.holelist = NULL;
        }
      }


      bool run_impl()
      {
        typedef viennagrid::segmented_mesh< triangle::input_mesh, triangle::input_segmentation > InputMeshType;
        typedef triangle::output_mesh OutputMeshType;

        viennamesh::result_of::const_parameter_handle<InputMeshType>::type input_mesh = get_required_input<InputMeshType>("default");
        output_parameter_proxy<OutputMeshType> output_mesh = output_proxy<OutputMeshType>("default");

        std::ostringstream options;
        options << "zp";

        const_double_parameter_handle min_angle = get_input<double>("min_angle");
        if (min_angle)
          options << "q" << min_angle() / M_PI * 180.0;


        const_double_parameter_handle cell_size = get_input<double>("cell_size");
        if (cell_size)
          options << "a" << cell_size();

        const_bool_parameter_handle delaunay = get_input<bool>("delaunay");
        if (delaunay && delaunay())
          options << "D";


        const_string_parameter_handle algorithm_type = get_input<string>("algorithm_type");
        if (algorithm_type)
        {
          if (algorithm_type() == "incremental_delaunay")
            options << "i";
          else if (algorithm_type() == "sweepline")
            options << "F";
          else if (algorithm_type() == "devide_and_conquer")
          {}
          else
          {
            warning(5) << "Algorithm not recognized: '" << algorithm_type() << "' supported algorithms:" << std::endl;
            warning(5) << "  'incremental_delaunay'" << std::endl;
            warning(5) << "  'sweepline'" << std::endl;
            warning(5) << "  'devide_and_conquer'" << std::endl;
          }
        }


        triangulateio tmp = input_mesh().mesh.triangle_mesh;

        REAL * tmp_regionlist = NULL;
        REAL * tmp_holelist = NULL;


        seed_point_2d_container seed_points;

        typedef viennamesh::result_of::const_parameter_handle<seed_point_2d_container>::type ConstSeedPointContainerHandle;
        ConstSeedPointContainerHandle seed_points_handle = get_input<seed_point_2d_container>("seed_points");
        if (seed_points_handle && !seed_points_handle().empty())
        {
          info(5) << "Found input seed points" << std::endl;
          seed_points = seed_points_handle();
        }

        point_2d_container hole_points;

        typedef viennamesh::result_of::const_parameter_handle<point_2d_container>::type ConstPointContainerHandle;
        ConstPointContainerHandle hole_points_handle = get_input<point_2d_container>("hole_points");
        if (hole_points_handle && !hole_points_handle().empty())
        {
          info(5) << "Found hole points" << std::endl;
          hole_points = hole_points_handle();
        }

        if (!hole_points.empty())
        {
          tmp_holelist = (REAL*)malloc( 2*sizeof(REAL)*(tmp.numberofholes+hole_points.size()) );
          memcpy( tmp_holelist, tmp.holelist, 2*sizeof(REAL)*tmp.numberofholes );

          for (std::size_t i = 0; i < hole_points.size(); ++i)
          {
            tmp_holelist[2*(tmp.numberofholes+i)+0] = hole_points[i][0];
            tmp_holelist[2*(tmp.numberofholes+i)+1] = hole_points[i][1];
          }

          tmp.numberofholes += hole_points.size();
          tmp.holelist = tmp_holelist;
        }




        bool extract_segment_seed_points = true;
        copy_input( "extract_segment_seed_points", extract_segment_seed_points );
        if (extract_segment_seed_points)
        {
          extract_seed_points( input_mesh().segmentation, tmp.numberofholes, tmp.holelist, seed_points );
        }

        if (!seed_points.empty())
        {
          tmp_regionlist = (REAL*)malloc( 4*sizeof(REAL)*(tmp.numberofregions+seed_points.size()) );
          memcpy( tmp_regionlist, tmp.regionlist, 4*sizeof(REAL)*tmp.numberofregions );

          for (unsigned int i = 0; i < seed_points.size(); ++i)
          {
            tmp_regionlist[4*(i+tmp.numberofregions)+0] = seed_points[i].first[0];
            tmp_regionlist[4*(i+tmp.numberofregions)+1] = seed_points[i].first[1];
            tmp_regionlist[4*(i+tmp.numberofregions)+2] = REAL(seed_points[i].second);
            tmp_regionlist[4*(i+tmp.numberofregions)+3] = 0;
          }

          tmp.numberofregions += seed_points.size();
          tmp.regionlist = tmp_regionlist;

          options << "A";
        }





        char * buffer = new char[options.str().length()];
        std::strcpy(buffer, options.str().c_str());

        viennautils::StdCapture capture;
        capture.start();

        triangulate( buffer, &tmp, &output_mesh().triangle_mesh, NULL);

        capture.finish();
        info(5) << capture.get() << std::endl;

        delete[] buffer;
        if (!hole_points.empty())
          free(tmp_regionlist);
        if (!seed_points.empty())
          free(tmp_holelist);

        return true;
      }

    private:
    };

  }


}

#endif