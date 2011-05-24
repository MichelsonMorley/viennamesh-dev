#include <mystdlib.h>
#include "incvis.hpp"


#include <myadt.hpp>
#include <meshing.hpp>
#include <csg.hpp>
#include <stlgeom.hpp>

#include <visual.hpp>


namespace netgen
{

  extern AutoPtr<Mesh> mesh;

  extern VisualSceneMesh vsmesh;


  VisualSceneSolution :: SolData :: SolData ()
    : name (0), data (0), solclass(0)
  { ; }

  VisualSceneSolution :: SolData :: ~SolData ()
  {
    delete [] name;
    delete data;
    delete solclass;
  }

  
  VisualSceneSolution :: VisualSceneSolution ()
    : VisualScene()
  {
    surfellist = 0;
    linelist = 0;
    clipplanelist = 0;
    isolinelist = 0;
    clipplane_isolinelist = 0;
    surface_vector_list = 0;
    cone_list = 0;
    isosurface_list = 0;

    fieldlineslist = 0;
    pointcurvelist = 0;

    num_fieldlineslists = 0;


    surfeltimestamp = GetTimeStamp();
    surfellinetimestamp = GetTimeStamp();
    clipplanetimestamp = GetTimeStamp();
    solutiontimestamp = GetTimeStamp();
    fieldlinestimestamp = GetTimeStamp();
    pointcurve_timestamp = GetTimeStamp();
    surface_vector_timestamp = GetTimeStamp();
    isosurface_timestamp = GetTimeStamp();
    timetimestamp = GetTimeStamp();
    AddVisualizationScene ("solution", &vssolution);
  }
  
  VisualSceneSolution :: ~VisualSceneSolution ()
  {
    ClearSolutionData();
  }

  void VisualSceneSolution :: AddSolutionData (SolData * sd)
  {
    NgLock meshlock1 (mesh->MajorMutex(), 1);

    int funcnr = -1;
    for (int i = 0; i < soldata.Size(); i++)
      {
        if (strcmp (soldata[i]->name, sd->name) == 0)
          {
            delete soldata[i];
            soldata[i] = sd;
            funcnr = i;
            break;
          }
      }

    if (funcnr == -1)
      {
        soldata.Append (sd);
        funcnr = soldata.Size()-1;
      }
    
    SolData * nsd = soldata[funcnr];

    nsd->size = 0;
    if (mesh)
      {
        switch (nsd->soltype)
          {
          case SOL_NODAL: nsd->size = mesh->GetNV(); break;
          case SOL_ELEMENT: nsd->size = mesh->GetNE(); break;
          case SOL_SURFACE_ELEMENT: nsd->size = mesh->GetNSE(); break;
          case SOL_NONCONTINUOUS: 
            {
              switch (nsd->order)
                {
                case 0: nsd->size =      mesh->GetNE(); break;
                case 1: nsd->size =  6 * mesh->GetNE(); break;
                case 2: nsd->size = 18 * mesh->GetNE(); break;
                }
              break;
            }
          case SOL_SURFACE_NONCONTINUOUS: 
            {
              switch (nsd->order)
                {
                case 0: nsd->size =     mesh->GetNSE(); break;
                case 1: nsd->size = 4 * mesh->GetNSE(); break;
                case 2: nsd->size = 9 * mesh->GetNSE(); break;
                }
              break;
            }
          }
        solutiontimestamp = NextTimeStamp();
      }
  }

  
  void VisualSceneSolution :: ClearSolutionData ()
  {
    for (int i = 0; i < soldata.Size(); i++)
      delete soldata[i];
    soldata.SetSize (0);
  }

  void VisualSceneSolution :: UpdateSolutionTimeStamp ()
  {
    solutiontimestamp = NextTimeStamp();
  }
    
  VisualSceneSolution::SolData * VisualSceneSolution :: GetSolData (int i)
  { 
    if (i >= 0 && i < soldata.Size())
      return soldata[i];
    else 
      return NULL;
  }
  



  void VisualSceneSolution :: SaveSolutionData (const char * filename) 
  {
    PrintMessage (1, "Write solution data to file ", filename);

    ofstream ost(filename);
    for (int i = 0; i < soldata.Size(); i++)
      {
        const SolData & sol = *soldata[i];
      
        ost << "solution " 
            << sol.name
            << " -size=" << sol.size 
            << " -components=" << sol.components
            << " -order=" << sol.order;
        if (sol.iscomplex)
          ost << " -complex";
      
        switch (sol.soltype)
          {
          case SOL_NODAL:
            ost << " -type=nodal"; break;
          case SOL_ELEMENT:
            ost << " -type=element"; break;
          case SOL_SURFACE_ELEMENT:
            ost << " -type=surfaceelement"; break;
          case SOL_NONCONTINUOUS:
            ost << " -type=noncontinuous"; break;
          case SOL_SURFACE_NONCONTINUOUS:
            ost << " -type=surfacenoncontinuous"; break;
          }
      
        ost << endl;
        for (int j = 0; j < sol.size; j++)
          {
            for (int k = 0; k < sol.components; k++)
              ost << sol.data[j*sol.dist+k] << " ";
            ost << "\n";
          }
      }
  }
  



  void VisualSceneSolution :: DrawScene ()
  {
    clock_t starttime, endtime;
    starttime = clock();


    if (!mesh) 
      {
        VisualScene::DrawScene();      
        return;
      }

    static NgLock mem_lock(mem_mutex);
    mem_lock.Lock();

    NgLock meshlock1 (mesh->MajorMutex(), true);
    NgLock meshlock (mesh->Mutex(), true);

    BuildScene();

    CreateTexture (numtexturecols, lineartexture, GL_MODULATE);

    glClearColor(backcolor, backcolor, backcolor, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SetLight();
    
    glPushMatrix();
    glMultMatrixf (transformationmat);

  

    glMatrixMode (GL_MODELVIEW); 
    
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    
    glPolygonOffset (1, 1);

    glEnable (GL_POLYGON_OFFSET_FILL);

    glEnable (GL_COLOR_MATERIAL);

    if (usetexture)
      {
        if (usetexture == 1)
          {
            glEnable (GL_TEXTURE_1D);
            glDisable (GL_TEXTURE_2D);
          }
        else
          {
            glEnable (GL_TEXTURE_2D);
            glDisable (GL_TEXTURE_1D);
          }

        glMatrixMode (GL_TEXTURE);
        glLoadIdentity();
        
        if (usetexture == 1)
          {
            double hmax = maxval;
            double hmin = minval;
            if (invcolor) Swap (hmax, hmin);

            if (fabs (hmax - hmin) > 1e-10) 
              glScalef (1.0 / (hmin - hmax), 0, 0);
            else
              glScalef (1e10, 0, 0);
            
            glTranslatef (-hmax, 0, 0);
          }
        else
          {
            glTranslatef (0.5, 0, 0);
            glRotatef(360 * vssolution.time, 0, 0, -1);
            if (fabs (maxval) > 1e-10)
              glScalef(0.5/maxval, 0.5/maxval, 0.5/maxval);
            else
              glScalef (1e10, 1e10, 1e10);
          }
        glMatrixMode (GL_MODELVIEW);
      }


    if (vispar.drawfilledtrigs || vispar.drawtetsdomain > 0 || vispar.drawdomainsurf > 0)
      {
        SetClippingPlane ();
        
        glCallList (surfellist);
        glCallList (surface_vector_list);
      
        glDisable(GL_CLIP_PLANE0);
      }


    if (showclipsolution)
      glCallList (clipplanelist);



    glMatrixMode (GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode (GL_MODELVIEW);








    if (draw_fieldlines)
      {
	SetClippingPlane();
        if (num_fieldlineslists <= 1)
          glCallList (fieldlineslist);
        else
          {  // animated
            int start = int (time / 10 * num_fieldlineslists);
            for (int ln = 0; ln < 10; ln++)
              {
                int nr = fieldlineslist + (start + ln) % num_fieldlineslists;
                glCallList (nr);
              }
          }
        glDisable(GL_CLIP_PLANE0);
      }

    if(drawpointcurves)
      {
	glCallList(pointcurvelist);
      }

    if (usetexture)
      {
        glDisable (GL_TEXTURE_1D);
        glDisable (GL_TEXTURE_2D);
      }


    glDisable (GL_POLYGON_OFFSET_FILL);
    glDisable (GL_COLOR_MATERIAL);



    if (draw_isosurface)
      glCallList (isosurface_list);
    
    
    GLfloat matcol0[] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matcol0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, matcol0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matcol0);
    
    glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth (1.0f);
    glColor3f (0.0f, 0.0f, 0.0f);
    glDisable (GL_LINE_SMOOTH);


    if (vispar.drawoutline && !numisolines)
      {
        SetClippingPlane ();
        glCallList (linelist);
        glDisable(GL_CLIP_PLANE0);
      }

    if (numisolines)
      {
        SetClippingPlane ();
        glCallList (isolinelist);

        glDisable(GL_CLIP_PLANE0);
        glCallList (clipplane_isolinelist);
      }

    glPopMatrix();
    
    glDisable(GL_CLIP_PLANE0);
    DrawColorBar (minval, maxval, logscale, lineartexture);
    
    if (vispar.drawcoordinatecross)
      DrawCoordinateCross ();
    DrawNetgenLogo ();
    
    glFinish();  

    
    // delete lock;
    mem_lock.UnLock();


    endtime = clock();
    //    cout << 1.0 / (double(endtime - starttime)/CLOCKS_PER_SEC) << " frames/sec" << endl;
  }
  


  void VisualSceneSolution :: RealVec3d (const double * values, Vec3d & v, 
                                         bool iscomplex, bool imag)
  {
    if (!iscomplex)
      {
        v.X() = values[0];
        v.Y() = values[1];
        v.Z() = values[2];
      }
    else
      {
        if (!imag)
          {
            v.X() = values[0];
            v.Y() = values[2];
            v.Z() = values[4];
          }
        else
          {
            v.X() = values[1];
            v.Y() = values[3];
            v.Z() = values[5];
          }
      }
  }


  void VisualSceneSolution :: RealVec3d (const double * values, Vec3d & v, 
                                         bool iscomplex, double phaser, double phasei)
  {
    if (!iscomplex)
      {
        v.X() = values[0];
        v.Y() = values[1];
        v.Z() = values[2];
      }
    else
      {
        for (int i = 0; i < 3; i++)
          v.X(i+1) = phaser * values[2*i] + phasei * values[2*i+1];
      }
  }


  

  void VisualSceneSolution :: BuildScene (int zoomall)
  {
    
    if (!mesh)
      {
        VisualScene::BuildScene (zoomall);
        return;
      }
    
    if (!cone_list)
      {
        cone_list = glGenLists (1);
        glNewList (cone_list, GL_COMPILE);
        DrawCone (Point<3> (0,0,0), Point<3> (0,0,1), 0.4);
        glEndList();
      }

    
    vispar.colormeshsize = 1;
    
    // recalc clipping plane
    SetClippingPlane ();
    glDisable(GL_CLIP_PLANE0);
    
    
    SolData * sol = NULL;
    SolData * vsol = NULL;
  
    if (scalfunction != -1) 
      sol = soldata[scalfunction];
    if (vecfunction != -1)
      vsol = soldata[vecfunction];

    if (mesh->GetTimeStamp () > solutiontimestamp)
      {
        sol = NULL;
        vsol = NULL;
      }


    if (sol && sol->solclass) sol->solclass->SetMultiDimComponent (multidimcomponent);
    if (vsol && vsol->solclass) vsol->solclass->SetMultiDimComponent (multidimcomponent);

    if (!autoscale || scalfunction == -1)
      {
        minval = mminval;
        maxval = mmaxval;
      }
    else
      {
        if (mesh->GetTimeStamp () > surfeltimestamp ||
            vispar.clipplanetimestamp > clipplanetimestamp ||
            solutiontimestamp > surfeltimestamp)
          {
            GetMinMax (scalfunction, scalcomp, minval, maxval);
          }
      }
 

    if (mesh->GetTimeStamp() > surfeltimestamp ||
        solutiontimestamp > surfeltimestamp || 
        zoomall)
      {
        if (mesh->GetTimeStamp() > surfeltimestamp ||
            zoomall)
          {
            // mesh has changed
          
            Point3d pmin, pmax;
            static double oldrad = 0;
          
            mesh->GetBox (pmin, pmax, -1);
            center = Center (pmin, pmax);
            rad = 0.5 * Dist (pmin, pmax);
          
            glEnable (GL_NORMALIZE);
          
            if (rad > 1.5 * oldrad ||
                mesh->GetMajorTimeStamp() > surfeltimestamp ||
                zoomall)
              {
                CalcTransformationMatrices();
                oldrad = rad;
              }
          }

        if (surfellist)
          glDeleteLists (surfellist, 1);
      
        surfellist = glGenLists (1);
        glNewList (surfellist, GL_COMPILE);
      
        DrawSurfaceElements();
      
        glEndList ();
      
        surfeltimestamp = max2 (solutiontimestamp, mesh->GetTimeStamp());
      }


    if (mesh->GetTimeStamp() > surfellinetimestamp ||
        solutiontimestamp > surfellinetimestamp || 
        zoomall)
      {
        if (linelist)
          glDeleteLists (linelist, 1);
      
        linelist = glGenLists (1);
        glNewList (linelist, GL_COMPILE);
      
        DrawSurfaceElementLines();
      
        glEndList ();
      
        surfellinetimestamp = max2 (solutiontimestamp, mesh->GetTimeStamp());
      }

  

    if (mesh->GetTimeStamp() > surface_vector_timestamp ||
        solutiontimestamp > surface_vector_timestamp ||
        zoomall)
      {
        if (surface_vector_list)
          glDeleteLists (surface_vector_list, 1);
      
        surface_vector_list = glGenLists (1);
        glNewList (surface_vector_list, GL_COMPILE);

        glEnable (GL_NORMALIZE);
        DrawSurfaceVectors();

        glEndList ();

        surface_vector_timestamp = 
          max2 (mesh->GetTimeStamp(), solutiontimestamp);
      }


    if (clipplanetimestamp < vispar.clipplanetimestamp ||
        clipplanetimestamp < solutiontimestamp)
      {

        //      cout << "clipsolution = " << clipsolution << endl;
        if (vispar.clipenable && clipsolution)      
          {
            // lock->UnLock();
            NgLock mlock (mesh->Mutex(), 0);
            mlock.UnLock(); 
            mesh->BuildElementSearchTree();
            mlock.Lock();

            // lock->Lock();
          }

        if (clipplanelist)
          glDeleteLists (clipplanelist, 1);
      

        clipplanelist = glGenLists (1);
        glNewList (clipplanelist, GL_COMPILE);
      
        if (vispar.clipenable && clipsolution == 1 && sol)
          {
            glDisable(GL_CLIP_PLANE0);
          
            ARRAY<ClipPlaneTrig> cpt;
            ARRAY<ClipPlanePoint> pts;
            GetClippingPlaneTrigs (cpt, pts);
          
            glNormal3d (-clipplane[0], -clipplane[1], -clipplane[2]);
            glColor3d (1.0, 1.0, 1.0);

            glDisable (GL_TEXTURE_1D);
            glDisable (GL_TEXTURE_2D);

            if (usetexture == 1)
              glEnable (GL_TEXTURE_1D);
            if (usetexture == 2)
              glEnable (GL_TEXTURE_2D);

            glBegin (GL_TRIANGLES);
            //            for (int i = 0; i < cpt.Size(); i++)
            // DrawClipPlaneTrig (sol, scalcomp, cpt[i], 0);
            DrawClipPlaneTrigs (sol, scalcomp, cpt, pts);
            glEnd();

            glEnable(GL_CLIP_PLANE0);
          }
      
      
        if (vispar.clipenable && clipsolution == 2 && vsol)
          {
            if (autoscale)
              GetMinMax (vecfunction, 0, minval, maxval);


            bool drawelem;
            ARRAY<ClipPlanePoint> cpp;
            GetClippingPlaneGrid (cpp);

            for (int i = 0; i < cpp.Size(); i++)
              {
                const ClipPlanePoint & p = cpp[i];
                double values[6];
                Vec3d v;

                drawelem = GetValues (vsol, p.elnr, p.lami(0), p.lami(1), p.lami(2), values);
                RealVec3d (values, v, vsol->iscomplex, imag_part);

                double val = v.Length();

                // "drawelem": added 07.04.2004 (FB)
                if (drawelem && val > 1e-10 * maxval)
                  {
                    v *= (rad / val / gridsize * 0.5);
                  
                    SetOpenGlColor  (val, minval, maxval, logscale);
                    DrawCone (p.p, p.p+v, rad / gridsize * 0.2);
                  }
              }
          }

        glEndList ();
      }




    if (mesh->GetTimeStamp() > isosurface_timestamp ||
        solutiontimestamp > isosurface_timestamp ||
        zoomall)
      {
        if (isosurface_list)
          glDeleteLists (isosurface_list, 1);
      
        isosurface_list = glGenLists (1);
        glNewList (isosurface_list, GL_COMPILE);

        glEnable (GL_NORMALIZE);
        DrawIsoSurface(sol, vsol, scalcomp);

        glEndList ();

        isosurface_timestamp = 
          max2 (mesh->GetTimeStamp(), solutiontimestamp);
      }

    if(mesh->GetTimeStamp() > pointcurve_timestamp ||
       solutiontimestamp > pointcurve_timestamp)
      {
	if(pointcurvelist)
	  glDeleteLists(pointcurvelist,1);
	
		
	if(mesh->GetNumPointCurves() > 0)
	  {
	    pointcurvelist = glGenLists(1);
	    glNewList(pointcurvelist,GL_COMPILE);
	    //glColor3f (1.0f, 0.f, 0.f);
	    
	    for(int i=0; i<mesh->GetNumPointCurves(); i++)
	      {
		double red,green,blue;
		mesh->GetPointCurveColor(i,red,green,blue);
		glColor3f (red, green, blue);
		for(int j=0; j<mesh->GetNumPointsOfPointCurve(i)-1; j++)
		  {
		    DrawCylinder(mesh->GetPointCurvePoint(i,j),
				 mesh->GetPointCurvePoint(i,j+1),
				 0.001*rad);
		  }
	      }
	    glEndList();
	  }
	
      }


    if (
        numisolines && 
        (clipplanetimestamp < vispar.clipplanetimestamp ||
         clipplanetimestamp < solutiontimestamp) 
        )
      {
        if (isolinelist) glDeleteLists (isolinelist, 1);
      
        isolinelist = glGenLists (1);
        glNewList (isolinelist, GL_COMPILE);

        Point<3> points[1100];
        double values[1100];
      
        int nse = mesh->GetNSE();

        CurvedElements & curv = mesh->GetCurvedElements();

        if (sol)
          {
            glBegin (GL_LINES);
          
            for (SurfaceElementIndex sei = 0; sei < nse; sei++)
              {
                const Element2d & el = (*mesh)[sei];

#ifdef PARALLEL
		// parallel visualization --> dont draw ghost elements
		if ( el . IsGhost() ) continue;
#endif
                bool curved = curv.IsHighOrder(); //  && curv.IsSurfaceElementCurved(sei);
              
                if (el.GetType() == TRIG || el.GetType() == TRIG6)
                  {
                    Point<3> lp1, lp2, lp3;
                    if (!curved)
                      {
                        GetPointDeformation (el[0]-1, lp1);
                        GetPointDeformation (el[1]-1, lp2);
                        GetPointDeformation (el[2]-1, lp3);
                      }
                  
                    int n = 1 << subdivisions;
                    int ii = 0;
                    int ix, iy;
                    for (iy = 0; iy <= n; iy++)
                      for (ix = 0; ix <= n-iy; ix++)
                        {
                          double x = double(ix) / n;
                          double y = double(iy) / n;
                        
                          // TODO: consider return value (bool: draw/don't draw element)
                          GetSurfValue (sol, sei, x, y, scalcomp, values[ii]);
                          Point<2> xref(x,y);
                        
                          if (curved)
                            mesh->GetCurvedElements().
                              CalcSurfaceTransformation (xref, sei, points[ii]);
                          else
                            points[ii] = lp3 + x * (lp1-lp3) + y * (lp2-lp3);
                        
                          if (deform)
                            {
                              points[ii] += GetSurfDeformation (sei, x, y);
                            }
                          ii++;
                        }
                  
                    ii = 0;
                    for (iy = 0; iy < n; iy++, ii++)
                      for (ix = 0; ix < n-iy; ix++, ii++)
                        {
                          int index[] = { ii, ii+1, ii+n-iy+1,
                                          ii+1, ii+n-iy+2, ii+n-iy+1 };
                        
                          DrawIsoLines (points[index[0]], points[index[1]], points[index[2]],
                                        values[index[0]], values[index[1]], values[index[2]],
                                        minval, maxval, numisolines);
                          if (ix < n-iy-1) 
                            DrawIsoLines (points[index[3]], points[index[4]], points[index[5]],
                                          values[index[3]], values[index[4]], values[index[5]],
                                          minval, maxval, numisolines);
                        }    
                  }
              
              
                if (el.GetType() == QUAD || el.GetType() == QUAD6 || el.GetType() == QUAD8 )
                  {
                    Point<3> lpi[4];
                    Vec<3> vx, vy, vtwist, def;
                    if (!curved)
                      {
                        for (int j = 0; j < 4; j++)
                          GetPointDeformation (el[j]-1, lpi[j]);
                        vx = lpi[1]-lpi[0];
                        vy = lpi[3]-lpi[0];
                        vtwist = (lpi[0]-lpi[1]) + (lpi[2]-lpi[3]);
                      }

                    int n = 1 << subdivisions;
                    int ix, iy, ii = 0;
                    for (iy = 0; iy <= n; iy++)
                      for (ix = 0; ix <= n; ix++, ii++)
                        {
                          double x = double(ix) / n;
                          double y = double(iy) / n;
                        
                          // TODO: consider return value (bool: draw/don't draw element)
                          GetSurfValue (sol, sei, x, y, scalcomp, values[ii]);
                          Point<2> xref(x,y);
                        
                          if (curved)
                            mesh->GetCurvedElements().
                              CalcSurfaceTransformation (xref, sei, points[ii]);
                          else
                            points[ii] = lpi[0] + x * vx + y * vy + x*y * vtwist;
                        
                          if (deform)
                            points[ii] += GetSurfDeformation (sei, x, y);
                        }
                  
                    ii = 0;
                    for (iy = 0; iy < n; iy++, ii++)
                      for (ix = 0; ix < n; ix++, ii++)
                        {
                          DrawIsoLines (points[ii], points[ii+1], points[ii+n+1],
                                        values[ii], values[ii+1], values[ii+n+1],
                                        minval, maxval, numisolines);
                          DrawIsoLines (points[ii+1], points[ii+n+2], points[ii+n+1],
                                        values[ii+1], values[ii+n+2], values[ii+n+1],
                                        minval, maxval, numisolines);
                        }       
                  }
              }
            glEnd();
          }
        glEndList ();

        if (clipplane_isolinelist) glDeleteLists (clipplane_isolinelist, 1);
            
        if (vispar.clipenable && clipsolution == 1 && sol)
          {
            clipplane_isolinelist = glGenLists (1);
            glNewList (clipplane_isolinelist, GL_COMPILE);

            ARRAY<ClipPlaneTrig> cpt;
            ARRAY<ClipPlanePoint> pts;
            GetClippingPlaneTrigs (cpt, pts);
            bool drawelem;
          
            glNormal3d (-clipplane[0], -clipplane[1], -clipplane[2]);
          
            if (numisolines)
              for (int i = 0; i < cpt.Size(); i++)
                {
                  const ClipPlaneTrig & trig = cpt[i];
                  double vali[3];
                  for (int j = 0; j < 3; j++)
                    {
                      Point<3> lami = pts[trig.points[j].pnr].lami;
                      drawelem = GetValue (sol, trig.elnr, lami(0), lami(1), lami(2),
                                           scalcomp, vali[j]);
                    }
                  if ( drawelem )
                    DrawIsoLines (pts[trig.points[0].pnr].p,
                                  pts[trig.points[1].pnr].p,
                                  pts[trig.points[2].pnr].p,
                                  // trig.points[1].p,
                                  // trig.points[2].p,
                                  vali[0], vali[1], vali[2], minval, maxval, numisolines);
                }
            glEndList ();
          }
        glEnd();
      

      }
  
    clipplanetimestamp = max2 (vispar.clipplanetimestamp, solutiontimestamp);
  }
  


  void  VisualSceneSolution :: DrawSurfaceElements ()
  {
    const SolData * sol = NULL;
    const SolData * vsol = NULL;
    bool drawelem = 0;
    
    if (scalfunction != -1)
      sol = soldata[scalfunction];
    if (vecfunction != -1)
      vsol = soldata[vecfunction];
    
    if (mesh->GetTimeStamp () > solutiontimestamp)
      {
        sol = NULL;
        vsol = NULL;
      }

    glLineWidth (1.0f);


    Point<3> points[1100];
    Vec<3> nvs[1100];
    double values[1100];
    double valuesc[1100][2];

    int nse = mesh->GetNSE();

    if ( usetexture )
      glColor3d (1.0, 1.0, 1.0);   
    else
      glColor3d (0.6, 0.6, 0.6);   
    CurvedElements & curv = mesh->GetCurvedElements();
  
    glBegin (GL_QUADS);
    for (SurfaceElementIndex sei = 0; sei < nse; sei++)
      {
        const Element2d & el = (*mesh)[sei];

#ifdef PARALLEL
	// parallel visualization --> dont draw ghost elements
	if ( el . IsGhost() ) continue;
#endif

        if(vispar.drawdomainsurf > 0 &&
           ((mesh->GetDimension() == 3 && 
             vispar.drawdomainsurf != mesh->GetFaceDescriptor(el.GetIndex()).DomainIn() &&
             vispar.drawdomainsurf != mesh->GetFaceDescriptor(el.GetIndex()).DomainOut()) ||
            (mesh->GetDimension() == 2 && el.GetIndex() != vispar.drawdomainsurf))) continue;

        if ( el.GetType() == QUAD || el.GetType() == QUAD6 )
          {
            bool curved = curv.IsHighOrder(); //  && curv.IsSurfaceElementCurved(sei);

            Point<3> lpi[4];
            Vec<3> vx, vy, vtwist;
            
            if (! curved)
              {
                for (int k = 0; k < 4; k++)
                  GetPointDeformation (el[k]-1, lpi[k]);
                
                vx = lpi[1]-lpi[0];
                vy = lpi[3]-lpi[0];
                vtwist = (lpi[0]-lpi[1]) + (lpi[2]-lpi[3]);
              }
	    
//             Vec<3> nv = Cross (lpi[1]-lpi[0], Center (lpi[2],lpi[3]) - lpi[0]);
//             nv.Normalize();
//             glNormal3dv (nv);

            int n = 1 << subdivisions;
            int ii = 0;
            int ix, iy;
            for (iy = 0; iy <= n; iy++)
              for (ix = 0; ix <= n; ix++)
                {
                  double x = double(ix) / n;
                  double y = double(iy) / n;

                  if (sol && sol->draw_surface) 
                    {
                      if (usetexture == 2)
                        drawelem = GetSurfValueComplex (sol, sei, x, y, scalcomp, valuesc[ii][0], valuesc[ii][1]);
                      else
                        drawelem = GetSurfValue (sol, sei, x, y, scalcomp, values[ii]);
                    }

                  if (curved)
                    {
                      Point<2> xref(x,y);
                      Mat<3,2> dxdxi;

                      mesh->GetCurvedElements().
                        CalcSurfaceTransformation (xref, sei, points[ii], dxdxi);
                      nvs[ii] = Cross (dxdxi.Col(0), dxdxi.Col(1));
                      nvs[ii].Normalize();
                    }
                  else
                    {
                      points[ii] = lpi[0] + x * vx + y * vy + x*y * vtwist;
                      nvs[ii] = Cross (vx, vy);
                      nvs[ii].Normalize();
                    }
                  
                  if (deform)
                    {
                      points[ii] += GetSurfDeformation (sei, x, y);
                    }

                  ii++;
                }
          
            ii = 0;
            for (iy = 0; iy < n; iy++, ii++)
              for (ix = 0; ix < n; ix++, ii++)
                {
                  double x = double(ix) / n;
                  double y = double(iy) / n;
                  
                  int index[] = { ii, ii+1, ii+n+2, ii+n+1 };
                  
                  for (int j = 0; j < 4; j++)
                    {
                      if (sol && sol->draw_surface)
                        {
                          if (usetexture == 2)
			    {
			      if(drawelem)
				glTexCoord2f ( valuesc[index[j]][0],
					       valuesc[index[j]][1] );
			      else
				glTexCoord2f ( minval,minval);
			    }
                          else
			    {
			      if(drawelem)
				SetOpenGlColor  (values[index[j]]);
			      else
				glColor3d(0.6,0.6,0.6);
			    }
                        }
                      // SetOpenGlColor  (values[index[j]], minval, maxval, logscale);
                      //else
                      //        glColor3f (0.4f, 0.4f, 0.4f);
                      
                      glNormal3dv (nvs[index[j]]);
                      glVertex3dv (points[index[j]]);
                    }
                }
          }
      }
    glEnd();




    for(SurfaceElementIndex sei = 0; sei < nse; sei++)
      {
        const Element2d & el = (*mesh)[sei];

#ifdef PARALLEL
	// parallel visualization --> dont draw ghost elements
	if ( el . IsGhost() ) continue;
#endif

        if(vispar.drawdomainsurf > 0 &&
           ((mesh->GetDimension() == 3 && 
             vispar.drawdomainsurf != mesh->GetFaceDescriptor(el.GetIndex()).DomainIn() &&
             vispar.drawdomainsurf != mesh->GetFaceDescriptor(el.GetIndex()).DomainOut()) ||
            (mesh->GetDimension() == 2 && el.GetIndex() != vispar.drawdomainsurf))) continue;
        
        
        if ( el.GetType() == TRIG || el.GetType() == TRIG6 )
          {
            bool curved = curv.IsHighOrder(); //  && curv.IsSurfaceElementCurved(sei);
            
            Point<3> p1, p2, p3;
            Mat<3,2> dxdxi;

            if (! curved)
              {
                GetPointDeformation (el[0]-1, p1, sei);
                GetPointDeformation (el[1]-1, p2, sei);
                GetPointDeformation (el[2]-1, p3, sei);

                for (int i = 0; i < 3; i++)
                  {
                    dxdxi(i, 0) = p1(i)-p3(i);
                    dxdxi(i, 1) = p2(i)-p3(i);
                  }
              }
            
            int n = 1 << subdivisions;
            double invn = 1.0 / n;

            for (int iy = 0, ii = 0; iy <= n; iy++)
              for (int ix = 0; ix <= n-iy; ix++, ii++)
                {
                  Point<2> pref(ix*invn,iy*invn);
                  if (curved)
                    {
                      mesh->GetCurvedElements().
                        CalcSurfaceTransformation (pref, sei, points[ii], dxdxi);
                    }
                  else
                    {
                      points[ii] = p3 + (invn*ix) * (p1-p3) + (invn*iy) * (p2-p3);
                    }
                  
                  nvs[ii] = Cross (dxdxi.Col(0), dxdxi.Col(1));
                  nvs[ii].Normalize();

                  if (sol && sol->draw_surface) 
                    {
                      if (usetexture == 2)
                        drawelem = GetSurfValueComplex (sol, sei, ix*invn, iy*invn, scalcomp, 
                                                        valuesc[ii][0], valuesc[ii][1]);
                      else
                        drawelem = GetSurfValue (sol, sei, pref, points[ii], &dxdxi(0,0), scalcomp, values[ii]);
                    }
                  
                  if (deform)
                    points[ii] += GetSurfDeformation (sei, invn*ix, invn*iy);
                }


            for (int iy = 0, ii = 0; iy < n; iy++)
              {
                glBegin (GL_TRIANGLE_STRIP);
                for (int ix = 0; ix <= n-iy; ix++, ii++)
                  for (int k = 0; k < 2; k++)
                    {
                      if (ix+iy+k > n) continue;
                      int hi = (k == 0) ? ii : ii+n-iy+1;

                      if (sol && sol->draw_surface)
                        {
                          switch (usetexture)
                            {
                            case 0:
			      if(drawelem)
				SetOpenGlColor  (values[hi]);
			      else
				glColor3d(0.6,0.6,0.6);
			      //SetOpenGlColor (minval);
                              break;
                            case 1:
			      if(drawelem)
				glTexCoord1f ( values[hi] );
			      else
				glTexCoord1f (minval);
                              break;
                            case 2:
			      if(drawelem)
				glTexCoord2f ( valuesc[hi][0], valuesc[hi][1] );
			      else
				glTexCoord2f ( minval,minval);
                              break;
                            }
                        }

                      glNormal3dv (nvs[hi]);
                      glVertex3dv (points[hi]);
                    }
                glEnd();
              }
	  }
      }
  }


  // Bernstein Pol B_{n,i}(x) = n! / i! / (n-i)! (1-x)^{n-i} x^i
  static inline double Bernstein (int n, int i, double x)
  {
    double val = 1;
    for (int j = 1; j <= i; j++)   
      val *= x;
    for (int j = 1; j <= n-i; j++) 
      val *= (1-x) * (j+i) / j;
    return val;
  }

  void  VisualSceneSolution :: DrawSurfaceElementLines ()
  {
    SurfaceElementIndex sei;

    glLineWidth (1.0f);
    glNormal3d (1, 0, 0);

    int nse = mesh->GetNSE();

    CurvedElements & curv = mesh->GetCurvedElements();

    for (sei = 0; sei < nse; sei++)
      {
        Element2d & el = (*mesh)[sei];


#ifdef PARALLEL
	// parallel visualization --> dont draw ghost elements
	if ( el . IsGhost() ) continue;
#endif

        bool curved = curv.IsHighOrder(); //  && curv.IsSurfaceElementCurved(sei);

        int nv;
        if (el.GetType() == TRIG || el.GetType() == TRIG6)
          nv = 3;
        else 
          nv = 4;
        
        Point<3> p1, p2, p3, p4;
        if (!curved)
          {
            p1 = (*mesh)[el[0]];
            p2 = (*mesh)[el[1]];
            p3 = (*mesh)[el[2]];
            if (nv == 4)
              p4 = (*mesh)[el[3]];
          }


        // glBegin (GL_LINE_LOOP);
        int n = 1 << subdivisions;
        // n = p;

        Point<3> pnt;
        for (int k = 0; k < nv; k++)
          {
            Point<2> p0;
            Vec<2> vtau;
            if (nv == 3)
              switch (k)
                {
                case 0:
                  p0 = Point<2> (0,0);
                  vtau = Vec<2> (1,0);
                  break;
                case 1:
                  p0 = Point<2> (1,0);
                  vtau = Vec<2> (-1,1);
                  break;
                case 2:
                  p0 = Point<2> (0,1);
                  vtau = Vec<2> (0,-1);
                  break;
                }
            else
              switch (k)
                {
                case 0:
                  p0 = Point<2> (0,0);
                  vtau = Vec<2> (1,0);
                  break;
                case 1:
                  p0 = Point<2> (1,0);
                  vtau = Vec<2> (0,1);
                  break;
                case 2:
                  p0 = Point<2> (1,1);
                  vtau = Vec<2> (-1,0);
                  break;
                case 3:
                  p0 = Point<2> (0,1);
                  vtau = Vec<2> (0,-1);
                  break;
                }
            
            glBegin (GL_LINE_STRIP);

            if (curved)
              {
                ArrayMem<Point<2>, 65> ptsloc(n+1);
                ArrayMem<Point<3>, 65> ptsglob(n+1);

                for (int ix = 0; ix <= n; ix++)
                  ptsloc[ix] = p0 + (double(ix) / n) * vtau;
                    
                mesh->GetCurvedElements().
                  CalcMultiPointSurfaceTransformation (&ptsloc, sei, &ptsglob, 0);

                for (int ix = 0; ix <= n; ix++)
                  {
                    if (deform)
                      ptsglob[ix] += GetSurfDeformation (sei, ptsloc[ix](0), ptsloc[ix](1));
                    glVertex3dv (ptsglob[ix]);
                  }
              }
            else
              {
                for (int ix = 0; ix <= n; ix++)
                  {
                    Point<2> p = p0 + (double(ix) / n) * vtau;
                    
                    if (nv == 3)
                      pnt = p3 + p(0) * (p1-p3) + p(1) * (p2-p3);
                    else
                      pnt = p1 + p(0) * (p2-p1) + p(1) * (p4-p1) + p(0)*p(1) * ( (p1-p2)+(p3-p4) );
                
                    if (deform)
                      pnt += GetSurfDeformation (sei, p(0), p(1) );
                    
                    glVertex3dv (pnt);
                  }
              }
            glEnd ();
          }
      }
  }









  void VisualSceneSolution :: DrawIsoSurface(const SolData * sol, 
                                             const SolData * vsol,
                                             int comp)
  {
    if (!draw_isosurface) return;
    if (!sol) return;

   
    glColor3d (1.0, 0, 0);
    glDisable (GL_TEXTURE_1D);
    glDisable (GL_TEXTURE_2D);
    glEnable (GL_COLOR_MATERIAL);

    /*
      GLfloat matcol0[] = { 0.5, 0, 0, 1 };
      glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, matcol0);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, matcol0);
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matcol0);
    */

    glBegin (GL_TRIANGLES);

    int np = mesh->GetNP();
    int ne = mesh->GetNE();


    const int edgei[6][2] =
      { { 0, 1 }, { 0, 2 }, { 0, 3 },
        { 1, 2 }, { 1, 3 }, { 2, 3 } };
    
    double edgelam[6];
    Point<3> edgep[6];
    Vec<3> normp[6];
    double nodevali[4];
    
    int cntce;
    int cpe1 = 0, cpe2 = 0, cpe3 = 0;
    
    int n = 1 << subdivisions;
    int n3 = (n+1)*(n+1)*(n+1);
    
    ARRAY<Point<3> > grid(n3);
    ARRAY<Point<3> > locgrid(n3);
    ARRAY<Mat<3,3> > trans(n3);
    ARRAY<double> val(n3);
    ARRAY<Vec<3> > grads(n3);
    ARRAY<int> compress(n3);
    
    MatrixFixWidth<3> pointmat(8);
    grads = Vec<3> (0.0);

    for (ElementIndex ei = 0; ei < ne; ei++)
      {
        // if(vispar.clipdomain > 0 && vispar.clipdomain != (*mesh)[ei].GetIndex()) continue;
        // if(vispar.donotclipdomain > 0 && vispar.donotclipdomain == (*mesh)[ei].GetIndex()) continue;

#ifdef PARALLEL
	// parallel visualization --> dont draw ghost elements
	if ( (*mesh)[ei] . IsGhost() ) continue;
#endif

        ELEMENT_TYPE type = (*mesh)[ei].GetType();
        if (type == HEX || type == PRISM || type == TET || type == PYRAMID)
          {
            const Element & el = (*mesh)[ei];
            
            int ii = 0;
            int cnt_valid = 0;
            
            for (int ix = 0; ix <= n; ix++)
              for (int iy = 0; iy <= n; iy++)
                for (int iz = 0; iz <= n; iz++, ii++)
                  {
                    Point<3> ploc;
                    compress[ii] = ii;
                    
                    switch (type)
                      {
                      case PRISM:
                        if (ix+iy <= n)
                          {
                            ploc = Point<3> (double(ix) / n, double(iy) / n, double(iz) / n);
                            compress[ii] = cnt_valid;
                            cnt_valid++;
                          }
                        else
                          compress[ii] = -1;
                        break;
                      case TET:
                        if (ix+iy+iz <= n)
                          {
                            ploc = Point<3> (double(ix) / n, double(iy) / n, double(iz) / n);
                            compress[ii] = cnt_valid;
                            cnt_valid++;
                          }
                        else
                          compress[ii] = -1;
                        break;
                      case HEX:
                        ploc = Point<3> (double(ix) / n, double(iy) / n, double(iz) / n);
                        break;
                      case PYRAMID:
                        ploc = Point<3> (double(ix) / n * (1-double(iz)/n),
                                         double(iy) / n * (1-double(iz)/n),
                                         double(iz)/n);
                        break;
                      }
                    if (compress[ii] != -1)
                      locgrid[compress[ii]] = ploc;
                  }
            
            if (type != TET && type != PRISM) cnt_valid = n3;
            
            
            if (mesh->GetCurvedElements().IsHighOrder() || 1)
              {
                mesh->GetCurvedElements().
                  CalcMultiPointElementTransformation (&locgrid, ei, &grid, &trans);
              }
            else
              {
                Vector shape(el.GetNP());
                for (int k = 0; k < el.GetNP(); k++)
                  for (int j = 0; j < 3; j++)
                    pointmat(k,j) = (*mesh)[el[k]](j);
                
                for (int i = 0; i < cnt_valid; i++)
                  {
                    el.GetShapeNew (locgrid[i], shape);
                    Point<3> pglob;
                    for (int j = 0; j < 3; j++)
                      {
                        pglob(j) = 0;
                        for (int k = 0; k < el.GetNP(); k++)
                          pglob(j) += shape(k) * pointmat(k,j);
                      }
                    grid[i] = pglob;
                  }
              }

            bool has_pos = 0, has_neg = 0;
                
            for (int i = 0; i < cnt_valid; i++)
              {
                GetValue (sol, ei, &locgrid[i](0), &grid[i](0), &trans[i](0), comp, val[i]);
        
                val[i] -= minval;

                if (vsol)
                  GetValues (vsol, ei, &locgrid[i](0), &grid[i](0), &trans[i](0), &grads[i](0));
                grads[i] *= -1;


                if (val[i] > 0)
                  has_pos = 1;
                else
                  has_neg = 1;
              }

            if (!has_pos || !has_neg) continue;
            
            for (int ix = 0; ix < n; ix++)
              for (int iy = 0; iy < n; iy++)
                for (int iz = 0; iz < n; iz++)
                  {
                    int base = iz + (n+1)*iy + (n+1)*(n+1)*ix;
                    int pi[8] = 
                      { base, base+(n+1)*(n+1), base+(n+1)*(n+1)+(n+1), base+(n+1),
                        base+1, base+(n+1)*(n+1)+1, base+(n+1)*(n+1)+(n+1)+1, base+(n+1)+1 };
                    
                    for (int j = 0; j < 8; j++)
                      pi[j] = compress[pi[j]];
                    
                    int tets[6][4] = 
                      { { 1, 2, 4, 5 },
                        { 4, 5, 2, 8 },
                        { 2, 8, 5, 6 },
                        { 2, 3, 4, 8 },
                        { 2, 3, 8, 6 },
                        { 3, 8, 6, 7 } };
                    
                    for (int ii = 0; ii < 6; ii++)
                      {
                        int teti[4];
                        for (int k = 0; k < 4; k++)
                          teti[k] = pi[tets[ii][k]-1];
                        
                        bool is_valid = 1;
                        for (int j = 0; j < 4; j++)
                          if (teti[j] == -1) is_valid = 0;
                        
                        if (!is_valid) continue;
                        
                        for (int j = 0; j < 4; j++)
                          nodevali[j] = val[teti[j]];
                        
                        cntce = 0;
                        for (int j = 0; j < 6; j++)
                          {
                            int lpi1 = edgei[j][0];
                            int lpi2 = edgei[j][1];
                            if ( (nodevali[lpi1] > 0) !=
                                 (nodevali[lpi2] > 0) )
                              {
                                Point<3> p1 = grid[teti[lpi1]];
                                Point<3> p2 = grid[teti[lpi2]];

                                edgelam[j] = nodevali[lpi2] / (nodevali[lpi2] - nodevali[lpi1]);
                                edgep[j] = grid[teti[lpi1]] + (1-edgelam[j]) * (grid[teti[lpi2]]-grid[teti[lpi1]]);
                                normp[j] = grads[teti[lpi1]] + (1-edgelam[j]) * (grads[teti[lpi2]]-grads[teti[lpi1]]);
                                
                                cntce++;
                                cpe3 = cpe2;
                                cpe2 = cpe1;
                                cpe1 = j;
                                if (cntce >= 3)
                                  {
                                    if (!vsol)
                                      {
                                        Point<3> points[3];
                                        
                                        points[0] = edgep[cpe1];
                                        points[1] = edgep[cpe2];
                                        points[2] = edgep[cpe3];


                                        Vec<3> normal = Cross (points[2]-points[0], points[1]-points[0]);
                                        if ( (normal * (p2-p1)) > 0 == nodevali[lpi1] < 0)
                                          normal *= -1;
                                        glNormal3dv (normal);

                                        glVertex3dv (points[0]);
                                        glVertex3dv (points[1]);
                                        glVertex3dv (points[2]);
                                      }
                                    else
                                      {
                                        // glNormal3dv (grads[teti[0]]);
                                        
                                        glNormal3dv (normp[cpe1]);
                                        glVertex3dv (edgep[cpe1]);
                                        glNormal3dv (normp[cpe2]);
                                        glVertex3dv (edgep[cpe2]);
                                        glNormal3dv (normp[cpe3]);
                                        glVertex3dv (edgep[cpe3]);
                                      }
                                  }
                              }
                          }
                      }
                  }
          }
      }
    glEnd();
  }
    







  void  VisualSceneSolution :: DrawTrigSurfaceVectors(const ARRAY< Point<3> > & lp, const Point<3> & pmin, const Point<3> & pmax,
                                                      const int sei, const SolData * vsol)
  {
    int dir,dir1,dir2;
    double s,t;
    bool drawelem;

    Vec<3> n = Cross (lp[1]-lp[0], lp[2]-lp[0]);
    Vec<3> na (fabs (n(0)), fabs(n(1)), fabs(n(2)));
    if (na(0) > na(1) && na(0) > na(2))
      dir = 1;
    else if (na(1) > na(2))
      dir = 2;
    else 
      dir = 3;
    
    dir1 = (dir % 3) + 1;
    dir2 = (dir1 % 3) + 1;

    Point<2> p2d[3];

    int k;

    for (k = 0; k < 3; k++)
      {
        p2d[k] = Point<2> ((lp[k](dir1-1) - pmin(dir1-1)) / (2*rad),
                           (lp[k](dir2-1) - pmin(dir2-1)) / (2*rad));
      }

    
    double minx2d, maxx2d, miny2d, maxy2d;
    minx2d = maxx2d = p2d[0](0);
    miny2d = maxy2d = p2d[0](1);
    for (k = 1; k < 3; k++)
      {
        minx2d = min2 (minx2d, p2d[k](0));
        maxx2d = max2 (maxx2d, p2d[k](0));
        miny2d = min2 (miny2d, p2d[k](1));
        maxy2d = max2 (maxy2d, p2d[k](1));
      }
    
    double mat11 = p2d[1](0) - p2d[0](0);
    double mat21 = p2d[1](1) - p2d[0](1);
    double mat12 = p2d[2](0) - p2d[0](0);
    double mat22 = p2d[2](1) - p2d[0](1);
    
    double det = mat11*mat22-mat21*mat12;
    double inv11 = mat22/det;
    double inv21 = -mat21/det;
    double inv12 = -mat12/det;
    double inv22 = mat11/det;
          
    //    cout << "drawsurfacevectors. xoffset = " << xoffset << ", yoffset = ";
    //    cout << yoffset << endl;
    
    for (s = xoffset/gridsize; s <= 1+xoffset/gridsize; s += 1.0 / gridsize)
      if (s >= minx2d && s <= maxx2d)
        for (t = yoffset/gridsize; t <= 1+yoffset/gridsize; t += 1.0 / gridsize)
          if (t >= miny2d && t <= maxy2d)
            {
              double lam1 = inv11 * (s - p2d[0](0)) + inv12 * (t-p2d[0](1));
              double lam2 = inv21 * (s - p2d[0](0)) + inv22 * (t-p2d[0](1));
              
              if (lam1 >= 0 && lam2 >= 0 && lam1+lam2 <= 1)
                {
                  Point<3> cp;
                  for (k = 0; k < 3; k++)
                    cp(k) = lp[0](k) + 
                      lam1 * (lp[1](k)-lp[0](k)) + 
                      lam2 * (lp[2](k)-lp[0](k));
                  
                  Vec<3> v;
                  double values[6];
                  drawelem = GetSurfValues (vsol, sei, lam1, lam2, values);
                  
                  if (!vsol->iscomplex)
                    for (k = 0; k < 3; k++)
                      v(k) = values[k];
                  else
                    {
                      if (!imag_part)
                        for (k = 0; k < 3; k++)
                          v(k) = values[2*k];
                      else
                        for (k = 0; k < 3; k++)
                          v(k) = values[2*k+1];
                    }
                  
                  if (mesh->GetDimension() == 2)
                    if ( (!vsol->iscomplex && vsol->components != 3) ||
                         (vsol->iscomplex && vsol->components != 6) )
                      v(2) = 0;
                  
                  double val = v.Length();
                  SetOpenGlColor  (val, minval, maxval, logscale);
                  
                  if (val > 1e-10 * maxval)
                    v *= (rad / val / gridsize * 0.5);
                  else drawelem = 0;
                  // "drawelem": added 07.04.2004 (FB)
                  if ( drawelem ) DrawCone (cp, cp+4*v, 0.8*rad / gridsize);
                  
                  
                  /*
                    v /= val;
                    
                    glPushMatrix();
                    glTranslated (cp(0), cp(1), cp(2));
                    
                    double l = 2*rad/gridsize;
                    double r = 0.8*rad/gridsize;
                    glScaled (l, l, l);
                    
                    double phi = acos (v(2));
                    glRotated (-180/M_PI*phi, v(1), -v(0), 0);
                    
                    glCallList (cone_list);
                    glPopMatrix();
                  */
                }
            }
    
  }



  void  VisualSceneSolution :: DrawSurfaceVectors ()
  {
    //int j, k;
    //int dir, dir1, dir2;
    SurfaceElementIndex sei;

    const SolData * vsol = NULL;
    // bool drawelem;

    if (vecfunction != -1)
      vsol = soldata[vecfunction];

    if (mesh->GetTimeStamp () > solutiontimestamp)
      vsol = NULL;
     
    if (!vsol) return;


    Point<3> pmin = center - Vec3d (rad, rad, rad);
    Point<3> pmax = center - Vec3d (rad, rad, rad);

    //double s, t;

    // draw surface cones
    //  if (0)
    /*
      if (vsol->soltype==SOL_SURFACE_ELEMENT ||
      vsol->soltype==SOL_SURFACE_NONCONTINUOUS ||
      vsol->soltype==SOL_VIRTUALFUNCTION)
    */

    glColor3d (1.0, 1.0, 1.0);

    if (vsol->draw_surface && showsurfacesolution)
      {
        int nse = mesh->GetNSE();
        for (sei = 0; sei < nse; sei++)
          {
            const Element2d & el = (*mesh)[sei];
          
#ifdef PARALLEL
	    // parallel visualization --> dont draw ghost elements
	    if ( el . IsGhost() ) continue;
#endif

            if (el.GetType() == TRIG || el.GetType() == TRIG6)
              {
          
                ARRAY< Point<3> > lp(3);
                //Point<2> p2d[3];
                /*
                  for (k = 0; k < 3; k++)
                  lp[k] = mesh->Point (el[k]);
                */
                lp[0] = mesh->Point(el[2]);
                lp[1] = mesh->Point(el[0]);
                lp[2] = mesh->Point(el[1]);

                DrawTrigSurfaceVectors(lp,pmin,pmax,sei,vsol);
                
                /*
                  Vec<3> n = Cross (lp[1]-lp[0], lp[2]-lp[0]);
                  Vec<3> na (fabs (n(0)), fabs(n(1)), fabs(n(2)));
                  if (na(0) > na(1) && na(0) > na(2))
                  dir = 1;
                  else if (na(1) > na(2))
                  dir = 2;
                  else 
                  dir = 3;

                  dir1 = (dir % 3) + 1;
                  dir2 = (dir1 % 3) + 1;
          
                  for (k = 0; k < 3; k++)
                  {
                  p2d[k] = Point<2> ((lp[k](dir1-1) - pmin(dir1-1)) / (2*rad),
                  (lp[k](dir2-1) - pmin(dir2-1)) / (2*rad));
                  }
          
                  double minx2d, maxx2d, miny2d, maxy2d;
                  minx2d = maxx2d = p2d[0](0);
                  miny2d = maxy2d = p2d[0](1);
                  for (k = 1; k < 3; k++)
                  {
                  minx2d = min2 (minx2d, p2d[k](0));
                  maxx2d = max2 (maxx2d, p2d[k](0));
                  miny2d = min2 (miny2d, p2d[k](1));
                  maxy2d = max2 (maxy2d, p2d[k](1));
                  }

                  double mat11 = p2d[1](0) - p2d[0](0);
                  double mat21 = p2d[1](1) - p2d[0](1);
                  double mat12 = p2d[2](0) - p2d[0](0);
                  double mat22 = p2d[2](1) - p2d[0](1);

                  double det = mat11*mat22-mat21*mat12;
                  double inv11 = mat22/det;
                  double inv21 = -mat21/det;
                  double inv12 = -mat12/det;
                  double inv22 = mat11/det;
          
                  //      cout << "drawsurfacevectors. xoffset = " << xoffset << ", yoffset = ";
                  //      cout << yoffset << endl;
          
                  for (s = xoffset/gridsize; s <= 1+xoffset/gridsize; s += 1.0 / gridsize)
                  if (s >= minx2d && s <= maxx2d)
                  for (t = yoffset/gridsize; t <= 1+yoffset/gridsize; t += 1.0 / gridsize)
                  if (t >= miny2d && t <= maxy2d)
                  {
                  double lam1 = inv11 * (s - p2d[0](0)) + inv12 * (t-p2d[0](1));
                  double lam2 = inv21 * (s - p2d[0](0)) + inv22 * (t-p2d[0](1));
                    
                  if (lam1 >= 0 && lam2 >= 0 && lam1+lam2 <= 1)
                  {
                  Point<3> cp;
                  for (k = 0; k < 3; k++)
                  cp(k) = lp[0](k) + 
                  lam1 * (lp[1](k)-lp[0](k)) + 
                  lam2 * (lp[2](k)-lp[0](k));

                  Vec<3> v;
                  double values[6];
                  drawelem = GetSurfValues (vsol, sei, lam1, lam2, values);

                  if (!vsol->iscomplex)
                  for (k = 0; k < 3; k++)
                  v(k) = values[k];
                  else
                  {
                  if (!imag_part)
                  for (k = 0; k < 3; k++)
                  v(k) = values[2*k];
                  else
                  for (k = 0; k < 3; k++)
                  v(k) = values[2*k+1];
                  }
                        
                  if (mesh->GetDimension() == 2)
                  if ( (!vsol->iscomplex && vsol->components != 3) ||
                  (vsol->iscomplex && vsol->components != 6) )
                  v(2) = 0;
                        
                  double val = v.Length();
                  SetOpenGlColor  (val, minval, maxval, logscale);

                  if (val > 1e-10 * maxval)
                  v *= (rad / val / gridsize * 0.5);
                  else drawelem = 0;
                  // "drawelem": added 07.04.2004 (FB)
                  if ( drawelem ) DrawCone (cp, cp+4*v, 0.8*rad / gridsize);


                  }
                  }
                */
              }
            else if (el.GetType() == QUAD)
              {
                ARRAY < Point<3> > lp(3);

                lp[0] = mesh->Point(el[0]);
                lp[1] = mesh->Point(el[1]);
                lp[2] = mesh->Point(el[2]);

                DrawTrigSurfaceVectors(lp,pmin,pmax,sei,vsol);

                lp[0] = mesh->Point(el[0]);
                lp[1] = mesh->Point(el[2]);
                lp[2] = mesh->Point(el[3]);

                DrawTrigSurfaceVectors(lp,pmin,pmax,sei,vsol);

                /*
                  Point<3> lp[4];
                  Point<2> p2d[4];
              
                  for (k = 0; k < 4; k++)
                  lp[k] = mesh->Point (el[k]);
                  

                  Vec<3> n = Cross (lp[1]-lp[0], lp[2]-lp[0]);
                  Vec<3> na (fabs (n(0)), fabs(n(1)), fabs(n(2)));
                  if (na(0) > na(1) && na(0) > na(2))
                  dir = 1;
                  else if (na(1) > na(2))
                  dir = 2;
                  else 
                  dir = 3;
              
                  dir1 = (dir % 3) + 1;
                  dir2 = (dir1 % 3) + 1;
              
                  for (k = 0; k < 4; k++)
                  {
                  p2d[k] = Point<2> ((lp[k](dir1-1) - pmin(dir1-1)) / (2*rad),
                  (lp[k](dir2-1) - pmin(dir2-1)) / (2*rad));
                  }
              
                  double minx2d, maxx2d, miny2d, maxy2d;
                  minx2d = maxx2d = p2d[0](0);
                  miny2d = maxy2d = p2d[0](1);
                  for (k = 1; k < 4; k++)
                  {
                  minx2d = min2 (minx2d, p2d[k](0));
                  maxx2d = max2 (maxx2d, p2d[k](0));
                  miny2d = min2 (miny2d, p2d[k](1));
                  maxy2d = max2 (maxy2d, p2d[k](1));
                  }
          
                  for (s = xoffset/gridsize; s <= 1+xoffset/gridsize; s += 1.0 / gridsize)
                  if (s >= minx2d && s <= maxx2d)
                  for (t = yoffset/gridsize; t <= 1+yoffset/gridsize; t += 1.0 / gridsize)
                  if (t >= miny2d && t <= maxy2d)
                  {
                  double lami[3];
                  Point3d p3d(2*rad*s+pmin(0), 2*rad*t+pmin(1),0);

                  if (mesh->PointContainedIn2DElement (p3d, lami, sei+1))
                  {
                  Point<3> cp = p3d;
                  double lam1 = lami[0];
                  double lam2 = lami[1];

                  //for (k = 0; k < 3; k++)
                  //cp(k) = lp[0](k) + 
                  //lam1 * (lp[1](k)-lp[0](k)) + 
                  //lam2 * (lp[2](k)-lp[0](k));
                              

                  Vec<3> v;
                  double values[6];
                  drawelem = GetSurfValues (vsol, sei, lam1, lam2, values);
                  (*testout) << "sei " << sei << " lam1 " << lam1 << " lam2 " << lam2 << " drawelem " << drawelem << endl;

                  if (!vsol->iscomplex)
                  for (k = 0; k < 3; k++)
                  v(k) = values[k];
                  else
                  {
                  if (!imag_part)
                  for (k = 0; k < 3; k++)
                  v(k) = values[2*k];
                  else
                  for (k = 0; k < 3; k++)
                  v(k) = values[2*k+1];
                  }
                        
                  if (mesh->GetDimension() == 2)
                  if ( (!vsol->iscomplex && vsol->components != 3) ||
                  (vsol->iscomplex && vsol->components != 6) )
                  v(2) = 0;
                        
                  double val = v.Length();
                  SetOpenGlColor  (val, minval, maxval, logscale);

                  (*testout) << "v " << v << endl;

                  if (val > 1e-10 * maxval)
                  v *= (rad / val / gridsize * 0.5);

                  (*testout) << "v " << v << endl;

                  if ( drawelem )
                  {
                  DrawCone (cp, cp+4*v, 0.8*rad / gridsize);
                  (*testout) << "cp " << cp << " rad " << rad << " gridsize " << gridsize << endl;
                  }
                                
                  }
                  }
                */
              }
          }
      }
  }
  
  
  
  
  void VisualSceneSolution :: 
  DrawIsoLines (const Point<3> & p1, 
                const Point<3> & p2, 
                const Point<3> & p3,
                double val1, double val2, double val3,
                double minval, double maxval, int n)
  {
    DrawIsoLines2 (p1, p2, p1, p3, val1, val2, val1, val3, minval, maxval, n);
    DrawIsoLines2 (p2, p1, p2, p3, val2, val1, val2, val3, minval, maxval, n);
    DrawIsoLines2 (p3, p1, p3, p2, val3, val1, val3, val2, minval, maxval, n);
  }

  void VisualSceneSolution :: 
  DrawIsoLines2 (const Point<3> & p1, 
                 const Point<3> & p2, 
                 const Point<3> & p3,
                 const Point<3> & p4,
                 double val1, double val2, double val3, double val4,
                 double minval, double maxval, int n)
  {
    if (val1 > val2) 
      DrawIsoLines2 (p2, p1, p3, p4, val2, val1, val3, val4, minval, maxval, n);
    if (val3 > val4) 
      DrawIsoLines2 (p1, p2, p4, p3, val1, val2, val4, val3, minval, maxval, n);

    val2 += 1e-10;
    val4 += 1e-10;

    double fac = (maxval-minval) / n;
    double idelta1 = 1.0 / (val2 - val1);
    double idelta2 = 1.0 / (val4 - val3);

    int mini = int ((max2 (val1, val3) - minval) / fac);
    int maxi = int ((min2 (val2, val4) - minval) / fac);
    if (mini < 0) mini = 0;
    if (maxi > n-1) maxi = n-1;

    for (int i = mini; i <= maxi+1; i++)
      {
        double val = minval + i * fac;
        double lam1 = (val - val1) * idelta1;
        double lam2 = (val - val3) * idelta2;
        if (lam1 >= 0 && lam1 <= 1 && lam2 >= 0 && lam2 <= 1)
          {
            Point<3> lp1 = p1 + lam1 * (p2-p1);
            Point<3> lp2 = p3 + lam2 * (p4-p3);
            glVertex3dv (lp1 );
            glVertex3dv (lp2 );
            glVertex3dv (lp2 );
            glVertex3dv (lp1 );
          }
      }
  }




  void VisualSceneSolution :: 
  GetMinMax (int funcnr, int comp, double & minv, double & maxv) const
  {
    const SolData * sol;
    double val;
    bool considerElem;

    bool hasit = false;
    minv = 0; maxv = 1;
    if (funcnr != -1)
      {
        sol = soldata[funcnr];

        if (sol->draw_volume)
          {
            int ne = mesh->GetNE();
            for (int i = 0; i < ne; i++)
              {
                // "considerElem": added 07.04.2004 (FB)
                considerElem = GetValue (sol, i, 0.333, 0.333, 0.333, comp, val);
                if (considerElem)
                  {
                    if (val > maxv || !hasit)
                      maxv = val;
                    if (val < minv || !hasit)
                      minv = val;
                    hasit = true;
                  }
              }
          }
        if (sol->draw_surface)
          {
            int nse = mesh->GetNSE();
            for (int i = 0; i < nse; i++)
              {
                // "considerElem": added 07.04.2004 (FB)
                ELEMENT_TYPE type = mesh->SurfaceElement(i+1).GetType();
                if (type == QUAD)
                  considerElem = GetSurfValue (sol, i, 0.5, 0.5, comp, val);
                else
                  considerElem = GetSurfValue (sol, i, 0.3333333, 0.3333333, comp, val);
                if (considerElem)
                  {
                    if (val > maxv || !hasit)
                      maxv = val;
                    if (val < minv || !hasit)
                      minv = val;
                    hasit = true;
                  }
              }
          }
      }
    if (minv == maxv) maxv = minv+1e-6;
  }





  bool VisualSceneSolution :: 
  GetValues (const SolData * data, ElementIndex elnr, 
             double lam1, double lam2, double lam3,
             double * values) const
  {
    bool ok;
    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          ok = data->solclass->GetValue (elnr, lam1, lam2, lam3, values);
          break;
        }
      default:
        {
          for (int i = 0; i < data->components; i++)
            ok = GetValue (data, elnr, lam1, lam2, lam3, i+1, values[i]);
        }
      }
    return ok;
  }

  bool VisualSceneSolution :: 
  GetValues (const SolData * data, ElementIndex elnr, 
             const double xref[], const double x[], const double dxdxref[], 
             double * values) const
  {
    bool ok;
    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          ok = data->solclass->GetValue (elnr, xref, x, dxdxref, values);
          break;
        }
      default:
        {
          for (int i = 0; i < data->components; i++)
            ok = GetValue (data, elnr, xref[0], xref[1], xref[2], i+1, values[i]);
        }
      }
    return ok;
  }


  bool VisualSceneSolution :: 
  GetValue (const SolData * data, ElementIndex elnr, 
            const double xref[], const double x[], const double dxdxref[], 
            int comp, double & val) const
  {

    double lam1 = xref[0];
    double lam2 = xref[1];
    double lam3 = xref[2];
        
    val = 0;
    bool ok = 0;


    if (comp == 0)
      {
        ArrayMem<double,20> values(data->components);
        ok = GetValues (data, elnr, xref, x, dxdxref, &values[0]);

        switch (evalfunc)
          {
          case FUNC_ABS:
            {
              for (int ci = 0; ci < data->components; ci++)
                val += sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_ABS_TENSOR:
            {
              int d;
              switch (data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]);
              for (ci = d; ci < data->components; ci++)
                val += 2*sqr (values[ci]);
              val = sqrt (val);
              break;
            }

          case FUNC_MISES:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              double trace = 0.;
              for (ci = 0; ci < d; ci++)
                trace += 1./3.*(values[ci]);
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]-trace);
              for (ci = d; ci < data->components; ci++)
                val += 2.*sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_MAIN:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              Mat<3,3> m ;
              Vec<3> ev;
              int ci;
              for (ci = 0; ci < d; ci++)
                m(ci,ci) = (values[ci]);
              m(0,1) = m(1,0) = values[3];
              m(0,2) = m(2,0) = values[4];
              m(1,2) = m(2,1) = values[5];

              EigenValues (m, ev);
              double help;
              for (int i=0; i<d; i++)
                {
                  for (int j=d-1; i<j; j--)
                    {
                      if ( abs(ev(j)) > abs(ev(j-1)) )
                        {
                          help = ev(j);
                          ev(j) = ev(j-1);
                          ev(j-1) = help;
                        }
                    }
                }
              val = (ev(0));
              break;
            }
          }

        return ok;      
      }


    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          double values[20];
          ok = data->solclass->GetValue (elnr, xref, x, dxdxref, values);

          val = values[comp-1];
          return ok;
        }
      case SOL_NODAL:
        {
          const Element & el = (*mesh)[elnr];

          double lami[8];
          int np, i;
        
          switch (el.GetType())
            {
            case TET:
            case TET10:
              {
                lami[1] = lam1;
                lami[2] = lam2;
                lami[3] = lam3;
                lami[0] = 1-lam1-lam2-lam3;
                np = 4;
                break;
              }
            case PRISM:
            case PRISM12:
              {
                lami[0] = (1-lam3) * (1-lam1-lam2);
                lami[1] = (1-lam3) * lam1;
                lami[2] = (1-lam3) * lam2;
                lami[3] = (lam3) * (1-lam1-lam2);
                lami[4] = (lam3) * lam1;
                lami[5] = (lam3) * lam2;
                np = 6;
                break;
              }     
            }

          for (i = 0; i < np; i++)
            val += lami[i] * data->data[(el[i]-1) * data->dist + comp-1];

          return 1;
        }

      case SOL_ELEMENT:
        {
          val = data->data[elnr * data->dist + comp-1];
          return 1;
        }

      case SOL_SURFACE_ELEMENT:
        return 0;

      case SOL_NONCONTINUOUS:
        {
          const Element & el = (*mesh)[elnr];

          double lami[8];
          int np, i;

          switch (el.GetType())
            {
            case TET:
            case TET10:
              {
                lami[1] = lam1;
                lami[2] = lam2;
                lami[3] = lam3;
                lami[0] = 1-lam1-lam2-lam3;
                np = 4;
                break;
              }
            case PRISM:
            case PRISM12:
              {
                lami[0] = (1-lam3) * (1-lam1-lam2);
                lami[1] = (1-lam3) * lam1;
                lami[2] = (1-lam3) * lam2;
                lami[3] = (lam3) * (1-lam1-lam2);
                lami[4] = (lam3) * lam1;
                lami[5] = (lam3) * lam2;
                np = 6;
                break;
              }
            case PYRAMID:
              {
                if (lam3 > 1-1e-5)
                  {
                    lami[0] = lami[1] = lami[2] = lami[3] = 0;
                    lami[4] = 1;
                  }
                else
                  {
                    double x0 = lam1 / (1-lam3);
                    double y0 = lam2 / (1-lam3);
                    lami[0] = (1-x0) * (1-y0) * (1-lam3);
                    lami[1] = (  x0) * (1-y0) * (1-lam3);
                    lami[2] = (  x0) * (  y0) * (1-lam3);
                    lami[3] = (1-x0) * (  y0) * (1-lam3);
                    lami[4] = lam3;
                    np = 5;
                  }
                break;
              }
            default:
              np = 0;
            }

          int base;
          if (data->order == 1)
            base = 6 * elnr;
          else
            base = 10 * elnr;


          for (i = 0; i < np; i++)
            val += lami[i] * data->data[(base+i) * data->dist + comp-1];

          return 1;
        }

      case SOL_MARKED_ELEMENTS:
        {
          val = (*mesh)[elnr].TestRefinementFlag();
          return 1;
        }
      
      case SOL_ELEMENT_ORDER:
        {
          val = (*mesh)[elnr].GetOrder();
          return 1;
        }
      }
    return 0;
  }



  bool VisualSceneSolution :: 
  GetValue (const SolData * data, ElementIndex elnr, 
            double lam1, double lam2, double lam3,
            int comp, double & val) const
  {

    val = 0;
    bool ok = 0;

    if (comp == 0)
      {
        ArrayMem<double,20> values(data->components);
        ok = GetValues (data, elnr, lam1, lam2, lam3, &values[0]);

        switch (evalfunc)
          {
          case FUNC_ABS:
            {
              for (int ci = 0; ci < data->components; ci++)
                val += sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_ABS_TENSOR:
            {
              int d;
              switch (data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]);
              for (ci = d; ci < data->components; ci++)
                val += 2*sqr (values[ci]);
              val = sqrt (val);
              break;
            }

          case FUNC_MISES:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              double trace = 0.;
              for (ci = 0; ci < d; ci++)
                trace += 1./3.*(values[ci]);
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]-trace);
              for (ci = d; ci < data->components; ci++)
                val += 2.*sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_MAIN:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              Mat<3,3> m ;
              Vec<3> ev;
              int ci;
              for (ci = 0; ci < d; ci++)
                m(ci,ci) = (values[ci]);
              m(0,1) = m(1,0) = values[3];
              m(0,2) = m(2,0) = values[4];
              m(1,2) = m(2,1) = values[5];

              EigenValues (m, ev);
              double help;
              for (int i=0; i<d; i++)
                {
                  for (int j=d-1; i<j; j--)
                    {
                      if ( abs(ev(j)) > abs(ev(j-1)) )
                        {
                          help = ev(j);
                          ev(j) = ev(j-1);
                          ev(j-1) = help;
                        }
                    }
                }
              val = (ev(0));
              break;
            }
          }

        return ok;      
      }


    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          double values[20];
          ok = data->solclass->GetValue (elnr, lam1, lam2, lam3, values);

          val = values[comp-1];
          return ok;
        }
      case SOL_NODAL:
        {
          const Element & el = (*mesh)[elnr];

          double lami[8];
          int np, i;
        
          switch (el.GetType())
            {
            case TET:
            case TET10:
              {
                lami[1] = lam1;
                lami[2] = lam2;
                lami[3] = lam3;
                lami[0] = 1-lam1-lam2-lam3;
                np = 4;
                break;
              }
            case PRISM:
            case PRISM12:
              {
                lami[0] = (1-lam3) * (1-lam1-lam2);
                lami[1] = (1-lam3) * lam1;
                lami[2] = (1-lam3) * lam2;
                lami[3] = (lam3) * (1-lam1-lam2);
                lami[4] = (lam3) * lam1;
                lami[5] = (lam3) * lam2;
                np = 6;
                break;
              }     
            }

          for (i = 0; i < np; i++)
            val += lami[i] * data->data[(el[i]-1) * data->dist + comp-1];

          return 1;
        }

      case SOL_ELEMENT:
        {
          val = data->data[elnr * data->dist + comp-1];
          return 1;
        }

      case SOL_SURFACE_ELEMENT:
        return 0;

      case SOL_NONCONTINUOUS:
        {
          const Element & el = (*mesh)[elnr];

          double lami[8];
          int np, i;

          switch (el.GetType())
            {
            case TET:
            case TET10:
              {
                lami[1] = lam1;
                lami[2] = lam2;
                lami[3] = lam3;
                lami[0] = 1-lam1-lam2-lam3;
                np = 4;
                break;
              }
            case PRISM:
            case PRISM12:
              {
                lami[0] = (1-lam3) * (1-lam1-lam2);
                lami[1] = (1-lam3) * lam1;
                lami[2] = (1-lam3) * lam2;
                lami[3] = (lam3) * (1-lam1-lam2);
                lami[4] = (lam3) * lam1;
                lami[5] = (lam3) * lam2;
                np = 6;
                break;
              }
            case PYRAMID:
              {
                if (lam3 > 1-1e-5)
                  {
                    lami[0] = lami[1] = lami[2] = lami[3] = 0;
                    lami[4] = 1;
                  }
                else
                  {
                    double x0 = lam1 / (1-lam3);
                    double y0 = lam2 / (1-lam3);
                    lami[0] = (1-x0) * (1-y0) * (1-lam3);
                    lami[1] = (  x0) * (1-y0) * (1-lam3);
                    lami[2] = (  x0) * (  y0) * (1-lam3);
                    lami[3] = (1-x0) * (  y0) * (1-lam3);
                    lami[4] = lam3;
                    np = 5;
                  }
                break;
              }
            default:
              np = 0;
            }

          int base;
          if (data->order == 1)
            base = 6 * elnr;
          else
            base = 10 * elnr;


          for (i = 0; i < np; i++)
            val += lami[i] * data->data[(base+i) * data->dist + comp-1];

          return 1;
        }

      case SOL_MARKED_ELEMENTS:
        {
          val = (*mesh)[elnr].TestRefinementFlag();
          return 1;
        }
      
      case SOL_ELEMENT_ORDER:
        {
          val = (*mesh)[elnr].GetOrder();
          return 1;
        }
      }
    return 0;
  }













  bool VisualSceneSolution :: 
  GetValueComplex (const SolData * data, ElementIndex elnr, 
                   double lam1, double lam2, double lam3,
                   int comp, double & valr, double & vali) const
  {

    valr = 0;
    vali = 0;
    bool ok = 0;

           
    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          double values[20];
          ok = data->solclass->GetValue (elnr, lam1, lam2, lam3, values);
          valr = values[comp-1];
          vali = values[comp];
          return ok;
        }
            
      } 
    return 0;
  }
  










  bool VisualSceneSolution :: 
  GetSurfValues (const SolData * data, SurfaceElementIndex selnr, 
                 double lam1, double lam2, 
                 double * values) const
  {
    bool ok;
    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          ok = data->solclass->GetSurfValue (selnr, lam1, lam2, values);
          // ok = 1;
          // values[0] = 1.0;
          break;
        }
      default:
        {
          for (int i = 0; i < data->components; i++)
            ok = GetSurfValue (data, selnr, lam1, lam2, i+1, values[i]);
        }
      }
    return ok;
  }


  bool VisualSceneSolution :: 
  GetSurfValues (const SolData * data, SurfaceElementIndex selnr, 
                 const double xref[], const double x[], const double dxdxref[], 
                 double * values) const
  {
    bool ok;
    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          ok = data->solclass->GetSurfValue (selnr, xref, x, dxdxref, values);
          break;
        }
      default:
        {
          for (int i = 0; i < data->components; i++)
            ok = GetSurfValue (data, selnr, xref[0], xref[1], i+1, values[i]);
        }
      }
    return ok;
  }



  bool VisualSceneSolution :: 
  GetSurfValueComplex (const SolData * data, SurfaceElementIndex selnr, 
                       double lam1, double lam2, 
                       int comp, double & valr, double & vali) const
  {
    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          ArrayMem<double,20> values(data->components);
          bool ok;
          
          ok = data->solclass->GetSurfValue (selnr, lam1, lam2, &values[0]);
          
          if (ok)
            {
              if (!data->iscomplex)
                {
                  valr =  values[comp-1];
		  vali = 0;
		}
              else
                {
                  valr = values[comp-1];
                  vali = values[comp];
                }
            }
          
          return ok;
        }
      }
    return 0;
  }
  
  bool VisualSceneSolution :: 
  GetSurfValue (const SolData * data, SurfaceElementIndex selnr, 
                double lam1, double lam2, 
                int comp, double & val) const
  {
    bool ok;
    if (comp == 0)
      {
        val = 0;
        ArrayMem<double,20> values(data->components);
        ok = GetSurfValues (data, selnr, lam1, lam2, &values[0]);
        // ok = 1;
        // values[0] = 1.0;

        switch (evalfunc)
          {
          case FUNC_ABS:
            {
              for (int ci = 0; ci < data->components; ci++)
                val += sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_ABS_TENSOR:
            {
              int d;
              switch (data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]);
              for (ci = d; ci < data->components; ci++)
                val += 2*sqr (values[ci]);
              val = sqrt (val);
              break;
            }

          case FUNC_MISES:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              double trace = 0.;
              for (ci = 0; ci < d; ci++)
                trace += 1./3.*(values[ci]);
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]-trace);
              for (ci = d; ci < data->components; ci++)
                val += 2.*sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_MAIN:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              Mat<3,3> m ;
              Vec<3> ev;
              int ci;
              for (ci = 0; ci < d; ci++)
                m(ci,ci) = (values[ci]);
              m(0,1) = m(1,0) = values[3];
              m(0,2) = m(2,0) = values[4];
              m(1,2) = m(2,1) = values[5];

              EigenValues (m, ev);
              double help;
              for (int i=0; i<d; i++)
                {
                  for (int j=d-1; i<j; j--)
                    {
                      if ( abs(ev(j)) > abs(ev(j-1)) )
                        {
                          help = ev(j);
                          ev(j) = ev(j-1);
                          ev(j-1) = help;
                        }
                    }
                }
              val = (ev(0));
              break;
            }
          }

        return ok;      


        /*
          int ci;
          double val = 0;
          for (ci = 1; ci <= data->components; ci++)
          val += sqr (GetSurfValue (data, selnr, lam1, lam2, ci));
          return sqrt (val);
        */
      }


    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
  
          ArrayMem<double,20> values(data->components);
          bool ok;

          ok = data->solclass->GetSurfValue (selnr, lam1, lam2, &values[0]);

          if (ok)
            {
              if (!data->iscomplex)
                val =  values[comp-1];
              else
                {
                  // cout << "time = " << time << ", cos = " << cos(time) << endl;
     
                  // old version: val = values[comp-1]*cos(3*time) + values[comp]*sin(3*time);
                  // SZ: Sept 06 
                  if(comp%2==0) 
                    val =  values[comp-1]*cos(3*time) - values[comp-2]*sin(3*time);
                  else
                    val = values[comp-1]*cos(3*time) + values[comp]*sin(3*time);
         
         
         
                }
            }

          return ok;
        }


      case SOL_NODAL:
        {
          const Element2d & el = (*mesh)[selnr];

          double lami[8];
          int np, i;
          val = 0;
          double lam3 = 1-lam1-lam2;

          switch (el.GetType())
            {
            case TRIG:
              /*
                lami[0] = lam3;
                lami[1] = lam1;
                lami[2] = lam2;
              */
              lami[0] = lam1;
              lami[1] = lam2;
              lami[2] = lam3;
              np = 3;
              break;

            case TRIG6:
              /*
                lami[0] = lam3*(2*lam3-1);
                lami[1] = lam1*(2*lam1-1);
                lami[2] = lam2*(2*lam2-1);
              */
              // hierarchical basis:
              lami[0] = lam3;
              lami[1] = lam1;
              lami[2] = lam2;
              lami[3] = 4*lam1*lam2;
              lami[4] = 4*lam2*lam3;
              lami[5] = 4*lam1*lam3;
              np = 6;
              break;

            case QUAD:
            case QUAD6:
              lami[0] = (1-lam1)*(1-lam2);
              lami[1] = lam1 * (1-lam2);
              lami[2] = lam1 * lam2;
              lami[3] = (1-lam1) * lam2;
              np = 4;
              break;

            default:
              np = 0;
            }

          for (i = 0; i < np; i++)
            val += lami[i] * data->data[(el[i]-1) * data->dist + comp-1];

          return 1;
        }

      case SOL_ELEMENT:
        {
          int el1, el2;
          mesh->GetTopology().GetSurface2VolumeElement (selnr+1, el1, el2);
          el1--;

          val = data->data[el1 * data->dist+comp-1];
          return 1;
        }

      case SOL_NONCONTINUOUS:
        {
          val = 0;
          // ?????
          return 0;
        }

      case SOL_SURFACE_ELEMENT:
        {
          val = data->data[selnr * data->dist + comp-1];
          return 1;
        }

      case SOL_SURFACE_NONCONTINUOUS:
        {
          const Element2d & el = (*mesh)[selnr];

          double lami[8];
          int np, i;
          val = 0;
          int order = data->order;

          switch (order)
            {
            case 0:
              return data->data[selnr * data->dist + comp-1];
            case 1:
              {
                switch (el.GetType())
                  {
                  case TRIG:
                  case TRIG6:
                    {
                      lami[1] = lam1;
                      lami[2] = lam2;
                      lami[0] = 1-lam1-lam2;
                      np = 3;
                      break;
                    }
                  }
                break;
              }
            case 2:
              {
                switch (el.GetType())
                  {
                  case TRIG:
                    {
                      lami[1] = lam1;
                      lami[2] = lam2;
                      lami[0] = 1-lam1-lam2;
                      np = 3;
                      break;
                    }
                  case TRIG6:
                    {
                      double lam3 = 1-lam1-lam2;
                      lami[1] = 2*lam1 * (lam1-0.5);
                      lami[2] = 2*lam2 * (lam2-0.5);
                      lami[0] = 2*lam3 * (lam3-0.5);
                      lami[3] = 4*lam1*lam2;
                      lami[4] = 4*lam2*lam3;
                      lami[5] = 4*lam1*lam3;
                      np = 6;
                      break;
                    }
                  }
                break;
              }
            }
        
          int base;
          if (order == 1)
            base = 4 * selnr;
          else 
            base = 9 * selnr;

          for (i = 0; i < np; i++)
            {
              val += lami[i] * data->data[(base+i) * data->dist + comp-1];
            }
          return 1;
        }

      case SOL_MARKED_ELEMENTS:
        {
          val = (*mesh)[selnr].TestRefinementFlag();
          return 1;
        }
      
      case SOL_ELEMENT_ORDER:
        {       
          val = (*mesh)[selnr].GetOrder();
          return 1;
        }

      }
    return 0;
  }












  bool VisualSceneSolution :: 
  GetSurfValue (const SolData * data, SurfaceElementIndex selnr,
                const double xref[], const double x[], const double dxdxref[], 
                int comp, double & val) const
  {
    double lam1 = xref[0], lam2 = xref[1];

    bool ok;
    if (comp == 0)
      {
        val = 0;
        ArrayMem<double,20> values(data->components);
        ok = GetSurfValues (data, selnr, xref, x, dxdxref, &values[0]);

        switch (evalfunc)
          {
          case FUNC_ABS:
            {
              for (int ci = 0; ci < data->components; ci++)
                val += sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_ABS_TENSOR:
            {
              int d;
              switch (data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]);
              for (ci = d; ci < data->components; ci++)
                val += 2*sqr (values[ci]);
              val = sqrt (val);
              break;
            }

          case FUNC_MISES:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              int ci;
              double trace = 0.;
              for (ci = 0; ci < d; ci++)
                trace += 1./3.*(values[ci]);
              for (ci = 0; ci < d; ci++)
                val += sqr (values[ci]-trace);
              for (ci = d; ci < data->components; ci++)
                val += 2.*sqr (values[ci]);
              val = sqrt (val);
              break;
            }
          case FUNC_MAIN:
            {
              int d;
              switch(data->components)
                {
                case 1: d = 1; break;
                case 3: d = 2; break;
                case 6: d = 3; break;
                }
              Mat<3,3> m ;
              Vec<3> ev;
              int ci;
              for (ci = 0; ci < d; ci++)
                m(ci,ci) = (values[ci]);
              m(0,1) = m(1,0) = values[3];
              m(0,2) = m(2,0) = values[4];
              m(1,2) = m(2,1) = values[5];

              EigenValues (m, ev);
              double help;
              for (int i=0; i<d; i++)
                {
                  for (int j=d-1; i<j; j--)
                    {
                      if ( abs(ev(j)) > abs(ev(j-1)) )
                        {
                          help = ev(j);
                          ev(j) = ev(j-1);
                          ev(j-1) = help;
                        }
                    }
                }
              val = (ev(0));
              break;
            }
          }

        return ok;      


        /*
          int ci;
          double val = 0;
          for (ci = 1; ci <= data->components; ci++)
          val += sqr (GetSurfValue (data, selnr, lam1, lam2, ci));
          return sqrt (val);
        */
      }


    switch (data->soltype)
      {
      case SOL_VIRTUALFUNCTION:
        {
          ArrayMem<double,20> values(data->components);
          bool ok;

          // ok = data->solclass->GetSurfValue (selnr, lam1, lam2, &values[0]);
          // cout << "data->solclass = " << flush << data->solclass << endl;
          ok = data->solclass->GetSurfValue (selnr, xref, x, dxdxref, &values[0]);
          // ok = 1;
          // values[0] = 1.0;

          if (ok)
            {
              if (!data->iscomplex)
                val =  values[comp-1];
              else
                {
                  // cout << "time = " << time << ", cos = " << cos(time) << endl;
     
                  // old version: val = values[comp-1]*cos(3*time) + values[comp]*sin(3*time);
                  // SZ: Sept 06 
                  if(comp%2==0) 
                    val =  values[comp-1]*cos(3*time) - values[comp-2]*sin(3*time);
                  else
                    val = values[comp-1]*cos(3*time) + values[comp]*sin(3*time);
                  
                }
            }

          return ok;
        }


      case SOL_NODAL:
        {
          const Element2d & el = (*mesh)[selnr];

          double lami[8];
          int np, i;
          val = 0;
          double lam3 = 1-lam1-lam2;

          switch (el.GetType())
            {
            case TRIG:
              /*
                lami[0] = lam3;
                lami[1] = lam1;
                lami[2] = lam2;
              */
              lami[0] = lam1;
              lami[1] = lam2;
              lami[2] = lam3;
              np = 3;
              break;

            case TRIG6:
              /*
                lami[0] = lam3*(2*lam3-1);
                lami[1] = lam1*(2*lam1-1);
                lami[2] = lam2*(2*lam2-1);
              */
              // hierarchical basis:
              lami[0] = lam3;
              lami[1] = lam1;
              lami[2] = lam2;
              lami[3] = 4*lam1*lam2;
              lami[4] = 4*lam2*lam3;
              lami[5] = 4*lam1*lam3;
              np = 6;
              break;

            case QUAD:
            case QUAD6:
              lami[0] = (1-lam1)*(1-lam2);
              lami[1] = lam1 * (1-lam2);
              lami[2] = lam1 * lam2;
              lami[3] = (1-lam1) * lam2;
              np = 4;
              break;

            default:
              np = 0;
            }

          for (i = 0; i < np; i++)
            val += lami[i] * data->data[(el[i]-1) * data->dist + comp-1];

          return 1;
        }

      case SOL_ELEMENT:
        {
          int el1, el2;
          mesh->GetTopology().GetSurface2VolumeElement (selnr+1, el1, el2);
          el1--;

          val = data->data[el1 * data->dist+comp-1];
          return 1;
        }

      case SOL_NONCONTINUOUS:
        {
          val = 0;
          // ?????
          return 0;
        }

      case SOL_SURFACE_ELEMENT:
        {
          val = data->data[selnr * data->dist + comp-1];
          return 1;
        }

      case SOL_SURFACE_NONCONTINUOUS:
        {
          const Element2d & el = (*mesh)[selnr];

          double lami[8];
          int np, i;
          val = 0;
          int order = data->order;

          switch (order)
            {
            case 0:
              return data->data[selnr * data->dist + comp-1];
            case 1:
              {
                switch (el.GetType())
                  {
                  case TRIG:
                  case TRIG6:
                    {
                      lami[1] = lam1;
                      lami[2] = lam2;
                      lami[0] = 1-lam1-lam2;
                      np = 3;
                      break;
                    }
                  }
                break;
              }
            case 2:
              {
                switch (el.GetType())
                  {
                  case TRIG:
                    {
                      lami[1] = lam1;
                      lami[2] = lam2;
                      lami[0] = 1-lam1-lam2;
                      np = 3;
                      break;
                    }
                  case TRIG6:
                    {
                      double lam3 = 1-lam1-lam2;
                      lami[1] = 2*lam1 * (lam1-0.5);
                      lami[2] = 2*lam2 * (lam2-0.5);
                      lami[0] = 2*lam3 * (lam3-0.5);
                      lami[3] = 4*lam1*lam2;
                      lami[4] = 4*lam2*lam3;
                      lami[5] = 4*lam1*lam3;
                      np = 6;
                      break;
                    }
                  }
                break;
              }
            }
        
          int base;
          if (order == 1)
            base = 4 * selnr;
          else 
            base = 9 * selnr;

          for (i = 0; i < np; i++)
            {
              val += lami[i] * data->data[(base+i) * data->dist + comp-1];
            }
          return 1;
        }

      case SOL_MARKED_ELEMENTS:
        {
          val = (*mesh)[selnr].TestRefinementFlag();
          return 1;
        }
      
      case SOL_ELEMENT_ORDER:
        {       
          val = (*mesh)[selnr].GetOrder();
          return 1;
        }

      }
    return 0;
  }









  Vec<3> VisualSceneSolution :: 
  GetDeformation (ElementIndex elnr, const Point<3> & p) const
  {
    Vec<3> def;
    if (deform && vecfunction != -1)
      {
        GetValues (soldata[vecfunction], elnr, p(0), p(1), p(2), &def(0));
        def *= scaledeform;

        if (soldata[vecfunction]->dist == 2) def(2) = 0;
      }
    else
      def = 0;
    return def;
  }


  Vec<3> VisualSceneSolution :: 
  GetSurfDeformation (SurfaceElementIndex elnr, double lam1, double lam2) const
  {
    Vec<3> def;
    if (deform && vecfunction != -1)
      {
        GetSurfValues (soldata[vecfunction], elnr, lam1, lam2,  &def(0));
        def *= scaledeform;

        if (soldata[vecfunction]->dist == 2) def(2) = 0;
      }
    else if (deform && scalfunction != -1 && mesh->GetDimension()==2)
      { // he: allow for 3d plots of 2d surfaces: usage: turn deformation on
        double values[4]; for (int i=0; i<4; i++) values[i]=0.0;
        GetSurfValue (soldata[scalfunction], elnr, lam1, lam2, scalcomp, def(2));
        def *= scaledeform;
      }
    else
      def = 0;
    return def;
  }

  void VisualSceneSolution :: GetPointDeformation (int pnum, Point<3> & p, 
                                                   SurfaceElementIndex elnr) const
  {
    p = mesh->Point (pnum+1);

    if (deform && vecfunction != -1)
      {
        const SolData * vsol = soldata[vecfunction];
      
        Vec<3> v(0,0,0);
        if (vsol->soltype == SOL_NODAL)
          {
            v = Vec3d(vsol->data[pnum * vsol->dist],
                      vsol->data[pnum * vsol->dist+1],
                      vsol->data[pnum * vsol->dist+2]);
          }
        else if (vsol->soltype == SOL_SURFACE_NONCONTINUOUS)
          {
            const Element2d & el = (*mesh)[elnr];
            for (int j = 0; j < el.GetNP(); j++)
              if (el[j] == pnum+1)
                {
                  int base = (4*elnr+j-1) * vsol->dist;
                  v = Vec3d(vsol->data[base],
                            vsol->data[base+1],
                            vsol->data[base+2]);
                }
          }

        if (vsol->dist == 2) v(2) = 0;
      
        v *= scaledeform;
        p += v;
      }
  }




  void VisualSceneSolution :: GetClippingPlaneTrigs (ARRAY<ClipPlaneTrig> & trigs,
                                                     ARRAY<ClipPlanePoint> & pts)
  {
    //static int timer1 = NgProfiler::CreateTimer ("ClipPlaneTrigs1");
    //static int timer2 = NgProfiler::CreateTimer ("ClipPlaneTrigs2");
    //static int timer3a = NgProfiler::CreateTimer ("ClipPlaneTrigs3a");
    //static int timer3b = NgProfiler::CreateTimer ("ClipPlaneTrigs3b");
    //static int timer4 = NgProfiler::CreateTimer ("ClipPlaneTrigs4");


    //NgProfiler::RegionTimer reg1 (timer1);

    int np = mesh->GetNP();
    int ne = mesh->GetNE();

    ARRAY<double> nodevals(np);
  
    for (int i = 0; i < np; i++)
      {
        Point<3> p;
        GetPointDeformation(i, p);
        nodevals[i] =
          p(0) * clipplane[0] +
          p(1) * clipplane[1] +
          p(2) * clipplane[2] +
          clipplane[3];
      }

    const int edgei[6][2] =
      { { 0, 1 }, { 0, 2 }, { 0, 3 },
        { 1, 2 }, { 1, 3 }, { 2, 3 } };

    double edgelam[6];
    Point<3> edgep[6];
    double nodevali[4];

    int cntce;
    int cpe1 = 0, cpe2 = 0, cpe3 = 0;

    ARRAY<Element> loctets;
    ARRAY<Element> loctetsloc;
    ARRAY<Point<3> > pointsloc;

    int n = 1 << subdivisions;
    int n3 = (n+1)*(n+1)*(n+1);

    ARRAY<Point<3> > grid(n3);
    ARRAY<Point<3> > locgrid(n3);
    ARRAY<Mat<3,3> > trans(n3);
    ARRAY<double> val(n3);
    ARRAY<int> compress(n3);

    MatrixFixWidth<3> pointmat(8);

    for (ElementIndex ei = 0; ei < ne; ei++)
      {
        int first_point_of_element = pts.Size();

#ifdef PARALLEL
	// parallel visualization --> dont draw ghost elements
	if ( (*mesh)[ei] . IsGhost() ) continue;
#endif

        //NgProfiler::RegionTimer reg2 (timer2);
	locgrid.SetSize(n3);
        if(vispar.clipdomain > 0 && vispar.clipdomain != (*mesh)[ei].GetIndex()) continue;
        if(vispar.donotclipdomain > 0 && vispar.donotclipdomain == (*mesh)[ei].GetIndex()) continue;

        ELEMENT_TYPE type = (*mesh)[ei].GetType();
        if (type == HEX || type == PRISM || type == TET || type == PYRAMID)
          {
            const Element & el = (*mesh)[ei];

            int ii = 0;
            int cnt_valid = 0;

            for (int ix = 0; ix <= n; ix++)
              for (int iy = 0; iy <= n; iy++)
                for (int iz = 0; iz <= n; iz++, ii++)
                  {
                    Point<3> ploc;
                    compress[ii] = ii;

                    switch (type)
                      {
                      case PRISM:
                        if (ix+iy <= n)
                          {
                            ploc = Point<3> (double(ix) / n, double(iy) / n, double(iz) / n);
                            compress[ii] = cnt_valid;
                            cnt_valid++;
                          }
                        else
                          compress[ii] = -1;
                        break;
                      case TET:
                        if (ix+iy+iz <= n)
                          {
                            ploc = Point<3> (double(ix) / n, double(iy) / n, double(iz) / n);
                            compress[ii] = cnt_valid;
                            cnt_valid++;
                          }
                        else
                          compress[ii] = -1;
                        break;
                      case HEX:
                        ploc = Point<3> (double(ix) / n, double(iy) / n, double(iz) / n);
                        break;
                      case PYRAMID:
                        ploc = Point<3> (double(ix) / n * (1-double(iz)/n),
                                         double(iy) / n * (1-double(iz)/n),
                                         double(iz)/n);
			if (iz == n) ploc = Point<3> (0,0,1-1e-8);
                        break;
                      }
                    if (compress[ii] != -1)
                      locgrid[compress[ii]] = ploc;
                  }

            if (type != TET && type != PRISM) cnt_valid = n3;

	    locgrid.SetSize(cnt_valid);


            if (mesh->GetCurvedElements().IsHighOrder())
              {
                mesh->GetCurvedElements().
                  CalcMultiPointElementTransformation (&locgrid, ei, &grid, 0);
              }
            else
              {
                Vector shape(el.GetNP());
                for (int k = 0; k < el.GetNP(); k++)
                  for (int j = 0; j < 3; j++)
                    pointmat(k,j) = (*mesh)[el[k]](j);
                    
                for (int i = 0; i < cnt_valid; i++)
                  {
                    el.GetShapeNew (locgrid[i], shape);
                    Point<3> pglob;
                    for (int j = 0; j < 3; j++)
                      {
                        pglob(j) = 0;
                        for (int k = 0; k < el.GetNP(); k++)
                          pglob(j) += shape(k) * pointmat(k,j);
                      }
                    grid[i] = pglob;
                  }
              }

            bool has_pos = 0, has_neg = 0;
                
            for (int i = 0; i < cnt_valid; i++)
              {
                val[i] = 
                  grid[i](0) * clipplane[0] + 
                  grid[i](1) * clipplane[1] + 
                  grid[i](2) * clipplane[2] + 
                  clipplane[3];
                    
                if (val[i] > 0)
                  has_pos = 1;
                else
                  has_neg = 1;
              }
                
            if (!has_pos || !has_neg) continue;
                
            //NgProfiler::RegionTimer reg4 (timer4);

            for (int ix = 0; ix < n; ix++)
              for (int iy = 0; iy < n; iy++)
                for (int iz = 0; iz < n; iz++)
                  {
                    int base = iz + (n+1)*iy + (n+1)*(n+1)*ix;
                    int pi[8] = 
                      { base, base+(n+1)*(n+1), base+(n+1)*(n+1)+(n+1), base+(n+1),
                        base+1, base+(n+1)*(n+1)+1, base+(n+1)*(n+1)+(n+1)+1, base+(n+1)+1 };

                    for (int j = 0; j < 8; j++)
                      pi[j] = compress[pi[j]];

                    const int tets[6][4] = 
                      { { 1, 2, 4, 5 },
                        { 4, 5, 2, 8 },
                        { 2, 8, 5, 6 },
                        { 2, 3, 4, 8 },
                        { 2, 3, 8, 6 },
                        { 3, 8, 6, 7 } };

                    for (int ii = 0; ii < 6; ii++)
                      {
                        int teti[4];
                        for (int k = 0; k < 4; k++)
                          teti[k] = pi[tets[ii][k]-1];

                        bool is_valid = 1;
                        for (int j = 0; j < 4; j++)
                          if (teti[j] == -1) is_valid = 0;
                        if (!is_valid) continue;

                        for (int j = 0; j < 4; j++)
                          nodevali[j] = val[teti[j]];
          
                        cntce = 0;
                        for (int j = 0; j < 6; j++)
                          {
                            int lpi1 = edgei[j][0];
                            int lpi2 = edgei[j][1];
                            if ( (nodevali[lpi1] > 0) !=
                                 (nodevali[lpi2] > 0) )
                              {
                                edgelam[j] = nodevali[lpi2] / (nodevali[lpi2] - nodevali[lpi1]);
                                Point<3> p1 = grid[teti[lpi1]];
                                Point<3> p2 = grid[teti[lpi2]];
                  
                                edgep[j] = p1 + (1-edgelam[j]) * (p2-p1);
                  
                                cntce++;
                                cpe3 = cpe2;
                                cpe2 = cpe1;
                                cpe1 = j;
                                if (cntce >= 3)
                                  {
                                    ClipPlaneTrig cpt;
                                    cpt.elnr = ei;
                                  
                                    for (int k = 0; k < 3; k++)
                                      {
                                        int ednr;
                                        switch (k)
                                          {
                                          case 0: ednr = cpe1; break;
                                          case 1: ednr = cpe2; break;
                                          case 2: ednr = cpe3; break;
                                          }
                                        // cpt.points[k].p = edgep[ednr];
                                      
                                        int pi1 = edgei[ednr][0];
                                        int pi2 = edgei[ednr][1];
                                        Point<3> p1 = locgrid[teti[pi1]];
                                        Point<3> p2 = locgrid[teti[pi2]];

                                        // cpt.points[k].lami = p2 + edgelam[ednr] * (p1-p2);

                                        ClipPlanePoint cppt;
                                        cppt.elnr = ei;
                                        cppt.p = edgep[ednr];
                                        cppt.lami =  p2 + edgelam[ednr] * (p1-p2);

                                        int pnr = -1;

                                        for (int l = first_point_of_element; l < pts.Size(); l++)
                                          if (fabs (cppt.lami(0)-pts[l].lami(0)) < 1e-8 &&
                                              fabs (cppt.lami(1)-pts[l].lami(1)) < 1e-8 &&
                                              fabs (cppt.lami(2)-pts[l].lami(2)) < 1e-8)
                                            {
                                              pnr = l;
                                              break;
                                            }

                                        if (pnr == -1)
                                          pnr = pts.Append (cppt)-1;

                                        cpt.points[k].pnr = pnr;
                                        cpt.points[k].locpnr = pnr-first_point_of_element;
                                      }
                                  
                                    trigs.Append (cpt);
                                  }
                              }
                          }
                      }
                  }
          }

        else
          {  // other elements not supported (JS, June 2007)
            return;
            /*
            // const Element & el = mesh->VolumeElement(i);

            (*mesh)[ei].GetTets (loctets);
            (*mesh)[ei].GetTetsLocal (loctetsloc);
            // (*mesh)[ei].GetNodesLocal (pointsloc);
            (*mesh)[ei].GetNodesLocalNew (pointsloc);

            for (int ii = 0; ii < loctets.Size(); ii++)
              {
                const Element & el = loctets[ii];
          
                for (int j = 0; j < 4; j++)
                  nodevali[j] = nodevals.Get(el[j]);
          
                cntce = 0;
                for (int j = 0; j < 6; j++)
                  {
                    int lpi1 = edgei[j][0];
                    int lpi2 = edgei[j][1];
                    if ( (nodevali[lpi1] > 0) !=
                         (nodevali[lpi2] > 0) )
                      {
                        edgelam[j] = nodevali[lpi2] / (nodevali[lpi2] - nodevali[lpi1]);
                        Point<3> p1, p2;
                        GetPointDeformation (el[lpi1]-1, p1);
                        GetPointDeformation (el[lpi2]-1, p2);
                  
                        edgep[j] = p1 + (1-edgelam[j]) * (p2-p1);
                  
                        cntce++;
                        cpe3 = cpe2;
                        cpe2 = cpe1;
                        cpe1 = j;
                        if (cntce >= 3)
                          {
                            ClipPlaneTrig cpt;
                            cpt.elnr = ei;

                            for (int k = 0; k < 3; k++)
                              {
                                int ednr;
                                switch (k)
                                  {
                                  case 0: ednr = cpe1; break;
                                  case 1: ednr = cpe2; break;
                                  case 2: ednr = cpe3; break;
                                  }
                                cpt.points[k].p = edgep[ednr];

                                int pi1 = edgei[ednr][0];
                                int pi2 = edgei[ednr][1];
                                Point<3> p1 = pointsloc.Get (loctetsloc[ii][pi1]);
                                Point<3> p2 = pointsloc.Get (loctetsloc[ii][pi2]);
                                for (int l = 0; l < 3; l++)
                                  cpt.points[k].lami(l) = 
                                    edgelam[ednr]     * p1(l) + 
                                    (1-edgelam[ednr]) * p2(l);
                              }

                            trigs.Append (cpt);
                          }
                      }
                  }
              }
            */
          }
      
      }
  }

  void VisualSceneSolution :: GetClippingPlaneGrid (ARRAY<ClipPlanePoint> & pts)
  {
    int i, j, k;
    int np = mesh->GetNV();
    int ne = mesh->GetNE();
  
    Vec3d n(clipplane[0], clipplane[1], clipplane[2]);

    double mu = -clipplane[3] / n.Length2();
    Point3d p(mu*n.X(), mu * n.Y(), mu * n.Z());

    n /= n.Length();
    Vec3d t1, t2;
    n.GetNormal (t1);
    t2 = Cross (n, t1);

    double xi1, xi2;

    double xi1mid = (center - p) * t1;
    double xi2mid = (center - p) * t2;

    pts.SetSize(0);

    int elnr;
    double lami[3];

    //  cout << "getclippingplanegrid. xoffset = " << xoffset << ", yoffset = ";
    //  cout << yoffset << endl;

    for (xi1 = xi1mid-rad+xoffset/gridsize; xi1 <= xi1mid+rad+xoffset/gridsize; xi1 += rad / gridsize)
      for (xi2 = xi2mid-rad+yoffset/gridsize; xi2 <= xi2mid+rad+yoffset/gridsize; xi2 += rad / gridsize)
        //  for (xi1 = xi1mid-rad; xi1 <= xi1mid+rad; xi1 += rad / gridsize)
        //    for (xi2 = xi2mid-rad; xi2 <= xi2mid+rad; xi2 += rad / gridsize)
        {
          Point3d hp = p + xi1 * t1 + xi2 * t2;

        
          int cindex(-1);
          bool allowindex(true);
          if(vispar.clipdomain > 0)
            {
              cindex = vispar.clipdomain;
            }
          else if(vispar.donotclipdomain > 0)
            {
              allowindex = false;
              cindex = vispar.donotclipdomain;
            }
          elnr = mesh->GetElementOfPoint (hp, lami,0,cindex,allowindex)-1;

          if (elnr != -1)
            {
              ClipPlanePoint cpp;
              cpp.p = hp;
              cpp.elnr = elnr;
              cpp.lami(0) = lami[0];
              cpp.lami(1) = lami[1];
              cpp.lami(2) = lami[2];
              pts.Append (cpp);
            }
        }
  };



  void VisualSceneSolution ::
  SetOpenGlColor(double val)
  {
    if (usetexture == 1 && !logscale)
      {
        glColor3d (1, 1, 1);
        glTexCoord1f ( val );
        return;
      }

    double valmin = minval;
    double valmax = maxval;

    double value;

    if (!logscale)
      value = (val - valmin) / (valmax - valmin);
    else
      {
        if (valmax <= 0) valmax = 1;
        if (valmin <= 0) valmin = 1e-4 * valmax;
        value = (log(fabs(val)) - log(valmin)) / (log(valmax) - log(valmin));
      }

    if (!invcolor)
      value = 1 - value;

    if (usetexture)
      {
        glColor3d (1, 1, 1);
        glTexCoord1f ( 0.999 * value + 0.001);
        return;
      };


    if (value > 1) value = 1;
    if (value < 0) value = 0;

    value *= 4;

    static const double colp[][3] =
      {
        { 1, 0, 0 },
        { 1, 1, 0 },
        { 0, 1, 0 },
        { 0, 1, 1 },
        { 0, 0, 1 },
        { 1, 0, 1 },
        { 1, 0, 0 },
      };
  
    int i = int(value);
    double r = value - i;

    GLdouble col[3];
    for (int j = 0; j < 3; j++)
      col[j] = (1-r) * colp[i][j] + r * colp[i+1][j];
  
    glColor3d (col[0], col[1], col[2]);
  }


  void VisualSceneSolution ::
  SetOpenGlColor(double h, double valmin, double valmax, int logscale)
  {
    if (usetexture == 1 && !logscale)
      {
        glColor3d (1, 1, 1);
        glTexCoord1f ( h );
        return;
      }


    double value;

    if (!logscale)
      value = (h - valmin) / (valmax - valmin);
    else
      {
        if (valmax <= 0) valmax = 1;
        if (valmin <= 0) valmin = 1e-4 * valmax;
        value = (log(fabs(h)) - log(valmin)) / (log(valmax) - log(valmin));
      }

    if (!invcolor)
      value = 1 - value;

    if (usetexture)
      {
        glColor3d (1, 1, 1);
        glTexCoord1f ( 0.999 * value + 0.001);
        return;
      };


    if (value > 1) value = 1;
    if (value < 0) value = 0;

    value *= 4;

    static const double colp[][3] =
      {
        { 1, 0, 0 },
        { 1, 1, 0 },
        { 0, 1, 0 },
        { 0, 1, 1 },
        { 0, 0, 1 },
        { 1, 0, 1 },
        { 1, 0, 0 },
      };
  
    int i = int(value);
    double r = value - i;

    GLdouble col[3];
    for (int j = 0; j < 3; j++)
      col[j] = (1-r) * colp[i][j] + r * colp[i+1][j];
  
    glColor3d (col[0], col[1], col[2]);
  }




  void VisualSceneSolution ::
  DrawCone (const Point<3> & p1, const Point<3> & p2, double r)
  {
    int n = 10, i;
    Vec<3> p1p2 = p2 - p1;

    p1p2.Normalize();
    Vec<3> p2p1 = -p1p2;

    Vec<3> t1 = p1p2.GetNormal();
    Vec<3> t2 = Cross (p1p2, t1);

    Point<3> oldp = p1 + r * t1;
    Vec<3> oldn = t1;

    Point<3> p;
    Vec<3> normal;

    Mat<2> rotmat;
    Vec<2> cs, newcs;
    cs(0) = 1;
    cs(1) = 0;
    rotmat(0,0) = rotmat(1,1) = cos(2*M_PI/n);
    rotmat(1,0) = sin(2*M_PI/n);
    rotmat(0,1) = -rotmat(1,0);

    glBegin (GL_TRIANGLES);
    double phi;
    for (i = 1; i <= n; i++)
      {
        /*
          phi = 2 * M_PI * i / n;
          normal = cos(phi) * t1 + sin(phi) * t2;
        */
        newcs = rotmat * cs;
        cs = newcs;
        normal = cs(0) * t1 + cs(1) * t2;

        p = p1 + r * normal;

        // cone
        glNormal3dv (normal);
        glVertex3dv (p);
        glVertex3dv (p2);
        glNormal3dv (oldn);
        glVertex3dv (oldp);

        // base-circle
        glNormal3dv (p2p1);
        glVertex3dv (p);
        glVertex3dv (p1);
        glVertex3dv (oldp);

        oldp = p;
        oldn = normal;
      }
    glEnd ();
  }



  void VisualSceneSolution ::
  DrawCylinder (const Point<3> & p1, const Point<3> & p2, double r)
  {
    int n = 10, i;
    Vec<3> p1p2 = p2 - p1;

    p1p2.Normalize();
    Vec<3> p2p1 = -p1p2;

    Vec<3> t1 = p1p2.GetNormal();
    Vec<3> t2 = Cross (p1p2, t1);

    Point<3> oldhp1 = p1 + r * t1;
    Point<3> oldhp2 = p2 + r * t1;
    Vec<3> oldn = t1;

    Point<3> hp1, hp2;
    Vec<3> normal;

    Mat<2> rotmat;
    Vec<2> cs, newcs;
    cs(0) = 1;
    cs(1) = 0;
    rotmat(0,0) = rotmat(1,1) = cos(2*M_PI/n);
    rotmat(1,0) = sin(2*M_PI/n);
    rotmat(0,1) = -rotmat(1,0);

    glBegin (GL_QUADS);
    double phi;
    for (i = 1; i <= n; i++)
      {
        newcs = rotmat * cs;
        cs = newcs;
        normal = cs(0) * t1 + cs(1) * t2;

        hp1 = p1 + r * normal;
        hp2 = p2 + r * normal;

        // cylinder
        glNormal3dv (normal);

        glVertex3dv (hp1);
        glVertex3dv (hp2);
        glVertex3dv (oldhp2);
        glVertex3dv (oldhp1);

        oldhp1 = hp1;
        oldhp2 = hp2;
        oldn = normal;
      }
    glEnd ();
  }













  void VisualSceneSolution :: MouseDblClick (int px, int py)
  {
    vsmesh.SetClippingPlane();
    vsmesh.BuildFilledList();
    vsmesh.MouseDblClick(px,py);
  }


  void VisualSceneSolution :: 
  DrawClipPlaneTrigs (const SolData * sol, int comp,
                      const ARRAY<ClipPlaneTrig> & trigs, 
                      const ARRAY<ClipPlanePoint> & points)
  {
    int maxlpnr = 0;
    for (int i = 0; i < trigs.Size(); i++)
      for (int j = 0; j < 3; j++)
        maxlpnr = max2 (maxlpnr, trigs[i].points[j].locpnr);

    ArrayMem<double, 1000> vals(maxlpnr+1);
    ArrayMem<int, 1000> elnrs(maxlpnr+1);
    ArrayMem<bool, 1000> trigok(maxlpnr+1);
    trigok = false;
    elnrs = -1;

    Point<3> p[3];
    double val[3],vali[3];
    bool ok;

    for (int i = 0; i < trigs.Size(); i++)
      {
        const ClipPlaneTrig & trig = trigs[i];
	ok = true;
        for (int j = 0; ok && j < 3; j++)
          {
            p[j] = points[trig.points[j].pnr].p;
            Point<3> ploc = points[trig.points[j].pnr].lami;
            
            if (deform)
              p[j] += GetDeformation (trig.elnr, ploc);
            
            if (usetexture != 2 || !sol->iscomplex)
              {
                if (elnrs[trig.points[j].locpnr] != trig.elnr)
                  {
                    elnrs[trig.points[j].locpnr] = trig.elnr;

                    Point<3> pglob;
                    Mat<3> trans;
                    
                    mesh->GetCurvedElements().
                      CalcElementTransformation (ploc, trig.elnr, pglob, trans);
                  
                    //double val;
		    ok = GetValue (sol, trig.elnr, &ploc(0), &pglob(0), &trans(0,0), scalcomp, val[j]);
                    vals[trig.points[j].locpnr] = val[j];
		    trigok[trig.points[j].locpnr] = ok;
		  }
                else
                  {   
		    ok = trigok[trig.points[j].locpnr];
                    //SetOpenGlColor (vals[trig.points[j].locpnr]);
                  }
              }
            else
              {
                //double valr, vali;
                ok = GetValueComplex (sol, trig.elnr, ploc(0), ploc(1), ploc(2),
				      scalcomp, val[j], vali[j]);
                
                //glTexCoord2f ( valr, vali );
              }
            //glVertex3dv (p);
          }
	if(ok)
	  for(int j=0; j<3; j++)
	    {
	      if (usetexture != 2 || !sol->iscomplex)
		{
		  if (elnrs[trig.points[j].locpnr] != trig.elnr)
		    SetOpenGlColor(val[j]);
		  else
		    SetOpenGlColor (vals[trig.points[j].locpnr]);
		}
	      else
		glTexCoord2f ( val[j], vali[j] );

	      glVertex3dv (p[j]);
	    }

      }
  }





  int Ng_Vis_Set (ClientData clientData,
                  Tcl_Interp * interp,
                  int argc, tcl_const char *argv[])

  {
    int i;
    if (argc >= 2)
      {
        if (strcmp (argv[1], "parameters") == 0)
          {
            vssolution.imag_part = 
                atoi (Tcl_GetVar (interp, "::visoptions.imaginary", TCL_GLOBAL_ONLY));      
            vssolution.usetexture = 
                atoi (Tcl_GetVar (interp, "::visoptions.usetexture", TCL_GLOBAL_ONLY));
            if (atoi (Tcl_GetVar (interp, "::visoptions.redrawperiodic", TCL_GLOBAL_ONLY)))
              vssolution.usetexture = 2;
                
            vssolution.invcolor = 
                atoi (Tcl_GetVar (interp, "::visoptions.invcolor", TCL_GLOBAL_ONLY));       

            vssolution.clipsolution = 0;

            if (strcmp (Tcl_GetVar (interp, "::visoptions.clipsolution", TCL_GLOBAL_ONLY), 
                        "scal") == 0)
              vssolution.clipsolution = 1;
            if (strcmp (Tcl_GetVar (interp, "::visoptions.clipsolution", TCL_GLOBAL_ONLY), 
                        "vec") == 0)
              vssolution.clipsolution = 2;
            
            tcl_const char * scalname =  (tcl_const char *)
                Tcl_GetVar (interp, "::visoptions.scalfunction", TCL_GLOBAL_ONLY);
            tcl_const char * vecname =  (tcl_const char *)
                Tcl_GetVar (interp, "::visoptions.vecfunction", TCL_GLOBAL_ONLY);
            tcl_const char * fieldlines_vecname =  (tcl_const char *)
                Tcl_GetVar (interp, "::visoptions.fieldlinesvecfunction", TCL_GLOBAL_ONLY);
                
          
            vssolution.scalfunction = -1;
            vssolution.vecfunction = -1;
            vssolution.fieldlines_vecfunction = -1;

            int pointpos; // SZ 
            const char * pch = strchr(scalname,'.');
            pointpos = int(pch-scalname+1);

            for (i = 0; i < vssolution.soldata.Size(); i++)
              {
                if (strlen (vssolution.soldata[i]->name) ==
                    pointpos-1 &&
                    strncmp (vssolution.soldata[i]->name, scalname,
                             pointpos-1) == 0)
                  {
                    vssolution.scalfunction = i;
                    vssolution.scalcomp = atoi (scalname + pointpos);
		    if ( vssolution.scalcomp > vssolution.soldata[i]->components )
			vssolution.scalcomp = 1;
		    char newscalname[100];
		    for ( int ii = 0; ii < pointpos; ii++ )
		      newscalname[ii] = scalname[ii];
		    newscalname[pointpos] = '.';
		    sprintf (newscalname+pointpos, "%i", vssolution.scalcomp);

                    if (strcmp (scalname, newscalname) != 0)
                       Tcl_SetVar ( interp, "::visoptions.scalfunction", newscalname, TCL_GLOBAL_ONLY );

                  }
                if (strcmp (vssolution.soldata[i]->name, vecname) == 0)
                  {
                    vssolution.vecfunction = i;
                    //cout  << "set vecfunction to " << i << endl;
                  }
                if (strcmp (vssolution.soldata[i]->name, fieldlines_vecname) == 0)
                  {
                    vssolution.fieldlines_vecfunction = i;
                    //cout  << "set fieldlines-vecfunction to " << i << endl;
                  }
              }

            if(vssolution.fieldlines_vecfunction != -1 &&
               vssolution.vecfunction == -1)
              {
                //cout << "WARNING: Setting vector function in Visualization toolbox to value from Fieldlines toolbox!" << endl;
                vssolution.vecfunction = vssolution.fieldlines_vecfunction;
              }
               
	    // reset visoptions.scalfunction and visoptions.vecfunction if not avialable 
	    if ( vssolution.scalfunction == -1 && strcmp (scalname, "none") != 0)
              Tcl_SetVar ( interp, "::visoptions.scalfunction", "none", TCL_GLOBAL_ONLY );
	    if ( vssolution.vecfunction == -1  && strcmp (vecname, "none") != 0)
              Tcl_SetVar ( interp, "::visoptions.vecfunction", "none", TCL_GLOBAL_ONLY );

            tcl_const char * evalname =  (tcl_const char *)
                Tcl_GetVar (interp, "::visoptions.evaluate", TCL_GLOBAL_ONLY);
          
            if (strcmp(evalname, "abs") == 0) vssolution.evalfunc = VisualSceneSolution::FUNC_ABS;
            if (strcmp(evalname, "abstens") == 0) vssolution.evalfunc = VisualSceneSolution::FUNC_ABS_TENSOR;
            if (strcmp(evalname, "mises") == 0) vssolution.evalfunc = VisualSceneSolution::FUNC_MISES;
            if (strcmp(evalname, "main") == 0) vssolution.evalfunc = VisualSceneSolution::FUNC_MAIN;

            vssolution.gridsize = 
                atoi (Tcl_GetVar (interp, "::visoptions.gridsize", TCL_GLOBAL_ONLY));

            vssolution.xoffset = 
                atof (Tcl_GetVar (interp, "::visoptions.xoffset", TCL_GLOBAL_ONLY));

            //    cout << "x-offset:" << vssolution.xoffset << endl;

            vssolution.yoffset = 
                atof (Tcl_GetVar (interp, "::visoptions.yoffset", TCL_GLOBAL_ONLY));

            vssolution.autoscale = 
                atoi (Tcl_GetVar (interp, "::visoptions.autoscale", TCL_GLOBAL_ONLY));


            /*
              vssolution.linear_colors = 
            atoi (Tcl_GetVar (interp, "::visoptions.lineartexture", TCL_GLOBAL_ONLY));
            */
            vssolution.logscale = 
                atoi (Tcl_GetVar (interp, "::visoptions.logscale", TCL_GLOBAL_ONLY));

            vssolution.mminval = 
                atof (Tcl_GetVar (interp, "::visoptions.mminval", TCL_GLOBAL_ONLY));
            vssolution.mmaxval = 
                atof (Tcl_GetVar (interp, "::visoptions.mmaxval", TCL_GLOBAL_ONLY));

            vssolution.showclipsolution = 
                atoi (Tcl_GetVar (interp, "::visoptions.showclipsolution", TCL_GLOBAL_ONLY));
            vssolution.showsurfacesolution = 
                atoi (Tcl_GetVar (interp, "::visoptions.showsurfacesolution", TCL_GLOBAL_ONLY));
            vssolution.lineartexture = 
                atoi (Tcl_GetVar (interp, "::visoptions.lineartexture", TCL_GLOBAL_ONLY));
            vssolution.numtexturecols = 
                atoi (Tcl_GetVar (interp, "::visoptions.numtexturecols", TCL_GLOBAL_ONLY));

            vssolution.multidimcomponent = 
                atoi (Tcl_GetVar (interp, "::visoptions.multidimcomponent", TCL_GLOBAL_ONLY));

	    vssolution.drawpointcurves = 
	      atoi (Tcl_GetVar (interp, "::visoptions.drawpointcurves", TCL_GLOBAL_ONLY));	      

            vssolution.draw_fieldlines = 
	      atoi (Tcl_GetVar (interp, "::visoptions.drawfieldlines", TCL_GLOBAL_ONLY));
            vssolution.num_fieldlines = 
                atoi (Tcl_GetVar (interp, "::visoptions.numfieldlines", TCL_GLOBAL_ONLY));
            vssolution.fieldlines_randomstart =
                atoi (Tcl_GetVar (interp, "::visoptions.fieldlinesrandomstart", TCL_GLOBAL_ONLY));

            vssolution.fieldlines_reltolerance =
                atof (Tcl_GetVar (interp, "::visoptions.fieldlinestolerance", TCL_GLOBAL_ONLY));

            if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesrktype", TCL_GLOBAL_ONLY), 
                        "euler") == 0)
              vssolution.fieldlines_rktype = 0;
            else if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesrktype", TCL_GLOBAL_ONLY), 
                             "eulercauchy") == 0)
              vssolution.fieldlines_rktype = 1;
            else if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesrktype", TCL_GLOBAL_ONLY), 
                             "simpson") == 0)
              vssolution.fieldlines_rktype = 2;
            else if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesrktype", TCL_GLOBAL_ONLY), 
                             "crungekutta") == 0)
              vssolution.fieldlines_rktype = 3;


            vssolution.fieldlines_rellength =
                atof (Tcl_GetVar (interp, "::visoptions.fieldlineslength", TCL_GLOBAL_ONLY));

            vssolution.fieldlines_maxpoints = 
                atoi (Tcl_GetVar (interp, "::visoptions.fieldlinesmaxpoints", TCL_GLOBAL_ONLY));

            vssolution.fieldlines_relthickness =
                atof (Tcl_GetVar (interp, "::visoptions.fieldlinesthickness", TCL_GLOBAL_ONLY));


            vssolution.fieldlines_fixedphase = 
                (atoi (Tcl_GetVar (interp, "::visoptions.fieldlinesonlyonephase", TCL_GLOBAL_ONLY)) != 0);

            if(vssolution.fieldlines_fixedphase)
              vssolution.fieldlines_phase =
                  atof (Tcl_GetVar (interp, "::visoptions.fieldlinesphase", TCL_GLOBAL_ONLY));


            if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesstartarea", TCL_GLOBAL_ONLY), 
                        "box") == 0)
              vssolution.fieldlines_startarea  = 0;
            else if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesstartarea", TCL_GLOBAL_ONLY), 
                             "file") == 0)
              vssolution.fieldlines_startarea  = 1;
            else if (strcmp (Tcl_GetVar (interp, "::visoptions.fieldlinesstartarea", TCL_GLOBAL_ONLY), 
                             "face") == 0)
              vssolution.fieldlines_startarea  = 2;

                
            if (vssolution.fieldlines_startarea == 0)
              {
                vssolution.fieldlines_startarea_parameter.SetSize(6);
                vssolution.fieldlines_startarea_parameter[0] = atof (Tcl_GetVar (interp, "::visoptions.fieldlinesstartareap1x", TCL_GLOBAL_ONLY));
                vssolution.fieldlines_startarea_parameter[1] = atof (Tcl_GetVar (interp, "::visoptions.fieldlinesstartareap1y", TCL_GLOBAL_ONLY));
                vssolution.fieldlines_startarea_parameter[2] = atof (Tcl_GetVar (interp, "::visoptions.fieldlinesstartareap1z", TCL_GLOBAL_ONLY));
                vssolution.fieldlines_startarea_parameter[3] = atof (Tcl_GetVar (interp, "::visoptions.fieldlinesstartareap2x", TCL_GLOBAL_ONLY));
                vssolution.fieldlines_startarea_parameter[4] = atof (Tcl_GetVar (interp, "::visoptions.fieldlinesstartareap2y", TCL_GLOBAL_ONLY));
                vssolution.fieldlines_startarea_parameter[5] = atof (Tcl_GetVar (interp, "::visoptions.fieldlinesstartareap2z", TCL_GLOBAL_ONLY));
              }
            else if (vssolution.fieldlines_startarea == 1)
              {
                vssolution.fieldlines_filename = Tcl_GetVar (interp, "::visoptions.fieldlinesfilename", TCL_GLOBAL_ONLY);
              }
            else if (vssolution.fieldlines_startarea == 2)
              {
                vssolution.fieldlines_startface = atoi (Tcl_GetVar (interp, "::visoptions.fieldlinesstartface", TCL_GLOBAL_ONLY));
              }

          
            vssolution.deform =
                atoi (Tcl_GetVar (interp, "::visoptions.deformation", TCL_GLOBAL_ONLY));
            vssolution.scaledeform =
                atof (Tcl_GetVar (interp, "::visoptions.scaledeform1", TCL_GLOBAL_ONLY)) *
                atof (Tcl_GetVar (interp, "::visoptions.scaledeform2", TCL_GLOBAL_ONLY));


            if (atoi (Tcl_GetVar (interp, "::visoptions.isolines", TCL_GLOBAL_ONLY)))
              vssolution.numisolines = atoi (Tcl_GetVar (interp, "::visoptions.numiso", TCL_GLOBAL_ONLY));
            else
              vssolution.numisolines = 0;
            vssolution.draw_isosurface = 
                atoi (Tcl_GetVar (interp, "::visoptions.isosurf", TCL_GLOBAL_ONLY));

            vssolution.subdivisions = 
                atoi (Tcl_GetVar (interp, "::visoptions.subdivisions", TCL_GLOBAL_ONLY));
            vssolution.UpdateSolutionTimeStamp();
          }
      
        if (strcmp (argv[1], "parametersrange") == 0)
          {
            vssolution.invcolor = 
                atoi (Tcl_GetVar (interp, "::visoptions.invcolor", TCL_GLOBAL_ONLY));       
            vssolution.mminval = 
                atof (Tcl_GetVar (interp, "::visoptions.mminval", TCL_GLOBAL_ONLY));
            vssolution.mmaxval = 
                atof (Tcl_GetVar (interp, "::visoptions.mmaxval", TCL_GLOBAL_ONLY));
            vssolution.lineartexture = 
                atoi (Tcl_GetVar (interp, "::visoptions.lineartexture", TCL_GLOBAL_ONLY));
            vssolution.numtexturecols = 
                atoi (Tcl_GetVar (interp, "::visoptions.numtexturecols", TCL_GLOBAL_ONLY));

            if (vssolution.usetexture == 0 || vssolution.logscale)
              vssolution.UpdateSolutionTimeStamp();
          }


        if (argc >= 3 && strcmp (argv[1], "time") == 0)
          {
            vssolution.time = double (atoi (argv[2])) / 1000;
         
            vssolution.timetimestamp = NextTimeStamp();
            cout << "\rtime = " << vssolution.time << "    " << flush;
          }

       }
    return TCL_OK;
  }

  int Ng_Vis_Field (ClientData clientData,
                    Tcl_Interp * interp,
                    int argc, tcl_const char *argv[])
  {
    int i;
    static char buf[1000];
    buf[0] = 0;

    if (argc >= 2)
      {
        if (strcmp (argv[1], "setfield") == 0)
          {
            if (argc < 3)
              return TCL_ERROR;

            for (i = 0; i < vssolution.GetNSolData(); i++)
              if (strcmp (vssolution.GetSolData(i)->name, argv[2]) == 0)
                {
                  cout << "found soldata " << i << endl;
                }
          }

        if (strcmp (argv[1], "getnfieldnames") == 0)
          {
            sprintf (buf, "%d", vssolution.GetNSolData());
          }
      
        if (strcmp (argv[1], "getfieldname") == 0)
          {
            sprintf (buf, "%s", vssolution.GetSolData(atoi(argv[2])-1)->name);
          }

        if (strcmp (argv[1], "iscomplex") == 0)
          {
            sprintf (buf, "%d", vssolution.GetSolData(atoi(argv[2])-1)->iscomplex);
          }

        if (strcmp (argv[1], "getfieldcomponents") == 0)
          {
            sprintf (buf, "%d", vssolution.GetSolData(atoi(argv[2])-1)->components);
          }

      
        if (strcmp (argv[1], "getfieldnames") == 0)
          {
            for (i = 0; i < vssolution.GetNSolData(); i++)
              {
                strcat (buf, vssolution.GetSolData(i)->name);
                strcat (buf, " ");
              }
            strcat (buf, "var1 var2 var3");
            Tcl_SetResult (interp, buf, TCL_STATIC);
          }

        if (strcmp (argv[1], "setcomponent") == 0)
          {
            cout << "set component " << argv[2] << endl;
          }

        if (strcmp (argv[1], "getactivefield") == 0)
          {
            sprintf (buf, "1");
          }

        if (strcmp (argv[1], "getdimension") == 0)
          {
            sprintf (buf, "%d", mesh->GetDimension());
          }
      }

    Tcl_SetResult (interp, buf, TCL_STATIC);
    return TCL_OK;
  }


  extern "C" int Ng_Vis_Init (Tcl_Interp * interp);

  int Ng_Vis_Init (Tcl_Interp * interp)
  {
    Tcl_CreateCommand (interp, "Ng_Vis_Set", Ng_Vis_Set,
                       (ClientData)NULL,
                       (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateCommand (interp, "Ng_Vis_Field", Ng_Vis_Field,
                       (ClientData)NULL,
                       (Tcl_CmdDeleteProc*) NULL);


    return TCL_OK;
  }
}