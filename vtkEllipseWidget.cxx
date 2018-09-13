/*=========================================================================
Program:   Visualization Toolkit
Module:    vtkEllipseWidget.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen, Ko Ka Long
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkEllipseWidget.h"
#include "vtkEllipseRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkEllipseWidget);


//-------------------------------------------------------------------------
vtkEllipseWidget::vtkEllipseWidget()
{
	this->WidgetState = vtkEllipseWidget::Start;
	this->Selectable = 1;
	this->Resizable = 1;

	this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
		vtkWidgetEvent::Select,
		this, vtkEllipseWidget::SelectAction);
	this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
		vtkWidgetEvent::EndSelect,
		this, vtkEllipseWidget::EndSelectAction);
	this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
		vtkWidgetEvent::Translate,
		this, vtkEllipseWidget::TranslateAction);
	this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
		vtkWidgetEvent::EndSelect,
		this, vtkEllipseWidget::EndSelectAction);
	this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
		vtkWidgetEvent::Move,
		this, vtkEllipseWidget::MoveAction);
}

//-------------------------------------------------------------------------
vtkEllipseWidget::~vtkEllipseWidget() = default;

//-------------------------------------------------------------------------
void vtkEllipseWidget::SetCursor(int cState)
{
	if (!this->Resizable && cState != vtkEllipseRepresentation::Inside)
	{
		this->RequestCursorShape(VTK_CURSOR_DEFAULT);
		return;
	}

	switch (cState)
	{
	case vtkEllipseRepresentation::AdjustingP0:
		this->RequestCursorShape(VTK_CURSOR_SIZENS);
		break;
	case vtkEllipseRepresentation::AdjustingP1:
		this->RequestCursorShape(VTK_CURSOR_SIZEWE);
		break;
	case vtkEllipseRepresentation::AdjustingP2:
		this->RequestCursorShape(VTK_CURSOR_SIZENS);
		break;
	case vtkEllipseRepresentation::AdjustingP3:
		this->RequestCursorShape(VTK_CURSOR_SIZEWE);
		break;
	case vtkEllipseRepresentation::Inside:
		if (reinterpret_cast<vtkEllipseRepresentation*>(this->WidgetRep)->GetMoving())
		{
			this->RequestCursorShape(VTK_CURSOR_SIZEALL);
		}
		else
		{
			this->RequestCursorShape(VTK_CURSOR_HAND);
		}
		break;
	default:
		this->RequestCursorShape(VTK_CURSOR_DEFAULT);
	}
}

void vtkEllipseWidget::SetEdgeColor(int cState)
{
	switch (cState)
	{
	case vtkEllipseRepresentation::AdjustingP0:
	case vtkEllipseRepresentation::AdjustingP1:
	case vtkEllipseRepresentation::AdjustingP2:
	case vtkEllipseRepresentation::AdjustingP3:
	case vtkEllipseRepresentation::Edge:
	{
		vtkSmartPointer<vtkPropCollection>actorCollection = vtkSmartPointer<vtkPropCollection>::New();
		this->GetEllipseRepresentation()->GetActors2D(actorCollection);
		vtkActor2D* ellipseActor = (vtkActor2D*)(actorCollection->GetLastProp());
		ellipseActor->GetProperty()->SetColor(1, 1, 0);
		this->Interactor->Render();
		break;
	}
	default:
	{
		vtkSmartPointer<vtkPropCollection>actorCollection = vtkSmartPointer<vtkPropCollection>::New();
		this->GetEllipseRepresentation()->GetActors2D(actorCollection);
		vtkActor2D* ellipseActor = (vtkActor2D*)(actorCollection->GetLastProp());
		ellipseActor->GetProperty()->SetColor(1, 1, 1);
		this->Interactor->Render();
		break;
	}

	}
}

//-------------------------------------------------------------------------
void vtkEllipseWidget::SelectAction(vtkAbstractWidget *w)
{
	vtkEllipseWidget *self = reinterpret_cast<vtkEllipseWidget*>(w);

	if (self->SubclassSelectAction() ||
		self->WidgetRep->GetInteractionState() == vtkEllipseRepresentation::Outside)
	{
		return;
	}

	// We are definitely selected
	self->GrabFocus(self->EventCallbackCommand);
	self->WidgetState = vtkEllipseWidget::Selected;

	// Picked something inside the widget
	int X = self->Interactor->GetEventPosition()[0];
	int Y = self->Interactor->GetEventPosition()[1];

	// This is redundant but necessary on some systems (windows) because the
	// cursor is switched during OS event processing and reverts to the default
	// cursor (i.e., the MoveAction may have set the cursor previously, but this
	// method is necessary to maintain the proper cursor shape)..
	self->SetCursor(self->WidgetRep->GetInteractionState());

	// convert to normalized viewport coordinates
	double XF = static_cast<double>(X);
	double YF = static_cast<double>(Y);
	self->CurrentRenderer->DisplayToNormalizedDisplay(XF, YF);
	self->CurrentRenderer->NormalizedDisplayToViewport(XF, YF);
	self->CurrentRenderer->ViewportToNormalizedViewport(XF, YF);
	double eventPos[2];
	eventPos[0] = XF;
	eventPos[1] = YF;
	self->WidgetRep->StartWidgetInteraction(eventPos);

	if (self->Selectable &&
		self->WidgetRep->GetInteractionState() == vtkEllipseRepresentation::Inside)
	{
		vtkEllipseRepresentation *rep = reinterpret_cast<vtkEllipseRepresentation*>(self->WidgetRep);
		double *fpos1 = rep->GetPositionCoordinate()->GetValue();
		double *fpos2 = rep->GetPosition2Coordinate()->GetValue();

		eventPos[0] = (XF - fpos1[0]) / fpos2[0];
		eventPos[1] = (YF - fpos1[1]) / fpos2[1];

		self->SelectRegion(eventPos);
	}

	self->EventCallbackCommand->SetAbortFlag(1);
	self->StartInteraction();
	self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void vtkEllipseWidget::TranslateAction(vtkAbstractWidget *w)
{
	vtkEllipseWidget *self = reinterpret_cast<vtkEllipseWidget*>(w);

	if (self->SubclassTranslateAction() ||
		self->WidgetRep->GetInteractionState() == vtkEllipseRepresentation::Outside)
	{
		return;
	}

	// We are definitely selected
	self->GrabFocus(self->EventCallbackCommand);
	self->WidgetState = vtkEllipseWidget::Selected;
	reinterpret_cast<vtkEllipseRepresentation*>(self->WidgetRep)->MovingOn();

	// Picked something inside the widget
	int X = self->Interactor->GetEventPosition()[0];
	int Y = self->Interactor->GetEventPosition()[1];

	// This is redundant but necessary on some systems (windows) because the
	// cursor is switched during OS event processing and reverts to the default
	// cursor.
	self->SetCursor(self->WidgetRep->GetInteractionState());

	// convert to normalized viewport coordinates
	double XF = static_cast<double>(X);
	double YF = static_cast<double>(Y);
	self->CurrentRenderer->DisplayToNormalizedDisplay(XF, YF);
	self->CurrentRenderer->NormalizedDisplayToViewport(XF, YF);
	self->CurrentRenderer->ViewportToNormalizedViewport(XF, YF);
	double eventPos[2];
	eventPos[0] = XF;
	eventPos[1] = YF;
	self->WidgetRep->StartWidgetInteraction(eventPos);

	self->EventCallbackCommand->SetAbortFlag(1);
	self->StartInteraction();
	self->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void vtkEllipseWidget::MoveAction(vtkAbstractWidget *w)
{
	vtkEllipseWidget *self = reinterpret_cast<vtkEllipseWidget*>(w);

	if (self->SubclassMoveAction())
	{
		return;
	}

	// compute some info we need for all cases
	int X = self->Interactor->GetEventPosition()[0];
	int Y = self->Interactor->GetEventPosition()[1];

	// Set the cursor appropriately
	if (self->WidgetState == vtkEllipseWidget::Start)
	{
		int stateBefore = self->WidgetRep->GetInteractionState();
		self->WidgetRep->ComputeInteractionState(X, Y);
		int stateAfter = self->WidgetRep->GetInteractionState();
		self->SetCursor(stateAfter);
		self->SetEdgeColor(stateAfter);

		vtkEllipseRepresentation* EllipseRepresentation =
			reinterpret_cast<vtkEllipseRepresentation*>(self->WidgetRep);
		if (self->Selectable || stateAfter != vtkEllipseRepresentation::Inside)
		{
			EllipseRepresentation->MovingOff();
		}
		else
		{
			EllipseRepresentation->MovingOn();
		}

		if (EllipseRepresentation->GetShowEllipse() == vtkEllipseRepresentation::ELLIPSE_ACTIVE &&
			stateBefore != stateAfter &&
			(stateBefore == vtkEllipseRepresentation::Outside || stateAfter == vtkEllipseRepresentation::Outside))
		{
			self->Render();
		}
		return;
	}

	if (!self->Resizable &&
		self->WidgetRep->GetInteractionState() != vtkEllipseRepresentation::Inside)
	{
		return;
	}

	// Okay, adjust the representation (the widget is currently selected)
	double newEventPosition[2];
	newEventPosition[0] = static_cast<double>(X);
	newEventPosition[1] = static_cast<double>(Y);
	self->WidgetRep->WidgetInteraction(newEventPosition);

	// start a drag
	self->EventCallbackCommand->SetAbortFlag(1);
	self->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
	self->Render();
}

//-------------------------------------------------------------------------
void vtkEllipseWidget::EndSelectAction(vtkAbstractWidget *w)
{
	vtkEllipseWidget *self = reinterpret_cast<vtkEllipseWidget*>(w);

	if (self->SubclassEndSelectAction() ||
		self->WidgetRep->GetInteractionState() == vtkEllipseRepresentation::Outside ||
		self->WidgetState != vtkEllipseWidget::Selected)
	{
		return;
	}

	// Return state to not selected
	self->ReleaseFocus();
	self->WidgetState = vtkEllipseWidget::Start;
	reinterpret_cast<vtkEllipseRepresentation*>(self->WidgetRep)->MovingOff();

	// stop adjusting
	self->EventCallbackCommand->SetAbortFlag(1);
	self->EndInteraction();
	self->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------
void vtkEllipseWidget::CreateDefaultRepresentation()
{
	if (!this->WidgetRep)
	{
		this->WidgetRep = vtkEllipseRepresentation::New();
	}
}

//-------------------------------------------------------------------------
void vtkEllipseWidget::SelectRegion(double* vtkNotUsed(eventPos[2]))
{
	this->InvokeEvent(vtkCommand::WidgetActivateEvent, nullptr);
}

//-------------------------------------------------------------------------
void vtkEllipseWidget::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);

	os << indent << "Selectable: " << (this->Selectable ? "On\n" : "Off\n");
	os << indent << "Resizable: " << (this->Resizable ? "On\n" : "Off\n");
}