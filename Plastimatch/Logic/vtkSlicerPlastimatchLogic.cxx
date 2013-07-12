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

#include "SlicerRtCommon.h"

// Plastimatch Logic includes
#include "vtkSlicerPlastimatchLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>

//STD includes
#include <string.h>

// ITK includes
#include <itkImageRegionIteratorWithIndex.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>

// Plastimatch includes
#include "plm_config.h"
#include "plm_image_header.h"
#include "plm_warp.h"
#include "plmregister.h"
#include "xform.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlastimatchLogic);

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::vtkSlicerPlastimatchLogic()
{
regp = new Registration_parms();
regd = new Registration_data();
XfOut = 0;
FixedId = new char [256];
MovingId = new char [256];
WarpedImg = new Plm_image();
FixedLandmarks = 0;
MovingLandmarks = 0;
}

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::~vtkSlicerPlastimatchLogic()
{
delete regp;
delete regd;
if (XfOut) delete XfOut;
delete &FixedId;
delete &MovingId;
delete WarpedImg;
if (FixedLandmarks) delete FixedLandmarks;
if (MovingLandmarks) delete MovingLandmarks;
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

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::set_input_images(char* FixedId, char* MovingId)
{
  strcpy(this->FixedId, FixedId);
  vtkMRMLVolumeNode* FixedVtkImg = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->FixedId));
  itk::Image<float, 3>::Pointer FixedItkImg = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(FixedVtkImg, FixedItkImg);
  
  strcpy(this->MovingId, MovingId);
  vtkMRMLVolumeNode* MovingVtkImg = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(this->MovingId));
  itk::Image<float, 3>::Pointer MovingItkImg = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(MovingVtkImg, MovingItkImg);
  
  this->regd->fixed_image = new Plm_image (FixedItkImg);
  this->regd->moving_image = new Plm_image (MovingItkImg);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::set_input_landmarks(char* FixedLandmarkFn, char* MovingLandmarkFn)
{
  FixedLandmarks = new Labeled_pointset();
  FixedLandmarks->load(FixedLandmarkFn);
  regd->fixed_landmarks = this->FixedLandmarks;  
  
  MovingLandmarks = new Labeled_pointset();
  MovingLandmarks->load(MovingLandmarkFn);
  regd->moving_landmarks = this->MovingLandmarks;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
:: add_stage()
{
  this->regp->append_stage();
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::set_par(char* key, char* val)
{    
  this->regp->set_key_val(key, val, 1);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::run_registration (char* OutputImageName)
{
  do_registration_pure (&this->XfOut, this->regd ,this->regp);
  apply_warp(this->WarpedImg, this->XfOut, this->regd->fixed_image, this->regd->moving_image,
    -1200, 0, 1);
  get_output_img(OutputImageName);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::apply_warp(Plm_image* WarpedImg, Xform* XfIn, Plm_image* FixedImg, Plm_image* InImg,
    float DefaultVal, int UseItk, int InterpLin )
{
  Plm_image_header* pih = new Plm_image_header(FixedImg);
  plm_warp(WarpedImg, 0, XfIn, pih, InImg, DefaultVal, UseItk, InterpLin);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::get_output_img (char* OutputImageName)
{
  itk::Image<float, 3>::Pointer OutputImgItk = this->WarpedImg->itk_float();    
  
  vtkSmartPointer<vtkImageData> OutputImgVtk = vtkSmartPointer<vtkImageData>::New();
  itk::Image<float, 3>::RegionType region = OutputImgItk->GetBufferedRegion();
  itk::Image<float, 3>::SizeType imageSize = region.GetSize();
  int extent[6]={0, (int) imageSize[0]-1, 0, (int) imageSize[1]-1, 0, (int) imageSize[2]-1};
  OutputImgVtk->SetExtent(extent);
  OutputImgVtk->SetScalarType(VTK_FLOAT);
  OutputImgVtk->SetNumberOfScalarComponents(1);
  OutputImgVtk->AllocateScalars();
  
  float* OutputImgPtr = (float*)OutputImgVtk->GetScalarPointer();
  itk::ImageRegionIteratorWithIndex< itk::Image<float, 3> > ItOutputImgItk(
  OutputImgItk, OutputImgItk->GetLargestPossibleRegion() );
  
  for ( ItOutputImgItk.GoToBegin(); !ItOutputImgItk.IsAtEnd(); ++ItOutputImgItk)
  {
    itk::Image<float, 3>::IndexType i = ItOutputImgItk.GetIndex();
    (*OutputImgPtr) = OutputImgItk->GetPixel(i);
    OutputImgPtr++;
  }
  
  // Read fixed image to get the geometrical information
  vtkMRMLVolumeNode* FixedVtkImg = vtkMRMLVolumeNode::SafeDownCast(
  this->GetMRMLScene()->GetNodeByID(this->FixedId));
  
  // Create new image node
  vtkSmartPointer<vtkMRMLScalarVolumeNode> WarpedImgNode = vtkSmartPointer<vtkMRMLScalarVolumeNode>::New();
  WarpedImgNode->SetAndObserveImageData (OutputImgVtk);
  WarpedImgNode->SetSpacing (
    OutputImgItk->GetSpacing()[0],
    OutputImgItk->GetSpacing()[1],
    OutputImgItk->GetSpacing()[2]);
  WarpedImgNode->SetOrigin (
    OutputImgItk->GetOrigin()[0],
    OutputImgItk->GetOrigin()[1],
    OutputImgItk->GetOrigin()[2]);
  std::string WarpedImgName = this->GetMRMLScene()->GenerateUniqueName(OutputImageName);
  WarpedImgNode->SetName(WarpedImgName.c_str());
  
  WarpedImgNode->SetScene(this->GetMRMLScene());
  WarpedImgNode->CopyOrientation(FixedVtkImg);
  this->GetMRMLScene()->AddNode(WarpedImgNode);
}

