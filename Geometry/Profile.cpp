// Geometry.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "json\json.h"

#include "profile.h"
#include "vertices.h"
#include "edges.h"
#include "Xform2D.h"

#include <windows.h>


using namespace std;


double Profile::fAccuracy__ = 0.001;


Profile::Profile(string strFile)
    {
    ifstream ssIn;
    ssIn.open(strFile);

    Json::Value root;
    ssIn >> root;

    // Read the vertices first so that EdgeList can reference them
    mapvert_.Read(root[VertexMap::JsonTypeName()]);

    lledges_ = EdgeList(root[EdgeList::JsonTypeName()], mapvert_);
    }

int Profile::GetEvalCount() const // just for diagnostics
    {
    int result = 0;
    for (shared_ptr<Edge> sp_edgeIter : getEdges())
        result += sp_edgeIter->GetEvalCount();
    return result;
    }

double Profile::ComputeMatlArea() const
    {
    /*
    To compute an area based on efficient orientation we try an orientation for
    every one degree in a 90 degree sweep.  90 degrees is sufficient because
    a rectangle is symmetric in 2 directions, thus 4 quandrants.

    There's plenty of ways to get better efficiency
    - Do 2 passes, a rough pass at 5 degree increments, then a finer pass when close
    - Convert everything to 32 bit integers
    - Make guesses from the prevailing direction of the LineSegments
    .. BUT NONE OF THE ABOVE IS NECESSARY.
    A part will have more than 2500 arcs before it takes more than a second to process.
    */

    double result = DBL_MAX;
    for (double fAngle = 0; fAngle < M_PI / 2; fAngle += M_PI / 180) // 90 degr sweep in 1 degr incr
        {
        double xMin = DBL_MAX;
        double xMax = DBL_MIN;
        double yMin = DBL_MAX;
        double yMax = DBL_MIN;
        Xform2D xf;

        xf.Rotate(fAngle); // Performance note: minor opportunity to improve here
        for (shared_ptr<Edge> sp_edgeIter : getEdges())
            sp_edgeIter->ProcLimits(xMin, xMax, yMin, yMax, xf);
        double area = (getPadding() + (xMax - xMin)) * (getPadding() + (yMax - yMin));

        // ..for diagnostics
        //ostringstream ssOut;
        //ssOut << fAngle * 180 / M_PI << ' ' << xMin << ' ' << xMax << ' ' << xMax - xMin << ' ' 
        //    << yMin << ' ' << yMax << ' ' << yMax - yMin << ' ' << area << endl;
        //OutputDebugStringA(ssOut.str().c_str());

        result = min(result, area);
        }
    return result;
    }

double Profile::ComputeCost() const
    {
    double result = ComputeMatlArea() * getMatlCost(); 
            
    for (shared_ptr<Edge> sp_edgeIter : getEdges())
            result += sp_edgeIter->TimeCut() * getMachineRate();

    return result;
    }

