/*=========================================================================
Program:   Visualization Toolkit
Module:    vtkEllipseRepresentation.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Ko Ka Long
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
#include "vtkLine.h"

#include "vtkXMLPolyDataWriter.h"

vtkStandardNewMacro(vtkEllipseRepresentation);

//-------------------------------------------------------------------------
vtkEllipseRepresentation::vtkEllipseRepresentation()
{
	this->InteractionState = vtkEllipseRepresentation::Outside;

	this->Resolution = 50;
	this->ShowEllipse = ELLIPSE_ON;
	this->ProportionalResize = 0;
	this->Tolerance = 3;
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
	this->EWPoints->SetNumberOfPoints(this->Resolution);
	vtkSmartPointer<vtkCellArray> outline = vtkSmartPointer<vtkCellArray>::New();
	vtkIdType* lineIndices = new vtkIdType[this->Resolution + 1];

	for (unsigned int i = 0; i < this->Resolution; i++)
	{
		const double angle = 2.0 * vtkMath::Pi() * static_cast<double>(i) /
			static_cast<double>(this->Resolution);
		EWPoints->InsertPoint(static_cast<vtkIdType>(i), 100*cos(angle)+100, 100*sin(angle)+100, 0.);
		lineIndices[i] = static_cast<vtkIdType>(i);
	}

	this->EWPolyData = vtkPolyData::New();
	this->EWPolyData->SetPoints(this->EWPoints);

	for (unsigned int i = 0; i < this->EWPoints->GetNumberOfPoints(); i++)
	{
		vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
		line->GetPointIds()->SetId(0, i);
		if (i != this->EWPoints->GetNumberOfPoints() - 1)
			line->GetPointIds()->SetId(1, i + 1);
		else
			line->GetPointIds()->SetId(1, 0);

		outline->InsertNextCell(line);
	}

	//delete[] lineIndices;
	
	this->EWPolyData->SetLines(outline);
	cerr << *this->EWPolyData;

	vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
	writer->SetInputData(this->EWPolyData);
	writer->SetFileName("./cricle.vtp");
	writer->Write();

	//// shift center to (0.5,0.5,0)
	//vtkSmartPointer<vtkTransform> initTransform = vtkSmartPointer<vtkTransform>::New();
	//initTransform->Translate(0.5, 0.5, 0);
	//vtkSmartPointer<vtkTransformPolyDataFilter> initTransformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	//initTransformFilter->SetTransform(initTransform);
	//initTransformFilter->SetInputData(this->EWPolyData);
	//this->EWPolyData->DeepCopy(initTransformFilter->GetOutput());

	this->EWTransform = vtkTransform::New();
	this->EWTransformFilter = vtkTransformPolyDataFilter::New();
	this->EWTransformFilter->SetTransform(this->EWTransform);
	this->EWTransformFilter->SetInputData(this->EWPolyData);

	this->EWMapper = vtkPolyDataMapper2D::New();
	//this->EWMapper->SetInputData(this->EWTransformFilter->GetOutput());
	this->EWMapper->SetInputData(this->EWPolyData);
	this->EWActor = vtkActor2D::New();
	this->EWActor->SetMapper(this->EWMapper);

	this->EllipseProperty = vtkProperty2D::New();
	this->EWActor->SetProperty(this->EllipseProperty);

	this->EllipseProperty->SetColor(1, 0, 0);
	this->EllipseProperty->SetLineWidth(10);
	this->EllipseProperty->SetOpacity(1);

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

	// scaling will be handled by point representations

	//// Based on the state, adjust the representation. Note that we force a
	//// uniform scaling of the widget when tugging on the corner points (and
	//// when proportional resize is on). This is done by finding the maximum
	//// movement in the x-y directions and using this to scale the widget.
	//if (this->ProportionalResize && !this->Moving)
	//{
	//	double sx = fpos2[0] / fpos2[1];
	//	double sy = fpos2[1] / fpos2[0];
	//	if (fabs(delX) > fabs(delY))
	//	{
	//		delY = sy*delX;
	//		delX2 = delX;
	//		delY2 = -delY;
	//	}
	//	else
	//	{
	//		delX = sx*delY;
	//		delY2 = delY;
	//		delX2 = -delX;
	//	}
	//}
	//else
	//{
	//	delX2 = delX;
	//	delY2 = delY;
	//}

	// The previous "if" statement has taken care of the proportional resize
	// for the most part. However, tugging on edges has special behavior, which
	// is to translate the ellipse.
	switch (this->InteractionState)
	{
	//case vtkEllipseRepresentation::AdjustingP0:
	//	par1[0] = par1[0] + delX;
	//	par1[1] = par1[1] + delY;
	//	break;
	//case vtkEllipseRepresentation::AdjustingP1:
	//	par2[0] = par2[0] + delX2;
	//	par1[1] = par1[1] + delY2;
	//	break;
	//case vtkEllipseRepresentation::AdjustingP2:
	//	par2[0] = par2[0] + delX;
	//	par2[1] = par2[1] + delY;
	//	break;
	//case vtkEllipseRepresentation::AdjustingP3:
	//	par1[0] = par1[0] + delX2;
	//	par2[1] = par2[1] + delY2;
	//	break;
	case vtkEllipseRepresentation::Edge:
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

	// Update the initial Ellipse geoemtry
	for (unsigned int i = 0; i < this->Resolution; i++)
	{
		const double angle = 2.0 * vtkMath::Pi() * static_cast<double>(i) / static_cast<double>(this->Resolution);
		this->EWPoints->SetPoint(i, cos(angle)*size[0] / 2. + 0.5, sin(angle)*size[1] / 2. +0.5, 0.);
	}
}

//-------------------------------------------------------------------------
int vtkEllipseRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
	int *pos1 = this->PositionCoordinate->
		GetComputedDisplayValue(this->Renderer);
	int *pos2 = this->Position2Coordinate->
		GetComputedDisplayValue(this->Renderer);

	// Figure out where we are in the widget.
	double center[2];
	center[0] = pos1[0] + pos2[0] / 2.0;
	center[1] = pos1[1] + pos2[1] / 2.0;

	double a = pos2[0] / 2.0;
	double b = pos2[1] / 2.0;

	auto ellipse = [](int x, int y, double *center, double a, double b, double tolerence)->_InteractionState {
		double dist = pow((x - center[0]) / a, 2) + pow((y - center[1]) / b, 2);
		if (dist < 1 - tolerence)
			return Inside;
		else if (dist > 1 + tolerence)
			return Outside;
		else
			return Edge;

	};

	_InteractionState state = ellipse(X, Y, center, a, b, this->Tolerance);
	this->InteractionState = state;

	switch (this->ShowEllipse)
	{
	case (ELLIPSE_ON):
		{
			this->EWActor->VisibilityOn();
			break;
		}
	case (ELLIPSE_ACTIVE):
	{
		this->EWActor->VisibilityOn();
		switch (state)
		{
		case (Edge):
		{
			this->EllipseProperty->SetColor(1, 1, 0);
			break;
		}
		default:
		{
			this->EllipseProperty->SetColor(1, 0, 0);
			break;
		}
		}
	}
	}

	return this->InteractionState;
}

//-------------------------------------------------------------------------
void vtkEllipseRepresentation::BuildRepresentation()
{
	if (this->GetMTime() > this->BuildTime ||
		(this->Renderer && this->Renderer->GetVTKWindow() &&
			this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
	{
		// Negotiate with subclasses
		if (!this->Negotiated)
		{
			this->NegotiateLayout();
			this->Negotiated = 1;
		}

		// Set things up
		int *pos1 = this->PositionCoordinate->
			GetComputedDisplayValue(this->Renderer);
		int *pos2 = this->Position2Coordinate->
			GetComputedDisplayValue(this->Renderer);

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
	else //if ( this->ShowEllipse == Ellipse_ACTIVE) 
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