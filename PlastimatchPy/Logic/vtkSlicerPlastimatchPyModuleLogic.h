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

// .NAME vtkSlicerPlastimatchPyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes

#ifndef __vtkSlicerPlastimatchPyModuleLogic_h
#define __vtkSlicerPlastimatchPyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// PlastimatchPy Module Logic
#include "vtkSlicerPlastimatchPyModuleLogicExport.h"

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

/// Class to wrap Plastimatch registration capability into the embedded Python shell in Slicer
class VTK_SLICER_PLASTIMATCHPY_MODULE_LOGIC_EXPORT vtkSlicerPlastimatchPyModuleLogic :
  public vtkSlicerModuleLogic
{
  typedef itk::Vector< float, 3 >  VectorType;
  typedef itk::Image< VectorType, 3 >  DeformationFieldType;

public:
  /// Constructor
  static vtkSlicerPlastimatchPyModuleLogic* New();
  vtkTypeMacro(vtkSlicerPlastimatchPyModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  /// Add a registration stage in the Plastimatch workflow
  void AddStage();

  /// Set parameter in stage
  void SetPar(char* key, char* value);

  /// Execute registration
  void RunRegistration();

  /// This function warps the landmarks according to OutputTransformation
  void WarpLandmarks();

public:
  /// Set the ID of the fixed image (\sa FixedImageID) (image data type must be "float").
  vtkSetStringMacro(FixedImageID);
  /// Get the ID of the fixed image (\sa FixedImageID) (image data type must be "float").
  vtkGetStringMacro(FixedImageID);

  /// Set the ID of the moving image (\sa MovingImageID) (image data type must be "float").
  vtkSetStringMacro(MovingImageID);
  /// Get the ID of the moving image (\sa MovingImageID) (image data type must be "float").
  vtkGetStringMacro(MovingImageID);
  
  /// Set the fcsv file name (\sa FixedLandmarksFileName) containing the fixed landmarks.
  vtkSetStringMacro(FixedLandmarksFileName);
  /// Get the fcsv file name (\sa FixedLandmarksFileName) containing the fixed landmarks.
  vtkGetStringMacro(FixedLandmarksFileName);

  /// Set the fcsv file name (\sa MovingLandmarksFileName) containing the moving landmarks.
  vtkSetStringMacro(MovingLandmarksFileName);
  /// Get the fcsv file name (\sa MovingLandmarksFileName) containing the moving landmarks.
  vtkGetStringMacro(MovingLandmarksFileName);

  /// Set the ID of a precomputed rigid/affine transformation (\sa InputTransformationID).
  vtkSetStringMacro(InitializationLinearTransformationID);
  /// Get the ID of a precomputed rigid/affine transformation (\sa InputTransformationID).
  vtkGetStringMacro(InitializationLinearTransformationID);

  /// Set the ID of the output image (\sa OutputVolumeID).
  vtkSetStringMacro(OutputVolumeID);
  /// Get the ID of the output image (\sa OutputVolumeID).
  vtkGetStringMacro(OutputVolumeID);

  /// Set the fixed landmarks (\sa FixedLandmarks) using a vtkPoints object.
  vtkSetObjectMacro(FixedLandmarks, vtkPoints);
  /// Get the fixed landmarks (\sa FixedLandmarks) using a vtkPoints object.
  vtkGetObjectMacro(FixedLandmarks, vtkPoints);

  /// Set the moving landmarks (\sa MovingLandmarks) using a vtkPoints object.
  vtkSetObjectMacro(MovingLandmarks, vtkPoints);
  /// Get the moving landmarks (\sa MovingLandmarks) using a vtkPoints object.
  vtkGetObjectMacro(MovingLandmarks, vtkPoints);

  /// Set the warped landmarks (\sa WarpedLandmarks) using a vtkPoints object.
  vtkSetObjectMacro(WarpedLandmarks, vtkPoints);
  /// Get the warped landmarks (\sa WarpedLandmarks) using a vtkPoints object.
  vtkGetObjectMacro(WarpedLandmarks, vtkPoints);

protected:
  /// This function sets the vtkPoints as input landmarks for Plastimatch registration
  void SetLandmarksFromSlicer();

  /// This function reads the fcsv files containing the landmarks and sets them as input landmarks for Plastimatch registration
  void SetLandmarksFromFiles();

  /// This function applies an initial affine trasformation modifing the moving image before the Plastimatch registration
  void ApplyInitialLinearTransformation();

  /// This function applies a linear/deformable transformation at an image.
  /// It is used from ApplyInitialLinearTransformation() and RunRegistration().
  void ApplyWarp(
    Plm_image* warpedImage,                        /*!< Output image as Plm_image pointer */
    DeformationFieldType::Pointer vectorFieldFromTransformation, /*!< Output vector field (optional) as DeformationFieldType::Pointer */
    Xform* inputTransformation,                    /*!< Input transformation as Xform pointer */
    Plm_image* fixedImage,                         /*!< Fixed image as Plm_image pointer */
    Plm_image* imageToWarp,                         /*!< Input image to warp as Plm_image pointer */
    float defaultValue,                            /*!< Value (float) for pixels without match */
    int useItk,                                    /*!< Int to choose between itk (1) or Plastimatch (0) algorithm for the warp task */
    int interpolationLinear                        /*!< Int to choose between trilinear interpolation (1) on nearest neighbor (0) */
    );

  /// This function shows the deformed image into the Slicer scene
  void SetWarpedImageInVolumeNode(Plm_image* warpedPlastimatchImage);

protected:
  vtkSlicerPlastimatchPyModuleLogic();
  virtual ~vtkSlicerPlastimatchPyModuleLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  virtual void UpdateFromMRMLScene();

protected:
  /// ID of the fixed image
  /// This value is a required parameter to execute a registration.
  char* FixedImageID;

  /// ID of the moving image
  /// This value is a required parameter to execute a registration.
  char* MovingImageID;

  /// Name of the fcsv containing the fixed landmarks
  /// The number of the fixed landmarks must be the same of the number of the moving landmarks.
  /// Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  /// Is not possible mix landmarks from vtkPoints and from files.
  /// If no vtkPoints objects are passing this value is a required parameter to execute a landmark based registration.
  char* FixedLandmarksFileName;
  
  /// Name of the fcsv containing the moving landmarks
  /// The number of the moving landmarks must be the same of the number of the fixed landmarks.
  /// Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  /// Is not possible mix landmarks from vtkPoints and from files.
  /// If no vtkPoints objects are passing this value is a required parameter to execute a landmark based registration.
  char* MovingLandmarksFileName;
  
  /// ID of the affine registration used as initialization for the Plastimatch registration
  /// This transformation will be used as initialization for the Plastimatch registration.
  /// This value is an optional parameter to execute a registration.
  char* InitializationLinearTransformationID;

  /// ID of the registered image
  /// This value is a required parameter to execute a registration.
  char* OutputVolumeID;

  /// vtkPoints object containing the fixed landmarks
  /// The number of the fixed landmarks must be the same of the number of the moving landmarks.
  /// Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  /// Is not possible mix landmarks from vtkPoints and from files.
  /// If no fcsv files are passing this value is a required parameter to execute a landmark based registration.
  vtkPoints* FixedLandmarks;

  /// vtkPoints object containing the moving landmarks
  /// The number of the moving landmarks must be the same of the number of fixed landmarks.
  /// Landmarks passing as vtkPoints have the priority over landmarks passing by files.
  /// Is not possible mix landmarks from vtkPoints and from files.
  /// If no fcsv files are passing this value is a required parameter to execute a landmark based registration.
  vtkPoints* MovingLandmarks;

  /// vtkPoints object containing the warped landmarks
  /// This value is a required parameter to execute a landmark based registration.
  vtkPoints* WarpedLandmarks;
  
  /// Plastimatch registration parameters
  Registration_parms* RegistrationParameters;

  /// Plastimatch registration data
  Registration_data* RegistrationData;

  /// Vector filed computed by Plastimatch
  DeformationFieldType::Pointer MovingImageToFixedImageVectorField;
  
private:
  vtkSlicerPlastimatchPyModuleLogic(const vtkSlicerPlastimatchPyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerPlastimatchPyModuleLogic&);            // Not implemented
};

#endif
