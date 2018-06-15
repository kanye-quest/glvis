#include <fstream>
#include <string>

#include "mfem.hpp"
#include "lib/palettes.hpp"
#include "lib/visual.hpp"

string plot_caption, extra_caption;
string keys;
Mesh * mesh;
GridFunction* grid_f;
VisualizationSceneScalarData *vs = NULL;
Vector sol, solu, solv, solw, normals;
bool fix_elem_orient = false;
GeometryRefiner GLVisGeometryRefiner;
void Extrude1DMeshAndSolution(Mesh **mesh_p, GridFunction **grid_f_p,
                              Vector *sol)
{
   Mesh *mesh = *mesh_p;

   if (mesh->Dimension() != 1 || mesh->SpaceDimension() != 1)
   {
      return;
   }

   // find xmin and xmax over the vertices of the 1D mesh
   double xmin = numeric_limits<double>::infinity();
   double xmax = -xmin;
   for (int i = 0; i < mesh->GetNV(); i++)
   {
      const double x = mesh->GetVertex(i)[0];
      if (x < xmin)
      {
         xmin = x;
      }
      if (x > xmax)
      {
         xmax = x;
      }
   }

   Mesh *mesh2d = Extrude1D(mesh, 1, 0.1*(xmax - xmin));

   if (grid_f_p && *grid_f_p)
   {
      GridFunction *grid_f_2d =
         Extrude1DGridFunction(mesh, mesh2d, *grid_f_p, 1);

      delete *grid_f_p;
      *grid_f_p = grid_f_2d;
   }
   if (sol && sol->Size() == mesh->GetNV())
   {
      Vector sol2d(mesh2d->GetNV());
      for (int i = 0; i < mesh->GetNV(); i++)
      {
         sol2d(2*i+0) = sol2d(2*i+1) = (*sol)(i);
      }
      *sol = sol2d;
   }

   delete mesh;
   *mesh_p = mesh2d;
}

int main()
{
   int         geom_ref_type = Quadrature1D::ClosedUniform;
   GLVisGeometryRefiner.SetType(geom_ref_type);
    InitVisualization("===Emscripten Test===", 0,0, 800, 600);
    ifstream ifs("glvis-saved.0001");
    string type_temp;
    ifs >> type_temp;
    int field_type = 0;
    mesh = new Mesh(ifs, 1, 0, fix_elem_orient);
    grid_f = new GridFunction(mesh, ifs);
    field_type = (grid_f->VectorDim() == 1) ? 0 : 1;
    if (field_type >= 0 && field_type <= 2)
    {
        if (grid_f)
        {
            Extrude1DMeshAndSolution(&mesh, &grid_f, NULL);
        }
    }

   double mesh_range = -1.0;
   if (field_type == 0 || field_type == 2)
   {
      if (grid_f)
      {
         grid_f->GetNodalValues(sol);
      }
      if (mesh->SpaceDimension() == 2)
      {
         VisualizationSceneSolution *vss;
         if (field_type == 2)
         {
            paletteSet(4);
         }
         if (normals.Size() > 0)
         {
            vs = vss = new VisualizationSceneSolution(*mesh, sol, &normals);
         }
         else
         {
            vs = vss = new VisualizationSceneSolution(*mesh, sol);
         }
         if (grid_f)
         {
            vss->SetGridFunction(*grid_f);
         }
         if (field_type == 2)
         {
            vs->OrthogonalProjection = 1;
            vs->light = 0;
            vs->Zoom(1.8);
         }
      }
   }

    if (vs)
    {
       // increase the refinement factors if visualizing a GridFunction
       if (grid_f)
       {
          vs->AutoRefine();
          vs->SetShading(2, true);
       }
       if (mesh_range > 0.0)
       {
          vs->SetValueRange(-mesh_range, mesh_range);
          vs->SetAutoscale(0);
       }
       if (mesh->SpaceDimension() == 2 && field_type == 2)
       {
          SetVisualizationScene(vs, 2, keys.c_str());
       }
       else
       {
          SetVisualizationScene(vs, 3, keys.c_str());
       }
    }

    cout << "End program";
    //KillVisualization(); // deletes vs
    return 0;
}
