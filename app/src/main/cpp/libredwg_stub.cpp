#include "dwg.h"
#include <stdlib.h>
#include <string.h>

extern "C" {

int dwg_read_file(const char *filename, Dwg_Data *data) {
  // UPDATED DEMO DATA: Includes Lines + Text
  data->num_objects = 3;
  data->object = (Dwg_Object *)calloc(3, sizeof(Dwg_Object));

  // 1. Line (Road)
  data->object[0].fixedtype = DWG_TYPE_LINE;
  data->object[0].tio.entity =
      (Dwg_Entity_BASE *)calloc(1, sizeof(Dwg_Entity_BASE));
  data->object[0].tio.entity->color = 1;
  data->object[0].tio.entity->layer = (Dwg_Layer *)calloc(1, sizeof(Dwg_Layer));
  data->object[0].tio.entity->layer->name = strdup("Roads");
  data->object[0].tio.entity->tio.LINE =
      (Dwg_Entity_LINE *)calloc(1, sizeof(Dwg_Entity_LINE));
  data->object[0].tio.entity->tio.LINE->start.x = 4500000.0;
  data->object[0].tio.entity->tio.LINE->start.y = 500000.0;
  data->object[0].tio.entity->tio.LINE->end.x = 4501000.0;
  data->object[0].tio.entity->tio.LINE->end.y = 501000.0;

  // 2. Polyline (Parcel)
  data->object[1].fixedtype = DWG_TYPE_LWPOLYLINE;
  data->object[1].tio.entity =
      (Dwg_Entity_BASE *)calloc(1, sizeof(Dwg_Entity_BASE));
  data->object[1].tio.entity->color = 3;
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

  // 3. Text (Parcel Number)
  data->object[2].fixedtype = DWG_TYPE_TEXT;
  data->object[2].tio.entity =
      (Dwg_Entity_BASE *)calloc(1, sizeof(Dwg_Entity_BASE));
  data->object[2].tio.entity->color = 7; // White
  data->object[2].tio.entity->layer = (Dwg_Layer *)calloc(1, sizeof(Dwg_Layer));
  data->object[2].tio.entity->layer->name = strdup("Texts");
  data->object[2].tio.entity->tio.TEXT =
      (Dwg_Entity_TEXT *)calloc(1, sizeof(Dwg_Entity_TEXT));
  data->object[2].tio.entity->tio.TEXT->ins_pt.x = 4500550.0;
  data->object[2].tio.entity->tio.TEXT->ins_pt.y = 500550.0;
  data->object[2].tio.entity->tio.TEXT->text_value = strdup("101/5");

  return 0; // Success
}

void dwg_free(Dwg_Data *data) {
  // Stub
}
}
