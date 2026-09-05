#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ogldev_math_3d.h"

typedef struct Vt {
	float x,y,z;
}Vertex;

typedef struct Pgn {
	int noSides;
	int *v;
}Polygon;

typedef struct offmodel {
	Vertex *vertices;
	Polygon *polygons;
	int numberOfVertices;
 	int numberOfPolygons;
}OffModel;

OffModel* readOffFile(char * OffFile) {
	FILE * input;
	char type[3]; 
	int noEdges;
	int i,j;
	float x,y,z;
	int n, v;
	int nv, np;
	OffModel *model;
	input = fopen(OffFile, "r");
	fscanf(input, "%s", type);
	/* First line should be OFF */
	if(strcmp(type,"OFF")) {
		printf("Not a OFF file");
		exit(1);
	}
	/* Read the no. of vertices, faces and edges */
	fscanf(input, "%d", &nv);
	fscanf(input, "%d", &np);
	fscanf(input, "%d", &noEdges);

	model = (OffModel*)malloc(sizeof(OffModel));
	model->numberOfVertices = nv;
	model->numberOfPolygons = np;
	
	
	/* allocate required data */
	model->vertices = (Vertex*) malloc(nv * sizeof(Vertex));
	model->polygons = (Polygon*) malloc(np * sizeof(Polygon));
	

	/* Read the vertices' location*/	
	for(i = 0;i < nv;i ++) {
		fscanf(input, "%f %f %f", &x,&y,&z);
		(model->vertices[i]).x = x;
		(model->vertices[i]).y = y;
		(model->vertices[i]).z = z;
	}

	/* Read the Polygons */	
	for(i = 0;i < np;i ++) {
		/* No. of sides of the polygon (Eg. 3 => a triangle) */
		fscanf(input, "%d", &n);
		
		(model->polygons[i]).noSides = n;
		(model->polygons[i]).v = (int *) malloc(n * sizeof(int));
		/* read the vertices that make up the polygon */
		for(j = 0;j < n;j ++) {
			fscanf(input, "%d", &v);
			(model->polygons[i]).v[j] = v;
		}
	}

	fclose(input);
	return model;
}

void printOffModel(OffModel* model) {
	int i, j;

	printf("OFF\n");
	printf("%d %d 0 \n", model->numberOfVertices, model->numberOfPolygons);

	// for(i = 0; i < model->numberOfVertices;i ++) {
		// printf("%f %f %f \n", (model->vertices[i]).x, (model->vertices[i]).y, (model->vertices[i]).z);
	// }

	// for(i = 0;i < model->numberOfPolygons;i ++) {
	// 	printf("%d ", (model->polygons[i]).noSides);
	// 	for(j = 0;j < (model->polygons[i]).noSides;j ++) {
	// 		printf("%d ", (model->polygons[i]).v[j]);
	// 	}
	// 	printf("\n");
	// }

	// for(i = 0;i < 3;i ++) {
	// 	printf("%d ", (model->polygons[i]).noSides);
	// 	for(j = 0;j < (model->polygons[i]).noSides;j ++) {
	// 		printf("%d ", (model->polygons[i]).v[j]);
	// 	}
	// 	printf("\n");
	// }
}

int FreeOffModel(OffModel* model) {
	int i,j;
	if(model == NULL){
		return 0;
	}
	free(model->vertices);
	for(i = 0; i < model->numberOfPolygons; ++i ){
		if((model->polygons[i]).v){
			free((model->polygons[i]).v);
		}
	}
	free(model->polygons);
	free(model);
	return 1;
}

int getNumVertices(OffModel* model){
    int numVertices = model->numberOfVertices;

    return numVertices;
}

int getNumTriangles(OffModel* model){
    int numTriangles = model->numberOfPolygons;

    return numTriangles;
}

Vector3f* getVertices(OffModel* model){

	int numVertices = getNumVertices(model);

    Vector3f* vertexArray = (Vector3f*)malloc(numVertices * sizeof(Vector3f));

    if (vertexArray == NULL) {
        printf("Memory allocation failed!\n");
        exit(1); // Exit the program if allocation fails
    }

    for (int i = 0; i < numVertices; i++) {
        vertexArray[i] = Vector3f((model->vertices[i]).x, (model->vertices[i]).y, (model->vertices[i]).z);
    }

    // Return the pointer to the allocated array
    return vertexArray;
}

unsigned int* getIndices(OffModel* model){
	
	int numTriangles = getNumTriangles(model);
	int numIndices = 3 * numTriangles;

    unsigned int* indexArray = (unsigned int*)malloc(numIndices * sizeof(unsigned int));

    if (indexArray == NULL) {
        printf("Memory allocation failed!\n");
        exit(1); // Exit the program if allocation fails
    }

	int index = 0;

	for (int triangle = 0; triangle < numTriangles; ++triangle) {
        if (model->polygons[triangle].noSides != 3) {
            printf("%d: Expected every polygon to have 3 vertices\n",index);
            free(indexArray);
            exit(1);
        }

        indexArray[index++] = model->polygons[triangle].v[0];
        indexArray[index++] = model->polygons[triangle].v[1];
        indexArray[index++] = model->polygons[triangle].v[2];
    }

    return indexArray;
}