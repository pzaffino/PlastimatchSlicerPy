/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#include <string.h>

#include "SlicerRtCommon.h"

// Plastimatch Logic includes
#include "vtkSlicerPlastimatchLogic.h"

// MRML includes

// VTK includes
#include <vtkNew.h>

// STD includes
#include <cassert>

#include "plm_config.h"
#include "plmregister.h"
#include "plm_config.h"
#include "plm_stages.h"
#include "registration_parms.h"
#include "registration_data.h"
#include "xform.h"
#include "plm_warp.h"
#include "plm_image_header.h"
#include "plm_image.h"

#include <itkImageRegionIteratorWithIndex.h>
#include <vtkSmartPointer.h>

#include <vtkMRMLScalarVolumeNode.h>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlastimatchLogic);

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::vtkSlicerPlastimatchLogic()
{
regp = new Registration_parms();
regd = new Registration_data();
xf_out = NULL;
warped_img = new Plm_image();
}

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::~vtkSlicerPlastimatchLogic()
{
delete regp;
delete regd;
delete xf_out;
delete warped_img;
}

//----------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

void vtkSlicerPlastimatchLogic
::set_input_images(std::string fixed_id, std::string moving_id)
{

    this->fixed_id=fixed_id;
    vtkMRMLVolumeNode* fixed_vtk_img = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->fixed_id.c_str()));
    itk::Image<float, 3>::Pointer fixed_itk_img = itk::Image<float, 3>::New();
    SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(fixed_vtk_img, fixed_itk_img);

    this->moving_id=moving_id;
    vtkMRMLVolumeNode* moving_vtk_img = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->moving_id.c_str()));
    itk::Image<float, 3>::Pointer moving_itk_img = itk::Image<float, 3>::New();
    SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(moving_vtk_img, moving_itk_img);
    
    this->regd->fixed_image = new Plm_image (fixed_itk_img);
    this->regd->moving_image = new Plm_image (moving_itk_img);

}

void vtkSlicerPlastimatchLogic
::set_par(std::string key, std::string val)
{    
    this->regp->set_key_val(key.c_str(), val.c_str(), 1);
}

void vtkSlicerPlastimatchLogic
:: add_stage()
{
    this->regp->append_stage();
}


void vtkSlicerPlastimatchLogic
::apply_warp(Plm_image *im_warped,   /* Output: Output image */
    Xform * xf_in,          /* Input:  Input image warped by this xform */
    Plm_image * fixed_img,   /* Input:  Size of output image */
    Plm_image * im_in,       /* Input:  Input image */
    float default_val,     /* Input:  Value for pixels without match */
    int use_itk,           /* Input:  Force use of itk (1) or not (0) */
    int interp_lin )

{
    Plm_image_header * pih = new Plm_image_header(fixed_img);  
    

    plm_warp(im_warped,   /* Output: Output image */
    0,    /* Output: Output vf (optional) */
    xf_in,          /* Input:  Input image warped by this xform */
    pih,   /* Input:  Size of output image */
    im_in,       /* Input:  Input image */
    default_val,     /* Input:  Value for pixels without match */
    use_itk,           /* Input:  Force use of itk (1) or not (0) */
    interp_lin);
}



void vtkSlicerPlastimatchLogic
::get_output_img (std::string output_image_name)
{
    itk::Image<float, 3>::Pointer output_img_itk = this->warped_img->itk_float();    

    vtkSmartPointer<vtkImageData> output_img_vtk = vtkSmartPointer<vtkImageData>::New();
    itk::Image<float, 3>::RegionType region = output_img_itk->GetBufferedRegion();
    itk::Image<float, 3>::SizeType imageSize = region.GetSize();
    int extent[6]={0, (int) imageSize[0]-1, 0, (int) imageSize[1]-1, 0, (int) imageSize[2]-1};
    output_img_vtk->SetExtent(extent);
    output_img_vtk->SetScalarType(VTK_FLOAT);
    output_img_vtk->SetNumberOfScalarComponents(1);
    output_img_vtk->AllocateScalars();
    
    float* output_img_Ptr = (float*)output_img_vtk->GetScalarPointer();
    itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > it_output_img_itk(
    output_img_itk, output_img_itk->GetLargestPossibleRegion() );

    for ( it_output_img_itk.GoToBegin(); !it_output_img_itk.IsAtEnd(); ++it_output_img_itk)
    {
        itk::Image<float, 3>::IndexType i = it_output_img_itk.GetIndex();
        (*output_img_Ptr) = output_img_itk->GetPixel(i);
        output_img_Ptr++;
    }

    // Read fixed image to get the geometrical information
    vtkMRMLVolumeNode* fixed_vtk_img = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->fixed_id.c_str()));

   // Create new image node
   vtkSmartPointer<vtkMRMLScalarVolumeNode> warped_img_node = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
   warped_img_node->SetAndObserveImageData (output_img_vtk);
   warped_img_node->SetSpacing (
        output_img_itk->GetSpacing()[0],
        output_img_itk->GetSpacing()[1],
        output_img_itk->GetSpacing()[2]);
    warped_img_node->SetOrigin (
        output_img_itk->GetOrigin()[0],
        output_img_itk->GetOrigin()[1],
        output_img_itk->GetOrigin()[2]);
    std::string warped_img_name = this->GetMRMLScene()->GenerateUniqueName(output_image_name);
    warped_img_node->SetName(warped_img_name.c_str());

    warped_img_node->SetScene(this->GetMRMLScene());
    warped_img_node->CopyOrientation(fixed_vtk_img);
    this->GetMRMLScene()->AddNode(warped_img_node);
}

void vtkSlicerPlastimatchLogic
::run_plm ()
{
    do_registration_pure (&this->xf_out, this->regd ,this->regp);
    apply_warp(this->warped_img, this->xf_out, this->regd->fixed_image, this->regd->moving_image,
    -1200, 0, 1);
}
