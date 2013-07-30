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
#include <vtkMRMLAnnotationFiducialNode.h>
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>

// STD includes
#include <string.h>

// ITK includes
#include <itkAffineTransform.h>
#include <itkArray.h>
#include <itkImageRegionIteratorWithIndex.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>

// Plastimatch includes
#include "bspline_interpolate.h"
#include "plm_config.h"
#include "plm_image_header.h"
#include "plm_warp.h"
#include "plmregister.h"
#include "pointset.h"
#include "xform.h"
#include "raw_pointset.h"
#include "volume.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlastimatchLogic);

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::vtkSlicerPlastimatchLogic()
{
  this->FixedId=NULL;
  this->MovingId=NULL;
  this->FixedLandmarksFn=NULL;
  this->MovingLandmarksFn=NULL;
  this->LandmarksWarp=NULL;
  this->regp=new Registration_parms();
  this->regd=new Registration_data();
  this->InputXfId=NULL;
  this->XfIn=NULL;
  this->XfOut=NULL;
  this->WarpedImg=new Plm_image();
  this->OutputImageName=NULL;
}

//----------------------------------------------------------------------------
vtkSlicerPlastimatchLogic::~vtkSlicerPlastimatchLogic()
{
  this->SetFixedId(NULL);
  this->SetMovingId(NULL);
  this->SetFixedLandmarksFn(NULL);
  this->SetMovingLandmarksFn(NULL);
  this->LandmarksWarp=NULL;
  this->regp=NULL;
  this->regd=NULL;
  this->SetInputXfId(NULL);
  this->XfIn=NULL;
  this->XfOut=NULL;
  this->WarpedImg=NULL;
  this->SetOutputImageName(NULL);
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
:: AddLandmark(char* landmarkId, char* landmarkType)
{
  vtkMRMLAnnotationFiducialNode* slicerLandmark = vtkMRMLAnnotationFiducialNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(landmarkId));
  
  double points[3] = {0.0};
  slicerLandmark->GetFiducialCoordinates(points);
  
  Point3d landmark;
  landmark.coord[0]=points[0];
  landmark.coord[1]=points[1];
  landmark.coord[2]=points[2];
  
  if (!strcmp(landmarkType, "fixed") ||
      !strcmp(landmarkType, "FIXED") ||
      !strcmp(landmarkType, "Fixed"))
  {
    this->FixedLandmarks.push_front(landmark);
    this->FixedLandmarksIds.push_front(landmarkId);
  }

  else if (!strcmp(landmarkType, "moving") ||
           !strcmp(landmarkType, "MOVING") ||
           !strcmp(landmarkType, "Moving"))
  {
    this->MovingLandmarks.push_front(landmark);
    this->MovingLandmarksIds.push_front(landmarkId);
  }
  else {
    printf("Unknow landmark type!\n");
  }

}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
:: AddStage()
{
  this->regp->append_stage();
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::SetPar(char* key, char* val)
{    
  this->regp->set_key_val(key, val, 1);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::RunRegistration()
{
  // Set input images
  vtkMRMLVolumeNode* FixedVtkImg = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(GetFixedId()));
  itk::Image<float, 3>::Pointer FixedItkImg = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(FixedVtkImg, FixedItkImg);

  vtkMRMLVolumeNode* MovingVtkImg = vtkMRMLVolumeNode::SafeDownCast(
    this->GetMRMLScene()->GetNodeByID(GetMovingId()));
  itk::Image<float, 3>::Pointer MovingItkImg = itk::Image<float, 3>::New();
  SlicerRtCommon::ConvertVolumeNodeToItkImage<float>(MovingVtkImg, MovingItkImg);

  this->regd->fixed_image = new Plm_image (FixedItkImg);
  this->regd->moving_image = new Plm_image (MovingItkImg);
  
  // Set landmarks 
  if (!this->FixedLandmarks.empty() && !this->MovingLandmarks.empty() &&
           this->FixedLandmarks.size() == this->MovingLandmarks.size())
  {
    // From Slicer
    SetLandmarksFromSlicer(); 
  }
  else if (GetFixedLandmarksFn() != NULL && GetFixedLandmarksFn() != NULL)
  {
    // From Files
    SetLandmarksFromFiles(); 
  }
  
  // Set initial affine transformation
  if (GetInputXfId() != NULL)
  {
    ApplyInitialLinearTransformation(); 
  } 
  
  // Run registration and warp image
  do_registration_pure (&this->XfOut, this->regd ,this->regp);
  ApplyWarp(this->WarpedImg, NULL, this->XfOut, this->regd->fixed_image,
            this->regd->moving_image, -1200, 0, 1);
  GetOutputImg(GetOutputImageName());
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::SetLandmarksFromSlicer()
{
    Labeled_pointset* fixedLandmarksSet = new Labeled_pointset();
    Labeled_pointset* movingLandmarksSet = new Labeled_pointset();

    std::list<Point3d>::iterator fixedLandmarkIt;
    std::list<Point3d>::iterator movingLandmarkIt;

    // Set all the fixed landmarks
    for (fixedLandmarkIt = this->FixedLandmarks.begin();
         fixedLandmarkIt != this->FixedLandmarks.end(); fixedLandmarkIt++) {

      Labeled_point* fixedLandmark = new Labeled_point("point",
                                       - fixedLandmarkIt->coord[0],
                                       - fixedLandmarkIt->coord[1],
                                       fixedLandmarkIt->coord[2]);

      fixedLandmarksSet->point_list.push_back(*fixedLandmark);
    }

    // Set all the moving landmarks
    for (movingLandmarkIt = this->MovingLandmarks.begin();
         movingLandmarkIt != this->MovingLandmarks.end(); movingLandmarkIt++) {

      Labeled_point* movingLandmark = new Labeled_point("point",
                                       - movingLandmarkIt->coord[0],
                                       - movingLandmarkIt->coord[1],
                                       movingLandmarkIt->coord[2]);

      movingLandmarksSet->point_list.push_back(*movingLandmark);
    }

    regd->fixed_landmarks = fixedLandmarksSet;
    regd->moving_landmarks = movingLandmarksSet;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::SetLandmarksFromFiles()
{
    Labeled_pointset* FixedLandmarksFromFile = new Labeled_pointset();
    FixedLandmarksFromFile->load(GetFixedLandmarksFn());
    regd->fixed_landmarks = FixedLandmarksFromFile;

    Labeled_pointset* MovingLandmarksFromFile = new Labeled_pointset();
    MovingLandmarksFromFile->load(GetMovingLandmarksFn());
    regd->moving_landmarks = MovingLandmarksFromFile;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::ApplyInitialLinearTransformation()
{
    // Get transformation as 4x4 matrix
    vtkMRMLLinearTransformNode* inputTransformation = vtkMRMLLinearTransformNode::SafeDownCast(
      this->GetMRMLScene()->GetNodeByID(GetInputXfId()));
    vtkMatrix4x4* inputVtkTransformation = inputTransformation->GetMatrixTransformToParent();

    // Create ITK array to store the parameters
    itk::Array<double> affineParameters;
    affineParameters.SetSize(12);

    // Set rotations
    printf("TRANSFORMATION: ");
    int index=0;
    for (int i=0; i < 3; i++) {
      for (int j=0; j < 3; j++) {
        affineParameters.SetElement(index, inputVtkTransformation->GetElement(j,i));
        printf("%g ", affineParameters.GetElement(index));
        index++;
      }
    }

    // Set translations
    affineParameters.SetElement(9, inputVtkTransformation->GetElement(0,3));
    affineParameters.SetElement(10, inputVtkTransformation->GetElement(1,3));
    affineParameters.SetElement(11, inputVtkTransformation->GetElement(2,3));
    printf("%g ", affineParameters.GetElement(9));
    printf("%g ", affineParameters.GetElement(10));
    printf("%g \n", affineParameters.GetElement(11));

    // Create ITK affine transformation
    itk::AffineTransform<double, 3>::Pointer inputItkTransformation = itk::AffineTransform<double, 3>::New();
    inputItkTransformation->SetParameters(affineParameters);

    // Set transformation
    this->XfIn = new Xform;
    this->XfIn->set_aff(inputItkTransformation);

    // Warp image using the input transformation
    Plm_image* outputImageFromInputXf = new Plm_image;
    ApplyWarp(outputImageFromInputXf, NULL, this->XfIn, this->regd->fixed_image,
              this->regd->moving_image, -1200, 0, 1);

    // Update moving image
    this->regd->moving_image=outputImageFromInputXf;
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::ApplyWarp(Plm_image* WarpedImg, DeformationFieldType::Pointer* VectorFieldOut, Xform* XfIn,
            Plm_image* FixedImg, Plm_image* InputImg, float DefaultVal, int UseItk, int InterpLin )
{
  Plm_image_header* pih = new Plm_image_header(FixedImg);
  plm_warp(WarpedImg, VectorFieldOut, XfIn, pih, InputImg, DefaultVal, UseItk, InterpLin);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::GetOutputImg (char* PublicOutputImageName)
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
  this->GetMRMLScene()->GetNodeByID(GetFixedId()));
  
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
  std::string WarpedImgName = this->GetMRMLScene()->GenerateUniqueName(PublicOutputImageName);
  WarpedImgNode->SetName(WarpedImgName.c_str());
  
  WarpedImgNode->SetScene(this->GetMRMLScene());
  WarpedImgNode->CopyOrientation(FixedVtkImg);
  this->GetMRMLScene()->AddNode(WarpedImgNode);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::WarpLandmarks()
{
  Volume *vector_field;
  vector_field = new Volume (this->regd->fixed_image->get_volume()->dim,
                             this->regd->fixed_image->get_volume()->offset,
                             this->regd->fixed_image->get_volume()->spacing,
                             this->regd->fixed_image->get_volume()->direction_cosines,
                             PT_VF_FLOAT_INTERLEAVED, 3);
  bspline_interpolate_vf (vector_field, this->XfOut->get_gpuit_bsp() );

  this->LandmarksWarp->m_fixed_landmarks  = pointset_create ();
  this->LandmarksWarp->m_moving_landmarks = pointset_create ();
  
  for(std::vector<Labeled_point>::iterator it =
      regd->fixed_landmarks->point_list.begin();
      it != regd->fixed_landmarks->point_list.end();
      ++it)
  { 
     float lm[3];
     lm[0]=(*it).p[0];
     lm[1]=(*it).p[1];
     lm[2]=(*it).p[2];
     pointset_add_point_noadjust( this->LandmarksWarp->m_fixed_landmarks, lm);
  }
  
  for(std::vector<Labeled_point>::iterator it =
      regd->moving_landmarks->point_list.begin();
      it != regd->moving_landmarks->point_list.end();
      ++it)
  {
    float lm[3];
    lm[0]=(*it).p[0];
    lm[1]=(*it).p[1];
    lm[2]=(*it).p[2];
    pointset_add_point_noadjust( this->LandmarksWarp->m_moving_landmarks, lm);
  }
  
  Volume* moving_ss_clone = new Volume();
  moving_ss_clone = this->regd->fixed_image->get_volume()->clone_raw();
  Plm_image* moving_ss_plmimage = new Plm_image();
  moving_ss_plmimage->set_volume (moving_ss_clone);
  
  this->LandmarksWarp->m_input_img = moving_ss_plmimage;
  this->LandmarksWarp->m_pih.set_from_plm_image(this->LandmarksWarp->m_input_img);
  
  this->LandmarksWarp->m_warped_landmarks = pointset_create ();
  calculate_warped_landmarks_by_vf(this->LandmarksWarp, vector_field);
}

//---------------------------------------------------------------------------
void vtkSlicerPlastimatchLogic
::UpdateMovingLandmarksToWarpedLandmarks()
{
  std::list<Point3d>::iterator movingLandmarkIt;
  std::list<std::string>::iterator movingLandmarkIdsIt;
  int i;
  
  // Update this->MovingLandmarks
  for (movingLandmarkIt = this->MovingLandmarks.begin(), i=0;
       movingLandmarkIt != this->MovingLandmarks.end(); movingLandmarkIt++, i++) {
    movingLandmarkIt->coord[0] = this->LandmarksWarp->m_warped_landmarks->points[i*3+0];
    movingLandmarkIt->coord[1] = this->LandmarksWarp->m_warped_landmarks->points[i*3+1];
    movingLandmarkIt->coord[2] = this->LandmarksWarp->m_warped_landmarks->points[i*3+2];
  }
  
  // Update this->regd->moving_landmarks
  for (i=0; i < (int) this->regd->moving_landmarks->point_list.size(); i++) {
    this->regd->moving_landmarks->point_list[i].p[0] =
        this->LandmarksWarp->m_warped_landmarks->points[i*3+0];
    this->regd->moving_landmarks->point_list[i].p[1] =
        this->LandmarksWarp->m_warped_landmarks->points[i*3+1];
    this->regd->moving_landmarks->point_list[i].p[2] =
        this->LandmarksWarp->m_warped_landmarks->points[i*3+2];
  }

  // Update landmarks in Slicer
  vtkMRMLAnnotationFiducialNode* slicerLandmark;
  for (movingLandmarkIdsIt = this->MovingLandmarksIds.end(), i=0;
       movingLandmarkIdsIt != this->MovingLandmarksIds.begin(); movingLandmarkIdsIt--, i++) {
    slicerLandmark = vtkMRMLAnnotationFiducialNode::SafeDownCast(
                       this->GetMRMLScene()->GetNodeByID(*movingLandmarkIdsIt));
    slicerLandmark->SetFiducialCoordinates(this->LandmarksWarp->m_warped_landmarks->points[i*3+0],
                                           this->LandmarksWarp->m_warped_landmarks->points[i*3+1],
                                           this->LandmarksWarp->m_warped_landmarks->points[i*3+2]);
  }

}
