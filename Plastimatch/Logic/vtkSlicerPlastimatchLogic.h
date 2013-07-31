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

// .NAME vtkSlicerPlastimatchLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes

#ifndef __vtkSlicerPlastimatchLogic_h
#define __vtkSlicerPlastimatchLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// Plastimatch Module Logic
#include "vtkSlicerPlastimatchModuleLogicExport.h"

// STD includes
#include <cstdlib>

// ITK includes
#include "itkImage.h"

// VTK includes
#include <vtkPoints.h>

// Plastimatch includes
#include "landmark_warp.h"
#include "plm_config.h"
#include "plm_image.h"
#include "plm_stages.h"
#include "pointset.h"
#include "registration_data.h"
#include "registration_parms.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLASTIMATCH_MODULE_LOGIC_EXPORT vtkSlicerPlastimatchLogic :
  public vtkSlicerModuleLogic
{
  typedef itk::Vector< float, 3 >  VectorType;
  typedef itk::Image< VectorType, 3 >  DeformationFieldType;

public:
  static vtkSlicerPlastimatchLogic *New();
  vtkTypeMacro(vtkSlicerPlastimatchLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  void AddStage();
  void SetPar(char* key, char* value);
  void RunRegistration();

public:
  vtkSetStringMacro(FixedID);
  vtkGetStringMacro(FixedID);
  vtkSetStringMacro(MovingID);
  vtkGetStringMacro(MovingID);

  vtkSetMacro(FixedLandmarks, vtkPoints*);
  vtkGetMacro(FixedLandmarks, vtkPoints*);

  vtkSetMacro(MovingLandmarks, vtkPoints*);
  vtkGetMacro(MovingLandmarks, vtkPoints*);
  
  vtkGetMacro(WarpedLandmarks, vtkPoints*);
  vtkSetMacro(WarpedLandmarks, vtkPoints*);
  
  vtkSetStringMacro(FixedLandmarksFileName);
  vtkGetStringMacro(FixedLandmarksFileName);
  vtkSetStringMacro(MovingLandmarksFileName);
  vtkGetStringMacro(MovingLandmarksFileName);

  vtkSetStringMacro(InputTransformationID);
  vtkGetStringMacro(InputTransformationID);

  vtkSetStringMacro(OutputVolumeID);
  vtkGetStringMacro(OutputVolumeID);
 
protected:
  vtkSlicerPlastimatchLogic();
  virtual ~vtkSlicerPlastimatchLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

private:
  vtkSlicerPlastimatchLogic(const vtkSlicerPlastimatchLogic&); // Not implemented
  void operator=(const vtkSlicerPlastimatchLogic&);              // Not implemented
  void SetLandmarksFromSlicer();
  void SetLandmarksFromFiles();
  void ApplyInitialLinearTransformation();
  void ApplyWarp(
    Plm_image* warpedImage,   /* Output: Output image */
    DeformationFieldType::Pointer* vectorFieldOut, /* Output: Output vf (optional) */
    Xform* inputTransformation,          /* Input:  Input image warped by this xform */
    Plm_image* fixedImage,   /* Input:  Size of output image */
    Plm_image* inputImage,       /* Input:  Input image */
    float defaultValue,     /* Input:  Value for pixels without match */
    int useItk,           /* Input:  Force use of itk (1) or not (0) */
    int interpolationLinear);
  void GetOutputImage();
  void WarpLandmarks();

private:
  char* FixedID;
  char* MovingID;
  vtkPoints* FixedLandmarks;
  vtkPoints* MovingLandmarks;
  char* FixedLandmarksFileName;
  char* MovingLandmarksFileName;
  vtkPoints* WarpedLandmarks;
  Registration_parms* RegistrationParameters;
  Registration_data* RegistrationData;
  char* InputTransformationID;
  Xform* InputTransformation;
  Xform* OutputTransformation;
  Plm_image* WarpedImage;
  char* OutputVolumeID;
};

#endif
