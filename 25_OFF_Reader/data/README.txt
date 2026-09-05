* This folder contains surface meshes for three proteins at multiple resolutions.
* The three proteins are 1GRM, 2OAR and 3SY7. 
* The surface meshes are stored in OFF file format at different levels of detail. 
* <protein>_1.off is the high resolution mesh with maximum number of triangles. 
* <protein>_2.off ... <protein>_16.off have the mesh at lower resolutions, with <protein>_i.off having (1/i) times the number of triangles compared to <protein>_1.off

* OFFReader.c can be used to parse OFF files. You can modify it and add it to your project as needed. Note that you HAVE to add new code to compute normals.

* Note: OFF is very simple file format for saving polygon meshes.
  first line : OFF
  second line : #number_of_vertices #number_of_polygons 0
  next #number_of_vertices lines have vertex info : x y z coordinates of the vertex i
  next #number_of_polygons lines have polygon info : #sides v[1] v[2] ... v[#sides] (For a triangle: 3 v1 v2 v3), v[i] is an index into vertex list. 
