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
// .NAME vtkEllipseRepresentation - represent a vtkEllipseWidget
// .SECTION Description
// This class is used to represent and render a vtEllipseWidget. To
// use this class, you need to specify the two corners of a elliptical
// region.
//
// The class is typically subclassed so that specialized representations can
// be created.  The class defines an API and a default implementation that
// the vtkEllipseRepresentation interacts with to render itself in the scene.

// .SECTION Caveats
// The separation of the widget event handling (e.g., vtkEllipseWidget) from
// the representation (vtkEllipseRepresentation) enables users and developers
// to create new appearances for the widget. It also facilitates parallel
// processing, where the client application handles events, and remote
// representations of the widget are slaves to the client (and do not handle
// events).

// .SECTION See Also
// vtkEllipseWidget

#ifndef __vtkEllipseRepresentation_h
#define __vtkEllipseRepresentation_h

#include "vtkWidgetRepresentation.h"
#include "vtkCoordinate.h"// Because of the viewport coordinate macro

class vtkProperty2D;
class vtkPoints;
class vtkPolyData;
class vtkActor2D;
class vtkTransform;
class vtkTransformPolyDataFilter;
class vtkPolyDataMapper2D;

class vtkEllipseRepresentation : public vtkWidgetRepresentation
{
public:
	// Description:
	// Instantiate this class.
	static vtkEllipseRepresentation *New();

	// Description:
	// Define standard methods.
	vtkTypeMacro(vtkEllipseRepresentation, vtkWidgetRepresentation);
	void PrintSelf(ostream& os, vtkIndent indent);

	// Description:
	// Specify opposite corners of the ellipse defining the boundary of the
	// widget. By default, these coordinates are in the normalized viewport
	// coordinate system, with Position the lower left of the outline, and
	// Position2 relative to Position. Note that using these methods are
	// affected by the ProportionalResize flag. That is, if the aspect ratio of
	// the representation is to be preserved (e.g., ProportionalResize is on),
	// then the ellipse (Position,Position2) is a bounding ellipse.
	vtkViewportCoordinateMacro(Position);
	vtkViewportCoordinateMacro(Position2);

	//BTX
	enum { ELLIPSE_OFF = 0, ELLIPSE_ON, ELLIPSE_ACTIVE };

	//ETX
	// Description:
	// Specify when and if the ellipse should appear. If ShowEllipse is "on",
	// then the ellipse will always appear. If ShowBorder is "off" then the
	// ellipse will never appear.  If ShowEllipse is "active" then the border
	// will appear when the mouse pointer enters the region bounded by the
	// border widget.
	vtkSetClampMacro(ShowEllipse, int, ELLIPSE_OFF, ELLIPSE_ACTIVE);
	vtkGetMacro(ShowEllipse, int);
	void SetShowEllipseToOff() { this->SetShowEllipse(ELLIPSE_OFF); }
	void SetShowEllipseToOn() { this->SetShowEllipse(ELLIPSE_ON); }
	void SetShowEllipseToActive() { this->SetShowEllipse(ELLIPSE_ACTIVE); }

	// Description:
	// Specify the properties of the ellipse.
	vtkGetObjectMacro(EllipseProperty, vtkProperty2D);

	//// Description:
	//// Indicate whether resizing operations should keep the x-y directions
	//// proportional to one another. Also, if ProportionalResize is on, then
	//// the ellipse (Position,Position2) is a bounding ellipse, and the
	//// representation will be placed in the ellipse in such a way as to 
	//// preserve the aspect ratio of the representation.
	//vtkSetMacro(ProportionalResize, int);
	//vtkGetMacro(ProportionalResize, int);
	//vtkBooleanMacro(ProportionalResize, int);

	// Description:
	// Resolution of the circle. Note that resolution should be at least 3 to create a "circular" object. Default = 50
	vtkSetMacro(Resolution, int);
	vtkGetMacro(Resolution, int);

	// Description:
	// Specify a minimum and/or maximum size (in pixels) that this representation
	// can take. These methods require two values: size values in the x and y
	// directions, respectively.
	vtkSetVector2Macro(MinimumSize, int);
	vtkGetVector2Macro(MinimumSize, int);
	vtkSetVector2Macro(MaximumSize, int);
	vtkGetVector2Macro(MaximumSize, int);

	// Description:
	// The tolerance representing the distance to the widget (in pixels)
	// in which the cursor is considered to be on the widget, or on a
	// widget feature (e.g., edge).
	vtkSetClampMacro(Tolerance, double, 0, 1);
	vtkGetMacro(Tolerance, double);

	// Description:
	// After a selection event within the region interior to the ellipse; the
	// normalized selection coordinates may be obtained.
	vtkGetVectorMacro(SelectionPoint, double, 2);

	// Description:
	// This is a modifier of the interaction state. When set, widget interaction
	// allows the border (and stuff inside of it) to be translated with mouse
	// motion.
	vtkSetMacro(Moving, int);
	vtkGetMacro(Moving, int);
	vtkBooleanMacro(Moving, int);

	//BTX
	// Description:
	// Define the various states that the representation can be in.
	enum _InteractionState
	{
		Outside = 0,
		Inside,
		Edge
	};
	//ETX

	// Description:
	// Subclasses should implement these methods. See the superclasses'
	// documentation for more information.
	virtual void BuildRepresentation();
	virtual void StartWidgetInteraction(double eventPos[2]);
	virtual void WidgetInteraction(double eventPos[2]);
	virtual void GetSize(double size[2])
	{
		size[0] = 1.0; size[1] = 1.0;
	}
	virtual int ComputeInteractionState(int X, int Y, int modify = 0);

	// Description:
	// These methods are necessary to make this representation behave as
	// a vtkProp.
	virtual void GetActors2D(vtkPropCollection*);
	virtual void ReleaseGraphicsResources(vtkWindow*);
	virtual int RenderOverlay(vtkViewport*);
	virtual int RenderOpaqueGeometry(vtkViewport*);
	virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
	virtual int HasTranslucentPolygonalGeometry();
	vtkPolyDataMapper2D        *EWMapper;

protected:
	vtkEllipseRepresentation();
	~vtkEllipseRepresentation();

	// Ivars
	int			  Resolution;
	int           ShowEllipse;
	vtkProperty2D *EllipseProperty;
	int           ProportionalResize;
	double        Tolerance;
	int           Moving;
	double        SelectionPoint[2];

	// Layout (position of lower left and upper right corners of ellipse)
	vtkCoordinate *PositionCoordinate;
	vtkCoordinate *Position2Coordinate;

	// Sometimes subclasses must negotiate with their superclasses
	// to achieve the correct layout.
	int Negotiated;
	virtual void NegotiateLayout();

	// Keep track of start position when moving border
	double StartPosition[2];

	// Ellipse representation. Subclasses may use the BWTransform class
	// to transform their geometry into the region surrounded by the border.
	vtkPoints                  *EWPoints;
	vtkActor2D                 *EWActor;
	vtkPolyData                *EWPolyData;
	vtkTransform               *EWTransform;
	vtkTransformPolyDataFilter *EWTransformFilter;

	// Constraints on size
	int MinimumSize[2];
	int MaximumSize[2];

private:
	vtkEllipseRepresentation(const vtkEllipseRepresentation&);  //Not implemented
	void operator=(const vtkEllipseRepresentation&);  //Not implemented

};

#endif