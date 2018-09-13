/*=========================================================================
Program:   Visualization Toolkit
Module:    vtkEllipseRepresentation.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkEllipseRepresentation.h"
#include "vtkRenderer.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>

vtkStandardNewMacro(vtkEllipseRepresentation);


//-------------------------------------------------------------------------
vtkEllipseRepresentation::vtkEllipseRepresentation()
{
	this->InteractionState = vtkEllipseRepresentation::Outside;

	this->ShowEllipse = ELLIPSE_ON;
	this->ProportionalResize = 0;
	this->Tolerance = 10;
	this->Resolution = 50;
	this->SelectionPoint[0] = this->SelectionPoint[1] = 0.0;

	// Initial positioning information
	this->Negotiated = 0;
	this->PositionCoordinate = vtkCoordinate::New();
	this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
	this->PositionCoordinate->SetValue(0.05, 0.05);
	this->Position2Coordinate = vtkCoordinate::New();
	this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
	this->Position2Coordinate->SetValue(0.1, 0.1); //may be updated by the subclass
	this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);

	// Create the geometry in canonical coordinates
	this->EWPoints = vtkPoints::New();
	this->EWPoints->SetDataTypeToDouble();
	this->EWPoints->SetNumberOfPoints(50);
	for (unsigned int i = 0; i < this->Resolution; i++)
	{
		const double angle = 2.0 * vtkMath::Pi() * static_cast<double>(i) /
			static_cast<double>(this->Resolution);
		this->EWPoints->SetPoint(i, 0.5*cos(angle) + 0.5, 0.5*sin(angle) + 0.5, 0);
	}

	vtkCellArray *outline = vtkCellArray::New();
	outline->InsertNextCell(this->Resolution + 1);
	for (int i = 0; i < this->EWPoints->GetNumberOfPoints(); i++)
	{
		outline->InsertCellPoint(i);
	}
	outline->InsertCellPoint(0);

	this->EWPolyData = vtkPolyData::New();
	this->EWPolyData->SetPoints(this->EWPoints);
	this->EWPolyData->SetLines(outline);
	outline->Delete();

	this->EWTransform = vtkTransform::New();
	this->EWTransformFilter = vtkTransformPolyDataFilter::New();
	this->EWTransformFilter->SetTransform(this->EWTransform);
	this->EWTransformFilter->SetInputData(this->EWPolyData);

	this->EWMapper = vtkPolyDataMapper2D::New();
	this->EWMapper->SetInputConnection(
		this->EWTransformFilter->GetOutputPort());
	this->EWActor = vtkActor2D::New();
	this->EWActor->SetMapper(this->EWMapper);

	this->EllipseProperty = vtkProperty2D::New();
	this->EWActor->SetProperty(this->EllipseProperty);

	this->MinimumSize[0] = 1;
	this->MinimumSize[1] = 1;
	this->MaximumSize[0] = 100000;
	this->MaximumSize[1] = 100000;

	this->Moving = 0;
}

//-------------------------------------------------------------------------
vtkEllipseRepresentation::~vtkEllipseRepresentation()
{
	this->PositionCoordinate->Delete();
	this->Position2Coordinate->Delete();

	this->EWPoints->Delete();
	this->EWTransform->Delete();
	this->EWTransformFilter->Delete();
	this->EWPolyData->Delete();
	this->EWMapper->Delete();
	this->EWActor->Delete();
	this->EllipseProperty->Delete();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkEllipseRepresentation::GetMTime()
{
	vtkMTimeType mTime = this->Superclass::GetMTime();
	mTime = std::max(mTime, this->PositionCoordinate->GetMTime());
	mTime = std::max(mTime, this->Position2Coordinate->GetMTime());
	mTime = std::max(mTime, this->EllipseProperty->GetMTime());
	return mTime;
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::StartWidgetInteraction(double eventPos[2])
{
	this->StartEventPosition[0] = eventPos[0];
	this->StartEventPosition[1] = eventPos[1];
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::WidgetInteraction(double eventPos[2])
{
	double XF = eventPos[0];
	double YF = eventPos[1];

	// convert to normalized viewport coordinates
	this->Renderer->DisplayToNormalizedDisplay(XF, YF);
	this->Renderer->NormalizedDisplayToViewport(XF, YF);
	this->Renderer->ViewportToNormalizedViewport(XF, YF);

	// there are four parameters that can be adjusted
	double *fpos1 = this->PositionCoordinate->GetValue();
	double *fpos2 = this->Position2Coordinate->GetValue();
	double par1[2];
	double par2[2];
	par1[0] = fpos1[0];
	par1[1] = fpos1[1];
	par2[0] = fpos1[0] + fpos2[0];
	par2[1] = fpos1[1] + fpos2[1];

	double delX = XF - this->StartEventPosition[0];
	double delY = YF - this->StartEventPosition[1];
	double delX2 = 0.0, delY2 = 0.0;

	// Based on the state, adjust the representation. Note that we force a
	// uniform scaling of the widget when tugging on the control points (and
	// when proportional resize is on). This is done by finding the maximum
	// movement in the x-y directions and using this to scale the widget.
	if (this->ProportionalResize && !this->Moving)
	{
		double sx = fpos2[0] / fpos2[1];
		double sy = fpos2[1] / fpos2[0];
		if (fabs(delX) > fabs(delY))
		{
			delY = sy*delX;
			delX2 = delX;
			delY2 = -delY;
		}
		else
		{
			delX = sx*delY;
			delY2 = delY;
			delX2 = -delX;
		}
	}
	else
	{
		delX2 = delX;
		delY2 = delY;
	}

	// The previous "if" statement has taken care of the proportional resize
	// for the most part. However, tugging on edges has special behavior, which
	// is to scale the box about its center.
	switch (this->InteractionState)
	{
	case vtkEllipseRepresentation::AdjustingP0:
		par1[1] = par1[1] + delY;
		break;
	case vtkEllipseRepresentation::AdjustingP1:
		par2[0] = par2[0] + delX;
		break;
	case vtkEllipseRepresentation::AdjustingP2:
		par2[1] = par2[1] + delY;
		break;
	case vtkEllipseRepresentation::AdjustingP3:
		par1[0] = par1[0] + delX;
		break;
	case vtkEllipseRepresentation::Inside:
		if (this->Moving)
		{
			par1[0] = par1[0] + delX;
			par1[1] = par1[1] + delY;
			par2[0] = par2[0] + delX;
			par2[1] = par2[1] + delY;
		}
		break;
	}

	// Modify the representation
	if (par2[0] > par1[0] && par2[1] > par1[1])
	{
		this->PositionCoordinate->SetValue(par1[0], par1[1]);
		this->Position2Coordinate->SetValue(par2[0] - par1[0], par2[1] - par1[1]);
		this->StartEventPosition[0] = XF;
		this->StartEventPosition[1] = YF;
	}

	this->Modified();
	this->BuildRepresentation();
}


//-------------------------------------------------------------------------
void vtkEllipseRepresentation::NegotiateLayout()
{
	double size[2];
	this->GetSize(size);

	// Update the initial ellipse geometry
	for (unsigned int i = 0; i < this->Resolution; i++)
	{
		const double angle = 2.0 * vtkMath::Pi() * static_cast<double>(i) /
			static_cast<double>(this->Resolution);
		this->EWPoints->SetPoint(i, 0.5*cos(angle) + 0.5, 0.5*sin(angle) + 0.5, 0);
	}
}


//-------------------------------------------------------------------------
int vtkEllipseRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
	int *pos1 = this->PositionCoordinate->
		GetComputedDisplayValue(this->Renderer);
	int *pos2 = this->Position2Coordinate->
		GetComputedDisplayValue(this->Renderer);

	// check for poximinity to control points
	// Figure out where we are in the widget. Exclude inside and outside case first.
	double center[2];
	center[0] = (pos1[0] + pos2[0]) / 2.0;
	center[1] = (pos1[1] + pos2[1]) / 2.0;
	double a = (pos2[0]-pos1[0]) / 2.0;
	double b = (pos2[1] - pos1[1]) / 2.0;

	auto ellipse = [](int x, int y, double *center, double a, double b, double tolerence)->_InteractionState {
		// ellipse equation: [(x-x0)/a]^2 + [(y-y0)/b]^2 = 1, where a and b are associated to major and minor axis length

		// create two ellipses, one larger and one smaller than the drawn ellipse, any region between the two ellipses is considered as edge
		double distBig = pow((x - center[0]) / (a+tolerence), 2) + pow((y - center[1]) / (b+tolerence), 2);
		double distSmall = pow((x - center[0]) / (a-tolerence), 2) + pow((y - center[1]) / (b-tolerence), 2);
		if (distBig > 1)
			return Outside;
		else if (distSmall < 1)
			return Inside;
		else
			return Edge;
	};

	this->InteractionState = ellipse(X, Y, center, a, b, this->Tolerance);

	if (this->InteractionState == Edge)// we are on the boundary of the ellipse
	{
		double p0[2] = { (pos1[0] + pos2[0])*0.5 , pos1[1] }; // bottom;
		double p1[2] = { pos2[0], (pos1[1] + pos2[1])*0.5 }; // right
		double p2[2] = { (pos1[0] + pos2[0])*0.5, pos2[1] }; //top
		double p3[2] = { pos1[0] ,(pos1[1] + pos2[1])*0.5 }; // left

		if (sqrt(pow(X - p0[0], 2) + pow(Y - p0[1], 2)) < this->Tolerance)
		{
			this->InteractionState = vtkEllipseRepresentation::AdjustingP0;
		}
		else if (sqrt(pow(X - p1[0], 2) + pow(Y - p1[1], 2)) < this->Tolerance)
		{
			this->InteractionState = vtkEllipseRepresentation::AdjustingP1;
		}
		else if (sqrt(pow(X - p2[0], 2) + pow(Y - p2[1], 2)) < this->Tolerance)
		{
			this->InteractionState = vtkEllipseRepresentation::AdjustingP2;
		}
		else if (sqrt(pow(X - p3[0], 2) + pow(Y - p3[1], 2)) < this->Tolerance)
		{
			this->InteractionState = vtkEllipseRepresentation::AdjustingP3;
		}
		else
		{
			this->InteractionState = vtkEllipseRepresentation::Edge;
		}
	}

	return this->InteractionState;
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::BuildRepresentation()
{
	if (this->Renderer &&
		(this->GetMTime() > this->BuildTime ||
		(this->Renderer->GetVTKWindow() &&
			this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime)))
	{
		// Negotiate with subclasses
		if (!this->Negotiated)
		{
			this->NegotiateLayout();
			this->Negotiated = 1;
		}

		// Set things up
		int *pos1 = this->PositionCoordinate->
			GetComputedViewportValue(this->Renderer);
		int *pos2 = this->Position2Coordinate->
			GetComputedViewportValue(this->Renderer);

		// If the widget's aspect ratio is to be preserved (ProportionalResizeOn),
		// then (pos1,pos2) are a bounding rectangle.
		if (this->ProportionalResize)
		{
		}

		// Now transform the canonical widget into display coordinates
		double size[2];
		this->GetSize(size);
		double tx = pos1[0];
		double ty = pos1[1];
		double sx = (pos2[0] - pos1[0]) / size[0];
		double sy = (pos2[1] - pos1[1]) / size[1];

		this->EWTransform->Identity();
		this->EWTransform->Translate(tx, ty, 0.0);
		this->EWTransform->Scale(sx, sy, 1);

		this->BuildTime.Modified();
	}
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::GetActors2D(vtkPropCollection *pc)
{
	pc->AddItem(this->EWActor);
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
	this->EWActor->ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkEllipseRepresentation::RenderOverlay(vtkViewport *w)
{
	this->BuildRepresentation();
	if (!this->EWActor->GetVisibility())
	{
		return 0;
	}
	return this->EWActor->RenderOverlay(w);
}

//-------------------------------------------------------------------------
int vtkEllipseRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
	this->BuildRepresentation();
	if (!this->EWActor->GetVisibility())
	{
		return 0;
	}
	return this->EWActor->RenderOpaqueGeometry(w);
}

//-----------------------------------------------------------------------------
int vtkEllipseRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *w)
{
	this->BuildRepresentation();
	if (!this->EWActor->GetVisibility())
	{
		return 0;
	}
	return this->EWActor->RenderTranslucentPolygonalGeometry(w);
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkEllipseRepresentation::HasTranslucentPolygonalGeometry()
{
	this->BuildRepresentation();
	if (!this->EWActor->GetVisibility())
	{
		return 0;
	}
	return this->EWActor->HasTranslucentPolygonalGeometry();
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);

	os << indent << "Show Ellipse: ";
	if (this->ShowEllipse == ELLIPSE_OFF)
	{
		os << "Off\n";
	}
	else if (this->ShowEllipse == ELLIPSE_ON)
	{
		os << "On\n";
	}
	else //if ( this->ShowVerticalEllipse == Ellipse_ACTIVE)
	{
		os << "Active\n";
	}

	if (this->EllipseProperty)
	{
		os << indent << "Ellipse Property:\n";
		this->EllipseProperty->PrintSelf(os, indent.GetNextIndent());
	}
	else
	{
		os << indent << "Ellipse Property: (none)\n";
	}

	os << indent << "Proportional Resize: "
		<< (this->ProportionalResize ? "On\n" : "Off\n");
	os << indent << "Minimum Size: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
	os << indent << "Maximum Size: " << this->MaximumSize[0] << " " << this->MaximumSize[1] << endl;

	os << indent << "Moving: " << (this->Moving ? "On\n" : "Off\n");
	os << indent << "Tolerance: " << this->Tolerance << "\n";

	os << indent << "Selection Point: (" << this->SelectionPoint[0] << ","
		<< this->SelectionPoint[1] << "}\n";
}