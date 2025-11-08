



#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mylcd.h>
#include <demos.h>
#include "split.h"
#include "polyfile.h"



// source .mp map
#define MAP_SOURCE				((uint8_t*)"M:\\RamdiskTemp\\map\\leisure_gmapsupp.mp")

// destination poly location
#define POLY_PATH				((uint8_t*)"M:\\polys\\250")


/*

Maps are by FSK derived from: https://freizeitkarte-osm.de/garmin/en/westeuropa.html

Converting from .mp to .poly's can generate 500k+ files. Use a RamDisk if you care about your SSD.
SoftPerfect RAM Disk (64-bit) is my preference.

Use: https://extract.bbbike.org/?lang=en
Create a coverage Region bounding box: Record those numbers somewhere
Select format 'Garmin Leisure (UTF8-8)'
Add a map name.
Add an Email to later receive the download link when processed
...
Download and Extract .img file
Import .img in to GPSMapEdit: https://www.geopainting.com 
Add Map Skin 6372.typ, then select for viewing
Save as 'Polish Format (.mp)'

*/



// Define your region. Keep it small at first for testing
#if 0
static const mp_coverage_t coverage = {		//  Rectangle of MAP_SOURCE coverage
		{{55.25573, -8.188934}, {54.022522, -5.413513}},
		2.775421/*2.77542*/,	// ° width
		1.233208/*1.23322*/		// ° height
};
#else
static const mp_coverage_t coverage = {
		{{55.3985791, -8.8708173},
		 {53.9454972, -5.3821777}},
		3.4886396,		// width
		1.4530819		// height
};
#endif



#define  aspectCorrection  (1.0 / ((double)VWIDTH/(double)VHEIGHT))
const float aspectOffset = (((VHEIGHT*0.5) * (1.0 - aspectCorrection)) * ((double)VWIDTH/(double)VHEIGHT));



static inline void location2Block (const vectorPt2_t *loc, int32_t *x_lon, int32_t *y_lat)
{
	*y_lat = (fabs(coverage.region.v1.lat) - fabs(loc->lat)) / GPS_LENGTH_LAT;
	*x_lon = (fabs(coverage.region.v1.lon) - fabs(loc->lon)) / GPS_LENGTH_LON;
}

static inline void location2Blockf (const vectorPt2_t *loc, double *x_lon, double *y_lat)
{
	*y_lat = (fabs(coverage.region.v1.lat) - fabs(loc->lat)) / GPS_LENGTH_LAT;
	*x_lon = (fabs(coverage.region.v1.lon) - fabs(loc->lon)) / GPS_LENGTH_LON;
}

static inline void block2Location (const int32_t x_lon, const int32_t y_lat, vectorPt2_t *loc)
{
	loc->lat = fabsf(coverage.region.v1.lat) - (y_lat * GPS_LENGTH_LAT);
	loc->lon = (x_lon * GPS_LENGTH_LON) - fabsf(coverage.region.v1.lon);
}

static inline void block2Locationf (const double x_lon, const double y_lat, vectorPt2_t *loc)
{
	loc->lat = fabs(coverage.region.v1.lat) - (y_lat * GPS_LENGTH_LAT);
	loc->lon = (x_lon * GPS_LENGTH_LON) - fabs(coverage.region.v1.lon);
}

static inline void block2LocationCentered (const double x_lon, const double y_lat, vectorPt2_t *loc)
{
	block2Location(x_lon, y_lat, loc);
	loc->lat -= (fabs(GPS_LENGTH_LAT)/2.0);
	loc->lon += (fabs(GPS_LENGTH_LON)/2.0);
}

static inline void moveGPSWindowPt2 (vectorPt4_t *in, vectorPt2_t *by)
{
	in->v1.lon += by->lon;
	in->v2.lon += by->lon;
	in->v1.lat += by->lat;
	in->v2.lat += by->lat;
}

static inline void moveGPSWindow (vectorPt4_t *in, const double lat, const double lon)
{
	in->v1.lon += lon;
	in->v2.lon += lon;
	in->v1.lat += lat;
	in->v2.lat += lat;
}

static inline void makeGPSWindow (const vectorPt2_t *center, const double spanMeters, vectorPt4_t *out)
{
	double m = (spanMeters/1000.0);
	out->v1.lon = center->lon - (m * GPS_1000M_LON);		// top left
	out->v1.lat = center->lat + (m * GPS_1000M_LAT);
	
	out->v2.lon = center->lon + (m * GPS_1000M_LON);		// bottom right
	out->v2.lat = center->lat - (m * GPS_1000M_LAT);
}

static inline void makeBlockWindow (const int32_t x_lon, const int32_t y_lat, vectorPt4_t *out)
{
	vectorPt2_t loc;
	block2Location(x_lon, y_lat, &loc);
	
	out->v1.lon = loc.lon;						// top left
	out->v1.lat = loc.lat;
	out->v2.lon = loc.lon + GPS_LENGTH_LON;		// bottom right
	out->v2.lat = loc.lat - GPS_LENGTH_LAT;
}

static inline double measureDistance (vector2_t *v1, vector2_t *v2)
{
	#define earthsRadius (6378137.0)
	#define pi80 (M_PI / 180.0)
		
	double lat1 = v1->y * pi80;
	double lon1 = v1->x * pi80;
	double lat2 = v2->y * pi80;
	double lon2 = v2->x * pi80;

	double dlat = fabs(lat2 - lat1);
	double dlon = fabs(lon2 - lon1);
	double a = sin(dlat / 2.0) * sin(dlat / 2.0) + cos(lat1) * cos(lat2) * sin(dlon /2.0) * sin(dlon / 2.0);
	double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
	double d = earthsRadius * c;

	return d;
}

static inline int countVectors (uint8_t *string)
{
	int count = 0;
	while(*string){
		count += (*string == ')');		// one ) per vector pair
		string++;
	}
	return count;
}

static inline vectors_t *vectorsAlloc (const uint32_t total)
{
	vectors_t *vectors = malloc(sizeof(vectors_t));
	if (vectors){
		vectors->total = total;
		vectors->list = malloc(total * sizeof(vector2_t));
	}

	return vectors;
}

static inline void vectorsFree (vectors_t *vectors)
{
	if (vectors->total) free(vectors->list);
	free(vectors);
}

static inline polyline_t *polylineAlloc (const uint32_t vectorSpace)
{
	polyline_t *polyline = calloc(1, sizeof(polyline_t));
	if (vectorSpace && polyline){
		polyline->vectors = vectorsAlloc(vectorSpace);
		if (!polyline->vectors){
			free(polyline);
			return NULL;
		}
	}
	return polyline;
}

static inline vectorNode_t *vectorNodeListAlloc (nodes_t *nodes)
{
	nodes->list = calloc(1, sizeof(vectorNode_t));
	return nodes->list;
}

static inline vectorNode_t *vectorNodeListResize (nodes_t *nodes, const int newSize)
{
	nodes->list = realloc(nodes->list, newSize * sizeof(vectorNode_t));
	return nodes->list;
}

static inline void vectorNodeListFree (nodes_t *nodes)
{
	nodes->total = 0;
	free(nodes->list);
}

static inline void polylineFree (polyline_t *polyline)
{
	if (polyline->vectors)
		vectorsFree(polyline->vectors);
		
	if (polyline->nodes.total)
		vectorNodeListFree(&polyline->nodes);
	
	free(polyline);
}

static inline polylines_t *polylinesAlloc (const uint32_t initalSize, const uint32_t increaseBy)
{
	polylines_t *polylines = calloc(1, sizeof(polylines_t));
	if (!polylines) return NULL;
	
	polylines->total = 0;
	polylines->incBy = increaseBy;
	polylines->size = initalSize;
	polylines->list = malloc(initalSize * sizeof(polyline_t*));
	if (!polylines->list){
		free(polylines);
		return NULL;
	}
	
	return polylines;
}

static inline polylines_t *polylinesResize (polylines_t *polylines)
{
	polylines->size += polylines->incBy;
	polylines->list = realloc(polylines->list, polylines->size * sizeof(polyline_t*));
	if (!polylines->list) return NULL;
	
	return polylines;
}

static inline void polylinesFree (polylines_t *polylines)
{
	for (int i = 0; i < polylines->total; i++)
		polylineFree(polylines->list[i]);
		
	free(polylines->list);
	free(polylines);
}

static inline int addPolyline (polylines_t *polylines, polyline_t *polyline)
{
	if (polylines->total == polylines->size){
		if (!polylinesResize(polylines))
			return 0;
	}
	
	polylines->list[polylines->total] = polyline;
	return ++polylines->total;
}

static inline int addNode (polyline_t *polyline, vectorNode_t *node)
{
	if (!polyline->nodes.total){
		if (!vectorNodeListAlloc(&polyline->nodes))
			return -1;
	}else{
		if (!vectorNodeListResize(&polyline->nodes, polyline->nodes.total+1))
			return -2;
	}
		
	polyline->nodes.list[polyline->nodes.total].index = node->index;
	polyline->nodes.list[polyline->nodes.total].id = node->id;
	
	return ++polyline->nodes.total;
}

static inline int isLinePolyline (const uint8_t *string) 
{
	return !strncmp((char*)string, "[POLYLINE]", 10);
}

static inline int isLinePolygon (const uint8_t *string) 
{
	return !strncmp((char*)string, "[POLYGON]", 9);
}

static inline int isLineEnd (const uint8_t *string) 
{
	return !strncmp((char*)string, "[END]", 5);
}

static inline int isLineNode (const uint8_t *string) 
{
	return !strncmp((char*)string, "Nod", 3);
}

static inline int isLineType (const uint8_t *string) 
{
	return !strncmp((char*)string, "Type=", 5);
}

static inline int isLineVectorString (const uint8_t *string) 
{
	if (!strncmp((char*)string, "Data", 4)){
		if (string[4] == '0' /*|| string[4] == '1' || string[4] == '2' || string[4] == '3'*/){
			return (string[5] == '=');
		}
	}
	return 0;
}

static inline int readType (const uint8_t *string, uint32_t *type)
{
	return sscanf((char*)string, "Type=%i", type) == 1;
}

static inline int readNode (const uint8_t *string, int32_t *nodeNo, vectorNode_t *node)
{
	//*nodeNo = -1;
	return sscanf((char*)string, "Nod%i=%i,%i", nodeNo, &node->index, &node->id) == 3;
}

static inline int readNode2 (const uint8_t *string, int32_t *nodeNo, vectorNode_t *node)
{
	//*nodeNo = -1;
	return sscanf((char*)string, "Nodes%i=(%i,%i", nodeNo, &node->index, &node->id) == 3;
}

static inline int readVector (const uint8_t *string, vector2_t *vec)
{
	return sscanf((char*)string, "(%f,%f)", &vec->y, &vec->x) == 2;
}

static inline uint8_t *nextVector (const uint8_t *string)
{
	char *next = strstr((char*)string, "),(");
	if (next) return (uint8_t*)(next+2);

	return NULL;
}

static inline uint8_t *firstVector (uint8_t *string)
{
	char *first = strchr((char*)string, '=');
	if (first) return (uint8_t*)(first+1);
	
	return NULL;
}

static inline vectors_t *readVectors (uint8_t *string)
{
	vectors_t *vectors = NULL;
	
	int vTotal = countVectors(string);
	if (vTotal){
		vectors = vectorsAlloc(vTotal);
		if (vectors){
			for (int v = 0; v < vTotal; v++){
				if (!readVector(string, &vectors->list[v]))
					break;
				string = nextVector(string);
				//if (!string) break;
			}
		}
	}
	return vectors;
}

static inline polyline_t *processPolyline (uint8_t **lines, const int lTotal)
{
	polyline_t *polyline = polylineAlloc(0);
	if (!polyline) return NULL;
	
	//int vIdx = 0;
	
	for (int i = 1; i < lTotal; i++){
		uint8_t *line = lines[i];
		
		if (isLineVectorString(line)){
			//if (vIdx < 2){
				if (!polyline->vectors){
					polyline->vectors = readVectors(firstVector(line));
					if (polyline->vectors){
						//vIdx++;
					}
				}
			//}
		}else if (isLineType(line)){
			readType(line, &polyline->type);
#if 0
		}else if (isLineNode(line)){
			int32_t nodeNo;
			vectorNode_t node;
			if (!readNode(line, &nodeNo, &node)){
				if (!readNode2(line, &nodeNo, &node)){
					printf("readNode Failed #%s#\n", line);
					continue;
				}
			}

			int ret = addNode(polyline, &node);
			if (ret < 0)
				printf("addNode Failed %i\n", ret);
#endif
		}else if (isLineEnd(line)){
			break;
		}
	}
	
	if (!polyline->vectors){
		polylineFree(polyline);
		polyline = NULL;
	}
	
	return polyline;
}

static inline polylines_t *importMP (const uint8_t *path)
{

	TASCIILINE *al = readFileA((char*)path);
	if (!al){
		printf("readFileA() failed\n");
		return NULL;
	}
	
	int vectors = 0;
	polylines_t *polylines = polylinesAlloc(512, 512);
	
	for (int i = 0; i < al->tlines; i++){
		uint8_t *line = al->line[i];
		const int isapolygon = isLinePolygon(line);
		
		if (isapolygon || isLinePolyline(line)){
			polyline_t *polyline = processPolyline(&al->line[i], al->tlines-i);
			if (polyline){
				/*total =*/ addPolyline(polylines, polyline);
				polyline->isPolygon = isapolygon;
				vectors += polyline->vectors->total;
			}
		}
	}

	polylines->vectorTotal = vectors;
	
	//printf("vector total %i %i\n", vectors, total);
	//printf("lines total %i\n", al->tlines);
	freeASCIILINE(al);
	
	return polylines;
}

static inline double vectorDistance (const vector2_t *v1, const vector2_t *v2)
{
	return sqrt((fabs(v1->x - v2->x)*fabs(v1->x - v2->x)) + (fabs(v1->y - v2->y)*fabs(v1->y - v2->y)));
}

static inline nodes_t *getNodes (polyline_t *polyline)
{
	return &polyline->nodes;
}

static inline vectorNode_t *getNode (nodes_t *nodes, const uint32_t idx)
{
	if (idx < nodes->total)
		return &nodes->list[idx];
	return NULL;
}

static inline vectors_t *getVectors (polyline_t *polyline)
{
	return polyline->vectors;
}

static inline vector2_t *getVector (polyline_t *polyline, const uint32_t vIdx)
{
	vectors_t *vectors = getVectors(polyline);
	return &vectors->list[vIdx];
}

static inline vector2_t *getPath (const vectors_t *vectors, const uint32_t vIdx)
{
	return &vectors->list[vIdx];
}

static inline polyline_t *getPolyline (const polylines_t *polylines, const int idx)
{
	return polylines->list[idx];
}

static inline int findVectorId (polyline_t *polyline, const int id)
{
	nodes_t *nodes = getNodes(polyline);
	if (!nodes) return -1;
	
	for (int i = 0; i < nodes->total; i++){
		vectorNode_t *node = getNode(nodes, i);
		if (node->id == id) return i;
	}
	return -1;
}

static inline int isPathInRegion (const vectorPt2_t *vec, const vectorPt4_t *win)
{
	return  (vec->lat <= win->v1.lat && vec->lat >= win->v2.lat) &&
			(vec->lon >= win->v1.lon && vec->lon <= win->v2.lon);
}

 
// Returns x-value of point of intersectipn of two
// lines
static inline float x_intersect (const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x4, const float y4)
{
    float num = (x1*y2 - y1*x2) * (x3-x4) - (x1-x2) * (x3*y4 - y3*x4);
    float den = (x1-x2) * (y3-y4) - (y1-y2) * (x3-x4);
    return num/den;
}
 
// Returns y-value of point of intersectipn of
// two lines
static inline float y_intersect (const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x4, const float y4)
{
    float num = (x1*y2 - y1*x2) * (y3-y4) - (y1-y2) * (x3*y4 - y3*x4);
    float den = (x1-x2) * (y3-y4) - (y1-y2) * (x3-x4);
    return num/den;
}
 
// This functions clips all the edges w.r.t one clip
// edge of clipping area
static inline void suthHodgClip (vector2_t *poly_points, int *poly_size, const float x1, const float y1, const float x2, const float y2, vector2_t *new_points)
{
    int new_poly_size = 0;
 
    // (ix,iy),(kx,ky) are the co-ordinate values of
    // the points
    for (int i = 0; i < *poly_size; i++){
        // i and k form a line in polygon
        int k = (i+1) % *poly_size;
        float ix = poly_points[i].x, iy = poly_points[i].y;
        float kx = poly_points[k].x, ky = poly_points[k].y;
        
       // printf("%i: %.2f %.2f   %.2f %.2f\n", i, ix, iy, kx, ky);
 
        // Calculating position of first point
        // w.r.t. clipper line
        float i_pos = (x2-x1) * (iy-y1) - (y2-y1) * (ix-x1);
 
        // Calculating position of second point
        // w.r.t. clipper line
        float k_pos = (x2-x1) * (ky-y1) - (y2-y1) * (kx-x1);
 
        // Case 1 : When both points are inside
        if (i_pos < 0  && k_pos < 0){
            //Only second point is added
            new_points[new_poly_size].x = kx;
            new_points[new_poly_size].y = ky;
            new_poly_size++;
            
        }else if (i_pos >= 0  && k_pos < 0){	// Case 2: When only first point is outside
            // Point of intersection with edge
            // and the second point is added
            new_points[new_poly_size].x = x_intersect(x1, y1, x2, y2, ix, iy, kx, ky);
            new_points[new_poly_size].y = y_intersect(x1, y1, x2, y2, ix, iy, kx, ky);
            new_poly_size++;
 
            new_points[new_poly_size].x = kx;
            new_points[new_poly_size].y = ky;
            new_poly_size++;
        }else if (i_pos < 0  && k_pos >= 0){    // Case 3: When only second point is outside
            //Only point of intersection with edge is added
            new_points[new_poly_size].x = x_intersect(x1, y1, x2, y2, ix, iy, kx, ky);
            new_points[new_poly_size].y = y_intersect(x1, y1, x2, y2, ix, iy, kx, ky);
            new_poly_size++;

        //}else{	 								 // Case 4: When both points are outside
            //No points are added
           // printf("nothing added %i\n", i);
        }
    }
    *poly_size = new_poly_size;
}
 
// Implements Sutherland–Hodgman algorithm
void vectorsClip (vector2_t *vectors, int *vtotal, const vector2_t *window)
{
	const int vTotalWindow = 4;
	vector2_t workingBuffer[32+(*vtotal*2)];
	
    //i and k are two consecutive indexes
    for (int i = 0; i < vTotalWindow; i++){
        int k = (i+1)%vTotalWindow;
 
        // We pass the current array of vertices, it's size
        // and the end points of the selected clipper line
        suthHodgClip(vectors, vtotal, window[i].x, window[i].y, window[k].x, window[k].y, workingBuffer);
        
		// set new points into original array
		for (int i = 0; i < *vtotal; i++){
			vectors[i].x = workingBuffer[i].x;
			vectors[i].y = workingBuffer[i].y;
		}
    }
}

vectors_t *polygonClip (vectors_t *in, vectorPt4_t *region)
{
	int poly_size = in->total;
	vector2_t poly_points[(poly_size*2)+32];

	
	for (int v = 0; v < poly_size; v++){
		poly_points[v].y = in->list[v].y;
		poly_points[v].x = in->list[v].x;
	}
	
	vector2_t window[4];
	window[0].x = region->v1.lon;
	window[0].y = region->v1.lat;
	
	window[1].x = region->v2.lon;
	window[1].y = region->v1.lat;
	
	window[2].x = region->v2.lon;
	window[2].y = region->v2.lat;
	
	window[3].x = region->v1.lon;
	window[3].y = region->v2.lat;
	
	vectorsClip(poly_points, &poly_size, window);
	
	if (poly_size){
		vectors_t *out = vectorsAlloc(poly_size);
		if (out){
			for (int v = 0; v < poly_size; v++){
				out->list[v].y = poly_points[v].y;
				out->list[v].x = poly_points[v].x;
			}
			return out;
		}
	}
	return NULL;
}

static inline void writeVectors (FILE *file, polyline_t *polyline, const vectors_t *vectors, const uint32_t from, const uint32_t to)
{
	const int32_t vtotal = (to - from) + 1;
	uint16_t type = polyline->type;
	if (polyline->isPolygon) type |= 0x8000;

	poly_field_t field = {type, vtotal};
	fwrite(&field, sizeof(field), 1, file);
	fwrite(&vectors->list[from], sizeof(vector2_t), vtotal, file);
}

static inline uint32_t doSplit (FILE *file, polylines_t *polylines, vectorPt4_t * const region)
{
	uint32_t count = 0;
	
	for (int p = 0; p < polylines->total; p++){
		polyline_t *polyline = getPolyline(polylines, p);
		
		if (!polyline->isPolygon){
			if (polyline->type == 0x1C || polyline->type == 0x1D || polyline->type == 0x1E 
				|| polyline->type&0x10000 || polyline->type == 0x15 /*sea to land border */ 
				|| polyline->type == 0x28 || polyline->type == 0x29 || polyline->type == 0x14
				|| polyline->type == 0x19 || polyline->type == 0x1A || polyline->type == 0x1B
			  	|| polyline->type == 0x1F
				){
				continue;
			}
		}else{
			if (/*polyline->type == 0x27 ||*/ polyline->type == 0x0E || polyline->type == 0x4A || polyline->type == 0x4B
			  	|| polyline->type&0x10000
				){
				continue;
			}
			
			vectors_t *out = polygonClip(polyline->vectors, region);
			if (out){
				if (out->total >= 3)
					writeVectors(file, polyline, out, 0, out->total-1);
				vectorsFree(out);
			}
			continue;
		}
		
		const vectors_t *vectors = getVectors(polyline);
		if (!vectors) continue;		

		int writeThis = 0;
		uint32_t vtotal = vectors->total;
		
		// from index 'from' to 'to'
		int from = 0;
		int to = vtotal-1;
		
		for (int v = 0; v < vectors->total; v++){
			vectorPt2_t *vec = (vectorPt2_t*)getPath(vectors, v);
			if (isPathInRegion(vec, region)){
				writeThis = 1;
				//break;
				
			}else if (writeThis){
				to = v;
				writeVectors(file, polyline, vectors, from, to);
				count++;

				writeThis = 0;
				from = v;
				to = vtotal-1;
				
			}else if (!writeThis){
				from = v;
			}
		}
		
		if (writeThis && ((to-from)+1 > 1)){
			writeVectors(file, polyline, vectors, from, to);
			count++;
		}
	}
	return count;
}

static inline uint32_t splitBlock (polylines_t *polylines, const uint8_t *dir, const int32_t x_lon, const int32_t y_lat)
{
	vectorPt4_t region;
	makeBlockWindow(x_lon, y_lat, &region);

	char filename[64];
	snprintf(filename, sizeof(filename), "%s\\%03i_%03i.poly", dir, y_lat, x_lon);
	
	uint32_t polyCount = 0;
	FILE *file = fopen(filename, "w+b");
	if (file){
		polyCount = doSplit(file, polylines, &region);
		fclose(file);
	}

	return polyCount;
}

static inline int polylineSearch (polylines_t *polylines, polyline_t *polylineB)
{
	const vector2_t *vectorB0 = getVector(polylineB, 0);
	const vector2_t *vectorB1 = getVector(polylineB, 1);
	const vector2_t *vectorB2 = getVector(polylineB, 2);
	const int totalB = polylineB->vectors->total;
		
	for (int j = 0; j < polylines->total; j++){
		polyline_t *polylineA = getPolyline(polylines, j);
		if (polylineA->vectors->total != totalB) continue;
		
		vector2_t *vectorA = getVector(polylineA, 0);
		if (vectorA && (vectorA->x == vectorB0->x) && (vectorA->y == vectorB0->y)){
			vectorA = getVector(polylineA, 1);
			if (vectorA && (vectorA->x == vectorB1->x) && (vectorA->y == vectorB1->y)){
				vectorA = getVector(polylineA, 2);
				if (vectorA && vectorA->x == vectorB2->x && vectorA->y == vectorB2->y)
					return 1;
			}
		}
	}

	return 0;
}

static inline int loadBlock (const uint8_t *dir, polylines_t *polylines, const int32_t x_lon, const int32_t y_lat)
{
	char filename[260];
	snprintf(filename, sizeof(filename)-1, "%s\\%03i_%03i.poly", dir, y_lat, x_lon);
	
	int32_t polylinesAdded = 0;
	
	FILE *file = fopen(filename, "r+b");
	if (!file){
		return -1;
	}else{
		while (1){		
			poly_field_t field;
			int ret = fread(&field, sizeof(field), 1, file);
			if (ret != 1){
				//printf("field not read: %i %i (#%s# ret = %i) %i\n", y_lat, x_lon, filename, ret, polylinesAdded);
				break;
			}
		
			polyline_t *polyline = polylineAlloc(field.total);
			if (polyline){
				polyline->type = field.type;
			
				if (fread(polyline->vectors->list, sizeof(vector2_t), field.total, file) == field.total){
					int exists = 0;//polylineSearch(polylines, polyline);
					if (!exists){
						if (addPolyline(polylines, polyline)){
							polylines->vectorTotal += polyline->vectors->total;
							polylinesAdded++;
						}else{
							//printf("polyline not added %i %i\n", y_lat, x_lon);
							break;
						}
					}else{
						polylineFree(polyline);
						//printf("poly exists %i %i\n", y_lat, x_lon);
						//break;
					}
				}else{
					polylineFree(polyline);
					//printf("readFailed %i %i\n", y_lat, x_lon);
					break;
				}
			}
		};

		//printf("polyline total %i\n", polylines->total);
		//printf("polylines->vectorTotal %i\n", polylines->vectorTotal);
		fclose(file);
	}
	return polylinesAdded;
}

uint32_t generatePolyBlocks (polylines_t *const polylines, const mp_coverage_t *coverage, const uint8_t *destDir, const int quadBlock)
{

	char *areaName[] = {"unasigned", "Top left", "Bottom left", "Top right", "Bottom right"};
	
	uint32_t totalPaths = 0;
	int blockTotalLon = ceilf(coverage->width / GPS_LENGTH_LON);
	int blockTotalLat = ceilf(coverage->height / GPS_LENGTH_LAT);
	// keep quads evenly sized
#if 1
	blockTotalLon += (blockTotalLon&0x01);
	blockTotalLat += (blockTotalLat&0x01);
#endif
	
	const int quadtotal = (blockTotalLon>>1) * (blockTotalLat>>1);
	
	printf("Map coverage: (Lon, Lat)\n");
	printf("\t%f %f\n", coverage->region.v1.lon, coverage->region.v1.lat);
	printf("\t%f %f\n", coverage->region.v2.lon, coverage->region.v2.lat);
	
	printf("Blocks:\n");
	printf("\t%i across by %i down\n", blockTotalLon, blockTotalLat);
	printf("\t%i total\n", blockTotalLon * blockTotalLat);

	printf("Processing region:\n");
	printf("\t%s quadrant\n", areaName[quadBlock]);
	printf("\t%i x %i = %i\n", blockTotalLon>>1, blockTotalLat>>1, quadtotal);
	printf("\n... ");
	
	
	if (quadBlock == 0){			// render a subregion from the predefine coverage area. Useful for testing.
		int32_t x_lon1, y_lat1;
		int32_t x_lon2, y_lat2;

		const vectorPt2_t loc1 = {54.656217, -6.017136};
		const vectorPt2_t loc2 = {54.546214, -5.788685};

		location2Block(&loc1, &x_lon1, &y_lat1);
		location2Block(&loc2, &x_lon2, &y_lat2);
		
		printf("\nBox coverage: %i,%i -> %i,%i\n", x_lon1, y_lat1, x_lon2, y_lat2);
		
		for (int y = y_lat1; y <= y_lat2; y++){
			for (int x = x_lon1; x <= x_lon2; x++)
				totalPaths += splitBlock(polylines, destDir, x, y);

			printf("\r Row:%i, Paths:%i ", y, totalPaths);
		}
	}else if (quadBlock == 1){
		int row = 1;
		
		for (int y = 0; y < blockTotalLat>>1; y++){
			for (int x = 0; x < blockTotalLon>>1; x++){
				totalPaths += splitBlock(polylines, destDir, x, y);
			}
			int n = row * (blockTotalLon>>1);
			printf("\r %i, %i, %.1f%% ", row, totalPaths, (100.0f / quadtotal) * n);
			row++;
		}
	}else if (quadBlock == 2){
		int row = 1;
		
		for (int y = blockTotalLat>>1; y < blockTotalLat; y++){
			for (int x = 0; x < blockTotalLon>>1; x++){
				totalPaths += splitBlock(polylines, destDir, x, y);
			}
			int n = row * (blockTotalLon>>1);
			printf("\r %i, %i, %.1f%% ", row, totalPaths, (100.0f / quadtotal) * n);
			row++;
		}
	}else if (quadBlock == 3){
		int row = 1;
		
		for (int y = 0; y < blockTotalLat>>1; y++){
			for (int x = blockTotalLon>>1; x < blockTotalLon; x++){
				totalPaths += splitBlock(polylines, destDir, x, y);
			}
			int n = row * (blockTotalLon>>1);
			printf("\r %i, %i, %.1f%% ", row, totalPaths, (100.0f / quadtotal) * n);
			row++;
		}
	}else if (quadBlock == 4){
		int row = 1;
		
		for (int y = blockTotalLat>>1; y < blockTotalLat; y++){
			for (int x = blockTotalLon>>1; x < blockTotalLon; x++){
				totalPaths += splitBlock(polylines, destDir, x, y);
			}

			int n = row * (blockTotalLon>>1);
			printf("\r %i, %i, %.1f%% ", row, totalPaths, (100.0f / quadtotal) * n);
			row++;
		}
	}else{
		printf("invalid block specified: %i\n", quadBlock);
	}

	printf("\n");

	polylinesFree(polylines);
	return totalPaths;
}

static inline polylines_t *importPolys (const uint8_t *dir, const mp_coverage_t *coverage)
{
	polylines_t *polylines = polylinesAlloc(512, 256);
	if (!polylines) return NULL;

	const int blockTotalLon = ceilf(coverage->width / GPS_LENGTH_LON);
	const int blockTotalLat = ceilf(coverage->height / GPS_LENGTH_LAT);
	//printf("blocks: %ix%i (%i)\n", blockTotalLat, blockTotalLon, blockTotalLat*blockTotalLon);
	
	int blocksLoaded = 0;
	int blocksFailed = 0;
	int blocksNoPolys = 0;
	
	for (int y = 0; y < blockTotalLat; y++){
		for (int x = 0; x < blockTotalLon; x++){
			int ret = loadBlock(dir, polylines, x, y);
			if (ret >= 1){
				blocksLoaded++;
			}else if (ret == -1){
				//printf("blockLoad failed: %i %i\n", y, x);
				blocksFailed++;
				
			}else{
				blocksNoPolys++;
				//printf("blocksNoPolys: %i %i\n", y, x);
				
			}
		}
	}

	printf("blocksLoaded:  %i\n", blocksLoaded);
	printf("blocksFailed:  %i\n", blocksFailed);
	printf("blocksNoPolys: %i\n", blocksNoPolys);
	
	return polylines;
}

static inline float dot (const float x1, const float y1, const float x2, const float y2)
{
    return x1 * x2 + y1 * y2;
}

static inline float length (const float x, const float y)
{
	return sqrtf((x * x) + (y * y));
}

static inline double courseTo (vectorPt2_t *vec1, vectorPt2_t *vec2)
{
	float dlon = toRadians(vec2->lon - vec1->lon);
	float lat1 = toRadians(vec1->lat);
	float lat2 = toRadians(vec2->lat);
	float a1 = sinf(dlon) * cosf(lat2);
	float a2 = sinf(lat1) * cosf(lat2) * cosf(dlon);
	a2 = cosf(lat1) * sin(lat2) - a2;
	a2 = atan2f(a1, a2);

	if (a2 < 0.0f) a2 += TWO_PI;
	return toDegrees(a2);
}

static inline void coordinateDecode64 (const int64_t source, double *_lat, double *_lon)
{
	// Extract grid and local offset
	char encoded[8] = {0};
	char *pEncoded = &encoded[0];
	
	((int64_t*)pEncoded)[0] = source;
	int grid = ((uint16_t*)pEncoded)[0];
	double ilon = ((uint16_t*)pEncoded)[1] + (((uint32_t)pEncoded[6]) << 16);
	double ilat = ((uint16_t*)pEncoded)[2] + (((uint32_t)pEncoded[7]) << 16);
	
	
	// Recalculate 0..(180/90) coordinates
	double dlon = (uint32_t)(grid % 360) + (ilon / 10000000);
	double dlat = (uint32_t)(grid / 360) + (ilat / 10000000);
	
	// Returns to WGS84
	*_lon = (double)(dlon - 180);
	*_lat = (double)(dlat - 90);
}

static inline int64_t coordinateEncode64 (double lat, double lon)
{
	// Ensure valid lat/lon
	if (lon < -180.0) lon = 180.0+(lon+180.0); else if (lon > 180.0) lon = -180.0 + (lon-180.0);
	if (lat < -90.0) lat = 90.0 + (lat + 90.0); else if (lat > 90.0) lat = -90.0 + (lat - 90.0);


	// Move to 0..(180/90)
	double dlon = (double)lon + 180;
	double dlat = (double)lat + 90;
	
	// Calculate grid
	int grid = (((int)dlat) * 360) + ((int)dlon);
	
	// Get local offset
	uint32_t ilon = (uint32_t)((dlon - (int)(dlon))*10000000);
	uint32_t ilat = (uint32_t)((dlat - (int)(dlat))*10000000);
	
	char encoded[8] = {0};
	char *pEncoded = &encoded[0];
	
	((uint16_t*)pEncoded)[0] = (uint16_t)grid;
	((uint16_t*)pEncoded)[1] = (uint16_t)(ilon&0xFFFF);
	((uint16_t*)pEncoded)[2] = (uint16_t)(ilat&0xFFFF);
	pEncoded[6] = (char)((ilon >> 16)&0xFF);
	pEncoded[7] = (char)((ilat >> 16)&0xFF);
	
	int64_t _encoded = ((int64_t*) pEncoded)[0];
	    
	return _encoded;
}

int main (const int argc, const char *argv[])
{
#if 1
	if (argc > 1){
		printf("Importing %s ...\n", MAP_SOURCE);
		polylines_t *const polylines = importMP(MAP_SOURCE);
		if (!polylines){
			printf("Import failed\n");
			return 0;
		}
		
		printf("%i vectors\n%i paths\n\n", polylines->vectorTotal, polylines->total);
		uint32_t totalPaths = generatePolyBlocks(polylines, &coverage, POLY_PATH, atoi(argv[1]));
		printf("\n\nPaths written: %i\n", totalPaths);
	}

#else
	printf("\n");
	printf("importing map: '%s'...\n", (char*)MAP_SOURCE);

	//polylines_t *polylines = importMP(MAP_SOURCE);
	polylines_t *polylines = importPolys(POLY_PATH, &coverage);
	printf("done\n\n");
	if (!polylines) abort();

	int max = 0;
	int maxI = 0;
	
	for (int i = 0; i <= polylines->total; i++){
		polyline_t *polyline = getPolyline(polylines, i);
		if (!polyline) continue;
		vectors_t *vectors = getVectors(polyline);
		
		if (vectors->total > max){
			max = vectors->total;
			maxI = i;
		}
	}

	printf("total %i %i, %i\n", max, maxI, polylines->total);
#endif
	
	return 1;
}

