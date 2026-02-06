#include "dwg.h"
#include <stdlib.h>
#include <string.h>

// STUB IMPLEMENTATION OF LIBREDWG
// This allows the app to compile and run in a demo mode.
// In a real scenario, this file is replaced by linking against libredwg.a

extern "C" {

int dwg_read_file(const char *filename, Dwg_Data *data) {
  // Generate dummy data for demonstration purposes so the user sees something
  // on the map
  data->num_objects = 2;
  data->object = (Dwg_Object *)calloc(2, sizeof(Dwg_Object));

  // Object 1: A Line
  data->object[0].fixedtype = DWG_TYPE_LINE;
  data->object[0].tio.entity =
      (Dwg_Entity_BASE *)calloc(1, sizeof(Dwg_Entity_BASE));
  data->object[0].tio.entity->color = 1; // Red

  // Create a dummy layer
  data->object[0].tio.entity->layer = (Dwg_Layer *)calloc(1, sizeof(Dwg_Layer));
  data->object[0].tio.entity->layer->name = strdup("Roads");

  data->object[0].tio.entity->tio.LINE =
      (Dwg_Entity_LINE *)calloc(1, sizeof(Dwg_Entity_LINE));
  // Sample coords (approx Turkey coords for DOM 27/30 etc demo)
  // Local coords (Y=Easting, X=Northing)
  data->object[0].tio.entity->tio.LINE->start.x = 4500000.0; // Northing
  data->object[0].tio.entity->tio.LINE->start.y = 500000.0;  // Easting
  data->object[0].tio.entity->tio.LINE->end.x = 4501000.0;
  data->object[0].tio.entity->tio.LINE->end.y = 501000.0;

  // Object 2: A Polyline
  data->object[1].fixedtype = DWG_TYPE_LWPOLYLINE;
  data->object[1].tio.entity =
      (Dwg_Entity_BASE *)calloc(1, sizeof(Dwg_Entity_BASE));
  data->object[1].tio.entity->color = 3; // Green
  data->object[1].tio.entity->layer = (Dwg_Layer *)calloc(1, sizeof(Dwg_Layer));
  data->object[1].tio.entity->layer->name = strdup("Parcels");

  data->object[1].tio.entity->tio.LWPOLYLINE =
      (Dwg_Entity_LWPOLYLINE *)calloc(1, sizeof(Dwg_Entity_LWPOLYLINE));
  data->object[1].tio.entity->tio.LWPOLYLINE->num_points = 3;
  data->object[1].tio.entity->tio.LWPOLYLINE->points =
      (Dwg_Point_2D *)calloc(3, sizeof(Dwg_Point_2D));

  data->object[1].tio.entity->tio.LWPOLYLINE->points[0].x = 4500500.0;
  data->object[1].tio.entity->tio.LWPOLYLINE->points[0].y = 500500.0;
  data->object[1].tio.entity->tio.LWPOLYLINE->points[1].x = 4500600.0;
  data->object[1].tio.entity->tio.LWPOLYLINE->points[1].y = 500600.0;
  data->object[1].tio.entity->tio.LWPOLYLINE->points[2].x = 4500500.0;
  data->object[1].tio.entity->tio.LWPOLYLINE->points[2].y = 500700.0;

  return 0; // Success
}

void dwg_free(Dwg_Data *data) {
  if (data->object) {
    // Full free logic omitted for stub
    free(data->object);
  }
}
}
