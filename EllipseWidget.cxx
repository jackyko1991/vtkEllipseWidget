#include <vtkSmartPointer.h>
#include <vtkWidgetCallbackMapper.h>
#include <vtkCommand.h>
#include <vtkWidgetEvent.h>
#include <vtkObjectFactory.h>
#include <vtkActor.h>
#include <vtkBorderRepresentation.h>
#include <vtkBorderWidget.h>
#include <vtkEllipseRepresentation.h>
#include <vtkEllipseWidget.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkActor2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor2D.h>

class vtkCustomEllipseWidget : public vtkEllipseWidget
{
public:
	static vtkCustomEllipseWidget *New();
	vtkTypeMacro(vtkCustomEllipseWidget, vtkBorderWidget);

	static void EndSelectAction(vtkAbstractWidget *w);

	vtkCustomEllipseWidget();

};

vtkStandardNewMacro(vtkCustomEllipseWidget);

vtkCustomEllipseWidget::vtkCustomEllipseWidget()
{
	this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
		vtkWidgetEvent::EndSelect,
		this, vtkCustomEllipseWidget::EndSelectAction);
}

void vtkCustomEllipseWidget::EndSelectAction(vtkAbstractWidget *w)
{
	vtkEllipseWidget *ellipseWidget =
		vtkEllipseWidget::SafeDownCast(w);

	// Get the actual box coordinates/planes
	vtkSmartPointer<vtkPolyData> polydata =
		vtkSmartPointer<vtkPolyData>::New();

	//// Get the bottom left corner
	//double* lowerLeft;
	//lowerLeft = static_cast<vtkEllipseRepresentation*>(ellipseWidget->GetRepresentation())->GetPosition();
	//std::cout << "Lower left: " << lowerLeft[0] << " "
	//	<< lowerLeft[1] << std::endl;

	//double* upperRight;
	//upperRight = static_cast<vtkEllipseRepresentation*>(ellipseWidget->GetRepresentation())->GetPosition2();
	//std::cout << "Upper right: " << upperRight[0] << " "
	//	<< upperRight[1] << std::endl;

	vtkEllipseWidget::EndSelectAction(w);
}

int main(int, char *[])
{
	// Sphere
	vtkSmartPointer<vtkSphereSource> sphereSource =
		vtkSmartPointer<vtkSphereSource>::New();
	sphereSource->SetRadius(0.1);
	sphereSource->SetCenter(0, 0, 0);
	sphereSource->Update();

	vtkSmartPointer<vtkPolyDataMapper> mapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(sphereSource->GetOutputPort());

	vtkSmartPointer<vtkActor> actor =
		vtkSmartPointer<vtkActor>::New();
	actor->SetMapper(mapper);

	// A renderer and render window
	vtkSmartPointer<vtkRenderer> renderer =
		vtkSmartPointer<vtkRenderer>::New();
	vtkSmartPointer<vtkRenderWindow> renderWindow =
		vtkSmartPointer<vtkRenderWindow>::New();
	renderWindow->AddRenderer(renderer);

	// An interactor
	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
		vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renderWindowInteractor->SetRenderWindow(renderWindow);

	vtkSmartPointer<vtkCustomEllipseWidget> ellipseWidget =
		vtkSmartPointer<vtkCustomEllipseWidget>::New();
	ellipseWidget->SetInteractor(renderWindowInteractor);
	ellipseWidget->CreateDefaultRepresentation();
	ellipseWidget->SelectableOff();
	//ellipseWidget->GetEllipseRepresentation()->MovingOff();

	// Add the actors to the scene
	renderer->AddActor(actor);
	//vtkEllipseRepresentation* representation = static_cast<vtkEllipseRepresentation*>(ellipseWidget->GetRepresentation());

	//auto actor2D= vtkSmartPointer<vtkActor2D>::New();
	//actor2D->SetMapper(representation->EWMapper);
	//renderer->AddActor2D(actor2D);

	// Render an image (lights and cameras are created automatically)
	renderWindowInteractor->Initialize();
	renderWindow->Render();
	ellipseWidget->On();

	renderer->ResetCamera();
	// Begin mouse interaction
	renderWindowInteractor->Start();

	return EXIT_SUCCESS;
}