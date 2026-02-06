#ifndef DWG_H
#define DWG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Minimal stub definitions for LibreDWG types used in native-lib.cpp

#define DWG_TYPE_LINE 1
#define DWG_TYPE_LWPOLYLINE 2

typedef struct _dwg_point_3d {
  double x;
  double y;
  double z;
} Dwg_Point_3D;

typedef struct _dwg_point_2d {
  double x;
  double y;
} Dwg_Point_2D;

typedef struct _dwg_layer {
  char *name;
} Dwg_Layer;

typedef struct _dwg_entity_base {
  int color;
  Dwg_Layer *layer;
  union {
    struct _dwg_entity_LINE *LINE;
    struct _dwg_entity_LWPOLYLINE *LWPOLYLINE;
  } tio;
} Dwg_Entity_BASE;

typedef struct _dwg_entity_LINE {
  Dwg_Point_3D start;
  Dwg_Point_3D end;
} Dwg_Entity_LINE;

typedef struct _dwg_entity_LWPOLYLINE {
  int num_points;
  Dwg_Point_2D *points;
} Dwg_Entity_LWPOLYLINE;

typedef struct _dwg_object {
  int fixedtype;
  struct {
    Dwg_Entity_BASE *entity;
  } tio;
} Dwg_Object;

typedef struct _dwg_data {
  unsigned int num_objects;
  Dwg_Object *object;
} Dwg_Data;

int dwg_read_file(const char *filename, Dwg_Data *data);
void dwg_free(Dwg_Data *data);

#ifdef __cplusplus
}
#endif

#endif // DWG_H
