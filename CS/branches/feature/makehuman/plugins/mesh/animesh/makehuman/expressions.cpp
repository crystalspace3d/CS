/*
    Copyright (C) 2012-2013 by Anthony Legrand
    Copyright (C) 2013 by Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "cssysdef.h"
#include "iutil/stringarray.h"
#include "character.h"
#include "qr_solve.h"

CS_PLUGIN_NAMESPACE_BEGIN (Makehuman)
{

/*-------------------------------------------------------------------------*
 * MakeHuman facial expressions
 *-------------------------------------------------------------------------*/

bool MakehumanCharacter::GenerateMacroExpressions (const ModelTargets& modelVals,
						   csArray<Target>& macroExpr)
{
  // Generate name and weight of expression folders
  printf ("\nGenerating macro-expressions:\n");
  csString name;
  float weight;
  csArray<csString> subdirNames;
  csArray<float> subdirWeights;

  for (size_t i=0; i< modelVals.gender.GetSize (); i++)
    for (size_t j=0; j< modelVals.age.GetSize (); j++)
    {
      name = modelVals.gender[i].name;
      name.Append (modelVals.age[j].name);
      name.ReplaceAll ("-", "_");
      weight = modelVals.gender[i].weight * modelVals.age[j].weight;
      subdirNames.Push (name);
      subdirWeights.Push (weight);
      printf ("  - from model '%s' with weight %.3f\n", name.GetData (), weight);
    }

  if (subdirNames.GetSize () == 0)
    return ReportError ("Didn't find any target file to generate model expressions");

  // Find expressions in Makehuman directories
  printf ("\nParsing model macro expressions (from subfolders of '%s'):\n", EXPRESSIONS_PATH);
  csString targetname, filename, exprname;
  size_t index;
  csRef<iVFS> VFS = csQueryRegistry<iVFS> (manager->objectRegistry);
  // use any EXPRESSIONS_PATH subdirectories because they all contain the same expressions
  csString dir = csString (EXPRESSIONS_PATH).Append (subdirNames[0]).Append ("/");

  if (!VFS->Exists (dir.GetData ()))
    return ReportError ("Didn't find Makehuman macro-expressions directory '%s' (deprecated)", dir.GetData ());

  csRef<iStringArray> expressionFiles = VFS->FindFiles (dir);
  expressionFiles->Sort ();

  // Init list of expression offsets
  for (size_t i = 0; i < expressionFiles->GetSize (); i++)
  {
    // Get expression name from filename
    csString targetfile = csString (expressionFiles->Get (i));
    index = targetfile.FindLast ('_');
    targetfile.SubString (filename, index + 1);
    index = filename.FindLast ('.');
    filename.SubString (exprname, 0, index);

    // Print expression name
    if (i!=0 && i%6==0) printf ("\n"); 
    printf ("%3i. ", (int)i+1); printf ("%-13s", exprname.GetData ());

    // Create a new expression
    Target newExpr (exprname.GetData (), nullptr, 0.0f);
    macroExpr.Push (newExpr);
  }
  printf ("\n\n");

  for (size_t i = 0; i < modelVals.ethnics.GetSize (); i++)
    if (modelVals.ethnics[i].name == "neutral" && modelVals.ethnics[i].weight < 1)
      printf ("WARNING: Generated macro-expressions are only adapted to 100%% Caucasian models\n"\
              "         while this model is partly African or Asian.\n\n");

  // Parse expression files and generate an offset buffer for each expression
  for (size_t i = 0; i < macroExpr.GetSize (); i++)
  {
    csHash<size_t, size_t> indexMapping;

    for (size_t j = 0; j < subdirNames.GetSize (); j++)
    {
      // Parse the expression (i.e. a target file) of the Makehuman target folder
      // Generate path of Makehuman target file
      filename = csString (EXPRESSIONS_PATH) + subdirNames[j];
      filename += csString ("/neutral_") + subdirNames[j] + csString ("_");
      filename += macroExpr[i].name + csString (".target");

      // Parse target file into a temporary offset buffer
      csArray<csVector3> offsets;
      csArray<size_t> indices;
      if (!ParseMakehumanTargetFile (filename.GetData (), offsets, indices))
        return ReportError ("Could not parse the Makehuman target file %s", filename.GetData ());

      // Cumulate displacement in the offset buffer of the expression
      float weight = subdirWeights[j];
      for (size_t k = 0; k < offsets.GetSize (); k++)
      {
	// Try to find the vertex in the current list
	size_t* index = indexMapping.GetElementPointer (indices[k]);
	if (index)
	  macroExpr[i].offsets[*index] += weight * offsets[k];

	else
	{
	  macroExpr[i].indices.Push (indices[k]);
	  macroExpr[i].offsets.Push (weight * offsets[k]);
	  indexMapping.Put (indices[k], macroExpr[i].indices.GetSize () - 1);
	}
      }
    }
  }

  return true;
}

bool MakehumanCharacter::ParseLandmarks (const char* bodypart, csArray<size_t>& landmarks)
{
  // Get path of landmarks file
  csString filename (LANDMARKS_PATH);
  filename.Append (bodypart).Append (".lmk");

  // Open file
  csRef<iFile> file = manager->OpenFile (filename.GetData (), TARGETS_PATH);
  if (!file)
    return ReportError ("Could not open file %s", filename.GetData ());

  // Parse landmarks file
  char line[256];
  int mhIndex;
  size_t totalMHVerts = coords.GetSize ();

  while (!file->AtEOF ())
  {
    // Parse a line
    if (!manager->ParseLine (file, line, 255))
    {
      if (!file->AtEOF ())
        return ReportError ("Malformed Makehuman landmarks file");
    }
    else
    {
      csStringArray words;
      size_t numVals = words.SplitString (csString (line).Trim (), " ", csStringArray::delimIgnore);

      if (numVals != 1)
        return ReportError ("Malformed Makehuman landmarks file");

      // Parse the index of Makehuman vertex
      if (sscanf (words[0], "%i", &mhIndex) != 1 || mhIndex > (int) totalMHVerts)
        return ReportError ("Wrong element in Makehuman target file");

      landmarks.Push (mhIndex);
    }
  }

  return true;
}

/***** Utility functions for micro-expressions processing *****/

void printVector (csArray<float> v)
{
  printf ("SIZE  %i\n", (int)v.GetSize ());
  for (size_t i = 0; i < v.GetSize (); i++)
  {
    printf (" %.4f", v[i]);
  }
  printf ("\n");
  fflush (stdout);
}

void printMatrix (size_t n, size_t m, double x[])
{
  printf ("SIZE  %ix%i\n", (int)n, (int)m);
  for (size_t li = 0; li < n; li++)
  {
    printf ("line %i: ", (int)li);
    for (size_t ci = 0; ci < m; ci++)
      printf (" %.4f", x[li*m+ci]);
    printf ("\n");
  }
  fflush (stdout);
}

void printVectors (csArray<csVector3> m)
{
  printf ("SIZE  %ix%i\n", (int)m.GetSize (), 3);
  for (size_t li = 0; li < m.GetSize (); li++)
  {
    printf ("line %i: ", (int)li);
    for (size_t ci = 0; ci < 3; ci++)
      printf (" %.4f", m[li][ci]);
    printf ("\n");
  }
  fflush (stdout);
}

// x (n x 3) & y (m x 3)  ==>  return (n x m)
double* dot (size_t n, size_t m, 
             const csArray<csVector3>& x, 
             const csArray<csVector3>& y)
{
  double* res = new double[n*m];
    
  for (size_t i = 0; i < n; i++)
    for (size_t j = 0; j < m; j++)
    {
      res[i*m + j] = 0.0f;
      for (size_t k = 0; k < 3; k++)
        res[i*m + j] += (double) x[i][k] * (double) y[j][k];
    }

  return res;
}

// x (l x n) & y (n x m)  ==>  return (l x m)
double* dot (size_t l, size_t n, size_t m, double x[], double y[])
{
  double* res = new double[l*m];
    
  for (size_t i = 0; i < l; i++)
    for (size_t j = 0; j < m; j++)
    {
      res[i*m + j] = 0.0f;
      for (size_t k = 0; k < n; k++)
        res[i*m + j] += x[i*n + k] * y[k*m + j];
    }

  return res;
}

// x (n x n) ==>  return (n)
double* diagonal (size_t n, double m[])
{
  double* res = new double[n];
    
  for (size_t i = 0; i < n; i++)
    res[i] = m[i*n + i];
  
  return res;
}

// x (n) & y (m)  ==>  return (n x m)
double* addnewaxis (size_t n, double x[], size_t m, double y[])
{
  double* res = new double[n*m];
    
  for (size_t li = 0; li < n; li++)
    for (size_t ci = 0; ci < m; ci++)
      res[li*m + ci] = x[li] + y[ci];

  return res;
}

// x (n x m) & y (m)  ==>  return (n x m)
double* addvector (size_t n, size_t m, double x[], double y[])
{
  double* res = new double[n*m];

  for (size_t li = 0; li < n; li++)
    for (size_t ci = 0; ci < m; ci++)
      res[li*m + ci] = x[li*m + ci] + y[ci];

  return res;
}

// x (n) ==> return (1)
double max (size_t n, double x[])
{
  double res = x[0];

  for (size_t i = 0; i < n; i++)
    if (res < x[i])
      res = x[i];

  return res;
}

// x (n x m) ==> return s2 (m)
void mincol (size_t n, size_t m, double x[], double s2[])
{
  for (size_t ci = 0; ci < m; ci++)
  {
    s2[ci] = x[ci];
    for (size_t li = 0; li < n; li++)
      if (s2[ci] > x[li*m + ci])
        s2[ci] = x[li*m + ci];
  }
}

// x (n x 3) & y (nullptr)  ==>  return (n x n)
// x (n x 3) & y (m x 3)  ==>  return (n x m)
double* compute_distance2 (size_t n, size_t m, 
                           const csArray<csVector3>& x, 
                           const csArray<csVector3>& y)
{
  double *gram, *tmp;
  size_t size = y.IsEmpty () ? n*n : n*m;
  double* res = new double[size];

  if (y.IsEmpty ())
  {
    gram = dot (n, n, x, x);
    double* diag = diagonal (n, gram);
    tmp = addnewaxis (n, diag, n, diag);

    // res = tmp - 2 * gram
    for (size_t li = 0; li < n; li++)
      for (size_t ci = 0; ci < n; ci++)
        res[li*n + ci] = tmp[li*n + ci] - 2 * gram[li*n + ci];

    delete [] diag;
  }
  else
  {
    gram = dot (n, m, x, y);

    // diagx = (x*x).sum (-1)
    double* diagx = new double[n];
    for (size_t li = 0; li < n; li++)
    {
      diagx[li] = 0.0f;
      for (size_t ci = 0; ci < 3; ci++)
        diagx[li] += (double) x[li][ci] * (double) x[li][ci];
    }   

    // diagy = (y*y).sum (-1)
    double* diagy = new double[m];
    for (size_t li = 0; li < m; li++)
    {
      diagy[li] = 0.0f;
      for (size_t ci = 0; ci < 3; ci++)
        diagy[li] += (double) y[li][ci] * (double) y[li][ci];
    }   

    tmp = addnewaxis (n, diagx, m, diagy);

    // res = tmp - 2 * gram
    for (size_t li = 0; li < n; li++)
      for (size_t ci = 0; ci < m; ci++)
        res[li*m + ci] = tmp[li*m + ci] - 2 * gram[li*m + ci];

    delete [] diagx;
    delete [] diagy;
  }

  delete [] gram;
  delete [] tmp;

  return res;
}

// This method is based on Makehuman file 'makehuman/core/warp.py':
// RBFs (Radial Basis Function) are Hardy functions:   
//    h_i (x) = sqrt ( |x - x_i|^2 + s_i^2 )
// where s_i = min_(j != i) |x_j - x_i| is the minimal distance to another landmark.
//
// x (n x 3) & y (nullptr) ==> return (n x n)  with s2 (n)
// x (n x 3) & y (m x 3) & s2 (m) ==> return (n x m)
double* rbf (size_t n, size_t m, 
             const csArray<csVector3>& x, 
             const csArray<csVector3>& y, 
             double s2[])
{
  double* dists2 = compute_distance2 (n, m, x, y);

  if (y.IsEmpty ())
  {
    // dtmp = dists2 + dmax * numpy.identity (x.shape[0])
    double dmax = max (n, dists2);
    double* dtmp = new double[n*n];
    for (size_t li = 0; li < n; li++)
    {
      for (size_t ci = 0; ci < n; ci++)
        dtmp[li*n + ci] = dists2[li*n + ci];
      dtmp[li*n + li] += dmax;
    }

    mincol (n, n, dtmp, s2);   // for each of the columns, find the minimum
    m = n;

    delete [] dtmp;
  }

  // numpy.sqrt (dists2 + self.s2)
  double* res = addvector (n, m, dists2, s2);

  for (size_t li = 0; li < n; li++)
    for (size_t ci = 0; ci < m; ci++)
      res[li*m + ci] = sqrt (res[li*m + ci]);

  delete [] dists2;

  return res;
}

// H (n x m) & yverts (n x 3) ==> return (m x 3)
double* leastSquare (size_t n, size_t m, 
                     double H[], 
                     const csArray<csVector3>& yverts)
{
  // w = numpy.linalg.lstsq (H, yverts)[0]
  double* w = new double[m*3];
  double* b = new double[n];

  // transpose H
  double* transH = new double[n*m];
  for (size_t li=0; li<n; li++)
    for (size_t ci=0; ci<m; ci++)
      transH[ci*n+li] = H[li*m+ci];

  for (size_t i=0; i<3; i++)
  {
    // init vector b with component i of yverts
    for (size_t li=0; li<n; li++)
      b[li] = (double) yverts[li][i];

    // resolve linear equations system (functions from QR_SOLVE library)
    double* wi = qr_solve (n, m, transH, b);
    // [equivalent instruction 1]  double* wi = svd_solve (n, m, transH, b);
    // [equivalent instruction 2]  int done; double* wi = normal_solve (n, m, transH, b, &done);

    // copy component i of least squares solution
    for (size_t li=0; li<m; li++)
      w[li*3 + i] = wi[li];

    delete [] wi;
  }

  delete [] b;
  delete [] transH;

  return w;
}

bool MakehumanCharacter::WarpMicroExpression (const csArray<csVector3>& xverts,
					      double s2[], double w[], Target& expr)
{
  // Get indices and displacements of non null offsets
  csArray<size_t> idx;
  csArray<csVector3> xmorph;
  for (size_t i = 0; i < expr.offsets.GetSize (); i++)
  {
    if (!expr.offsets[i].IsZero ())
    {
      idx.Push (i);
      // Apply basic model properties (gender/ethnic/age) and expression offsets
      // to source vertices
      xmorph.Push (basicMesh[expr.indices[i]] + basicMorph[expr.indices[i]] + expr.offsets[i]);
    }
  }

  // Compute rbf matrix of micro-expression
  // H = rbf (xmorph, xverts)
  size_t n = xverts.GetSize ();
  double* H = rbf (xmorph.GetSize (), n, xmorph, xverts, s2);

  // Compute displacements of micro-expression
  double* ymorph = dot (xmorph.GetSize (), n, 3, H, w);

  // Save micro-expression offsets
  for (size_t i = 0; i < xmorph.GetSize (); i++)
  {
    size_t index = idx[i];
    expr.offsets[index] = csVector3 (ymorph[i * 3] - coords[expr.indices[index]][0], 
				     ymorph[i * 3 + 1] - coords[expr.indices[index]][1],
				     ymorph[i * 3 + 2] - coords[expr.indices[index]][2]);
  }  

  delete [] H;
  delete [] ymorph;

  return true;
}

bool MakehumanCharacter::ParseMicroExpressions (const ModelTargets& modelVals,
						csArray<Target>& microExpr)
{
  // Generate name and weight of expression folders
  printf ("Generating micro-expressions:\n");
  csString name;
  float weight;
  csArray<csString> subdirNames;
  csArray<float> subdirWeights;

  for (size_t i = 0; i < modelVals.ethnics.GetSize (); i++)
    for (size_t j = 0; j < modelVals.gender.GetSize (); j++)
      for (size_t k = 0; k < modelVals.age.GetSize (); k++)
      {
        name = modelVals.ethnics[i].name;
        if (name == "neutral")
          name.Replace ("caucasian");
        name.Append ("/").Append (modelVals.gender[j].name);
        name.Append (modelVals.age[k].name);
        name.ReplaceAll ("-", "_");
        weight = modelVals.ethnics[i].weight * modelVals.gender[j].weight * modelVals.age[k].weight;
        subdirNames.Push (name);
        subdirWeights.Push (weight);
        printf ("  - from model '%s' with weight %.3f\n", name.GetData (), weight);
      }
  
  if (subdirNames.GetSize () == 0)
    return ReportError ("Didn't find any target file to generate model expressions");

  // Find expressions in Makehuman directories
  csString dir = csString (EXPRESSIONS_PATH).Append ("units/");
  printf ("\nParsing model micro-expressions (from subfolders of '%s'):", dir.GetData ());
  csString targetname, filename, exprname;
  size_t index;
  csRef<iVFS> VFS = csQueryRegistry<iVFS> (manager->objectRegistry);
  // use any EXPRESSIONS_PATH subdirectories because they all contain the same expressions
  dir.Append (subdirNames[0]).Append ("/");

  if (!VFS->Exists (dir.GetData ()))
    return ReportError ("Didn't find Makehuman expressions directory '%s' (deprecated)", dir.GetData ());

  csRef<iStringArray> expressionFiles = VFS->FindFiles (dir);
  expressionFiles->Sort ();

  // Init list of micro-expression offsets
  for (size_t i = 0; i < expressionFiles->GetSize (); i++)
  {
    // Get expression name from filename
    csString targetfile = csString (expressionFiles->Get (i));
    index = targetfile.FindLast ('/');
    targetfile.SubString (filename, index + 1);
    index = filename.FindLast ('.');
    filename.SubString (exprname, 0, index);
    // Print expression name
    if (i%4 == 0) printf ("\n");
    printf ("%3i. ", (int) i+1); printf ("%-28s", exprname.GetData ());
    // Create a new expression
    Target newExpr (exprname.GetData (), nullptr, 0.0f);
    microExpr.Push (newExpr);
  }
  printf ("\n");

  // Parse micro-expression files and generate an offset buffer 
  // for each micro-expression
  for (size_t i = 0; i < microExpr.GetSize (); i++)
  {
    csHash<size_t, size_t> indexMapping;

    for (size_t j = 0; j < subdirNames.GetSize (); j++)
    {
      // Generate path of Makehuman target file
      filename = csString (EXPRESSIONS_PATH).Append ("units/");
      filename.Append (subdirNames[j]).Append ("/");
      filename.Append (microExpr[i].name).Append (".target");

      // Parse target file into a temporary offset buffer
      csArray<csVector3> offsets;
      csArray<size_t> indices;
      if (!ParseMakehumanTargetFile (filename.GetData (), offsets, indices))
        return ReportError ("Could not parse the Makehuman target file %s", filename.GetData ());

      // Cumulate displacement in the offset buffer of the expression
      float weight = subdirWeights[j];
      for (size_t k = 0; k < offsets.GetSize (); k++)
      {
	// Try to find the vertex in the current list
	size_t* index = indexMapping.GetElementPointer (indices[k]);
	if (index)
	  microExpr[i].offsets[*index] += weight * offsets[k];

	else
	{
	  microExpr[i].indices.Push (indices[k]);
	  microExpr[i].offsets.Push (weight * offsets[k]);
	  indexMapping.Put (indices[k], microExpr[i].indices.GetSize () - 1);
	}
      }
    }
  }

  return true;
}

bool MakehumanCharacter::GenerateMicroExpressions (const ModelTargets& modelVals,
						   csArray<Target>& microExpr)
{
  // Code based on files 'makehuman/apps/warpmodifier.py' and 'makehuman/core/warp.py'
  // from Makehuman 1.0 alpha 7.

  // Init warping data

  // Parse 'face' landmarks
  const char* bodypart = "face";
  csArray<size_t> landmarks;
  if (!ParseLandmarks (bodypart, landmarks))
    return ReportError ("Error while parsing landmarks file of body part '%s'", bodypart);

  // Parse micro-expressions offsets
  if (!ParseMicroExpressions (modelVals, microExpr))
    return ReportError ("Could not parse the Makehuman micro-expressions");

  // Get source and target vertices, using indices of facial landmarks
  csArray<csVector3> xverts, yverts;
  size_t ldk = landmarks.GetSize ();
  for (size_t i = 0; i < ldk; i++)
  {
    size_t mhIndex = landmarks[i];
    // Apply basic model properties (gender/ethnic/age) to source vertices
    // corresponding to facial landmarks
    xverts.Push (basicMesh[mhIndex] + basicMorph[mhIndex]);
    // Get target vertices
    yverts.Push (coords[mhIndex]);
  }

  // H = rbf (xverts)
  double* s2 = new double[ldk];
  csArray<csVector3> tmp;  // unused data
  double* H = rbf (ldk, 0, xverts, tmp, s2);

  // w = numpy.linalg.lstsq (H, self.yverts)[0]
  double* w = leastSquare (ldk, ldk, H, yverts);

  // Adapt micro-expressions to human model
  for (size_t i = 0; i < microExpr.GetSize (); i++)
    if (!WarpMicroExpression (xverts, s2, w, microExpr[i]))
      return ReportError ("Could not warp mirco expression %s", microExpr[i].name.GetData ());

  delete [] s2;
  delete [] H;
  delete [] w;

  return true;
}

bool MakehumanCharacter::AddExpressionsToModel (CS::Mesh::iAnimatedMeshFactory* amfact,
						const csArray<VertBuf>& mapBuf,
						const char* prefix,
						const csArray<Target>& mhExpressions)
{
  if (!amfact)
    return ReportError ("Error while adding new expression targets to model: no animesh factory");

  size_t csIndex;
  size_t totalCSVerts = amfact->GetVertexCount ();
  size_t totalMHVerts = mapBuf.GetSize ();
  if (totalCSVerts < totalMHVerts)
    return ReportError ("The animesh should have at least as much vertices as the mapping buffer");

  // Clear target subsets of animesh factory
  amfact->ClearSubsets ();

  // For each expression, convert Makehuman offsets to Crystal Space offsets,
  // create a new morph target and add it to the animesh factory
  for (size_t ei = 0; ei < mhExpressions.GetSize (); ei++)
  {
    // Initialize and lock the CS offset buffer
    csRef<iRenderBuffer> csExpression;
    csExpression = csRenderBuffer::CreateRenderBuffer 
      (totalCSVerts, CS_BUF_STATIC, CS_BUFCOMP_FLOAT, 3);
    csRenderBufferLock<csVector3> csOffsets (csExpression);
    for (size_t i = 0; i < totalCSVerts; i++)
      csOffsets[i] = csVector3 (0.0f);

    // Fill CS offset buffer with Makehuman offsets
    for (size_t mhIndex = 0; mhIndex < mhExpressions[ei].offsets.GetSize (); mhIndex++)
    {
      csVector3 offset = mhExpressions[ei].offsets[mhIndex];
      offset[2] *= -1.0f;   // inverse Z component sign

      // Copy the parsed offset to all cs vertices associated with this Makehuman vertex
      for (size_t i = 0; i < mapBuf[mhExpressions[ei].indices[mhIndex]].vertices.GetSize (); i++)
      {
        csIndex = mapBuf[mhExpressions[ei].indices[mhIndex]].vertices[i];
        csOffsets[csIndex] = offset;
      }
    }

    // Create a new expression target of animesh factory
    csExpression->Release ();
    CS::Mesh::iAnimatedMeshMorphTarget* morphTarget = 
      amfact->CreateMorphTarget ((csString (prefix) + mhExpressions[ei].name).GetData ());
    morphTarget->SetVertexOffsets (csExpression);
    morphTarget->Invalidate ();
  }

  // Optimize added morph targets
  amfact->Invalidate ();

  return true;
}

}
CS_PLUGIN_NAMESPACE_END (Makehuman)
