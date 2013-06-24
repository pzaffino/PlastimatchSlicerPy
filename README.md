This Slicer module allows to use Plastimatch (www.plastimatch.org) as a Python module from the embedded shell in Slicer (www.slicer.org).
This folder has to be copied into the root directory of source code of the SlicerRT module (www.slicerrt.org).
In order to enable the build of this module is needed edit the main CMakeList.txt file of the SlicerRT project.
In particular in the section where are listed all the subdirectories to be included (more or less around the row #71) has to be added this line:
add_subdirectory(Plastimatch)

For further information please email to: p dot zaffino at unicz dot it
