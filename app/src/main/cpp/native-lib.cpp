#include <jni.h>
#include <string>
#include <vector>
#define _USE_MATH_DEFINES // Ensure M_PI is visible
#include <android/log.h>
#include <cmath>
#include <sstream>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern "C" {
#include "dwg.h"
}

// ... (Rest of the file is identical, assume valid from previous context,
// re-pasting for completeness if I were replacing, but for edit I'd use
// replace. Since I'm using write_to_file overwrite=true, I must provide full
// content. To save tokens/time and ensure correctness, I will paste the full
// working version with the fix.)

#define TAG "DWG_ENGINE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

struct LatLon {
  double lat;
  double lon;
};

const double a = 6378137.0;
const double f = 1.0 / 298.257223563;
const double b = a * (1.0 - f);
const double e2 = (a * a - b * b) / (a * a);
const double e_prime_2 = (a * a - b * b) / (b * b);

double to_deg(double rad) { return rad * 180.0 / M_PI; }

LatLon tm_to_wgs84(double easting, double northing, double cm) {
  double k0 = 1.0;
  double false_easting = 500000.0;
  double false_northing = 0.0;

  double M = (northing - false_northing) / k0;
  double mu =
      M /
      (a * (1.0 - e2 / 4.0 - 3.0 * e2 * e2 / 64.0 - 5.0 * pow(e2, 3) / 256.0));

  double e1 = (1.0 - sqrt(1.0 - e2)) / (1.0 + sqrt(1.0 - e2));
  double J1 = (3.0 * e1 / 2.0 - 27.0 * pow(e1, 3) / 32.0);
  double J2 = (21.0 * e1 * e1 / 16.0 - 55.0 * pow(e1, 4) / 32.0);
  double J3 = (151.0 * pow(e1, 3) / 96.0);
  double J4 = (1097.0 * pow(e1, 4) / 512.0);

  double phi1 = mu + J1 * sin(2.0 * mu) + J2 * sin(4.0 * mu) +
                J3 * sin(6.0 * mu) + J4 * sin(8.0 * mu);

  double C1 = e_prime_2 * pow(cos(phi1), 2);
  double T1 = pow(tan(phi1), 2);
  double N1 = a / sqrt(1.0 - e2 * pow(sin(phi1), 2));
  double R1 = a * (1.0 - e2) / pow((1.0 - e2 * pow(sin(phi1), 2)), 1.5);
  double D = (easting - false_easting) / (N1 * k0);

  double lat_rad =
      phi1 -
      (N1 * tan(phi1) / R1) *
          (D * D / 2.0 -
           (5.0 + 3.0 * T1 + 10.0 * C1 - 4.0 * C1 * C1 - 9.0 * e_prime_2) *
               pow(D, 4) / 24.0 +
           (61.0 + 90.0 * T1 + 298.0 * C1 + 45.0 * T1 * T1 - 252.0 * e_prime_2 -
            3.0 * C1 * C1) *
               pow(D, 6) / 720.0);
  double lon_rad = (D - (1.0 + 2.0 * T1 + C1) * pow(D, 3) / 6.0 +
                    (5.0 - 2.0 * C1 + 28.0 * T1 - 3.0 * C1 * C1 +
                     8.0 * e_prime_2 + 24.0 * T1 * T1) *
                        pow(D, 5) / 120.0) /
                   cos(phi1);

  return {to_deg(lat_rad), cm + to_deg(lon_rad)};
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_antigravity_dwgviewer_DwgNative_parseDwg(JNIEnv *env,
                                                  jobject /* this */,
                                                  jstring pathStr, jint dom) {

  const char *filename = env->GetStringUTFChars(pathStr, 0);
  Dwg_Data dwg_data = {0};
  int success = dwg_read_file(filename, &dwg_data);
  env->ReleaseStringUTFChars(pathStr, filename);

  if (success != 0)
    return nullptr;

  std::vector<float> lineCoords;
  std::vector<int> lineColors;
  std::vector<std::string> lineLayers;
  std::vector<float> textCoords;
  std::vector<std::string> textContents;
  std::vector<std::string> textLayers;

  for (unsigned int i = 0; i < dwg_data.num_objects; i++) {
    Dwg_Object *obj = &dwg_data.object[i];
    if (!obj->tio.entity)
      continue;

    char *rawLayer = "0";
    if (obj->tio.entity->layer && obj->tio.entity->layer->name)
      rawLayer = obj->tio.entity->layer->name;
    std::string layerName(rawLayer);
    int color = obj->tio.entity->color;

    if (obj->fixedtype == DWG_TYPE_LINE) {
      Dwg_Entity_LINE *line = obj->tio.entity->tio.LINE;
      if (line) {
        LatLon p1 = tm_to_wgs84(line->start.y, line->start.x, (double)dom);
        LatLon p2 = tm_to_wgs84(line->end.y, line->end.x, (double)dom);
        lineCoords.push_back((float)p1.lat);
        lineCoords.push_back((float)p1.lon);
        lineCoords.push_back((float)p2.lat);
        lineCoords.push_back((float)p2.lon);
        lineColors.push_back(color);
        lineLayers.push_back(layerName);
      }
    } else if (obj->fixedtype == DWG_TYPE_LWPOLYLINE) {
      Dwg_Entity_LWPOLYLINE *pl = obj->tio.entity->tio.LWPOLYLINE;
      if (pl && pl->points) {
        for (int v = 0; v < pl->num_points - 1; v++) {
          LatLon p1 =
              tm_to_wgs84(pl->points[v].y, pl->points[v].x, (double)dom);
          LatLon p2 = tm_to_wgs84(pl->points[v + 1].y, pl->points[v + 1].x,
                                  (double)dom);
          lineCoords.push_back((float)p1.lat);
          lineCoords.push_back((float)p1.lon);
          lineCoords.push_back((float)p2.lat);
          lineCoords.push_back((float)p2.lon);
          lineColors.push_back(color);
          lineLayers.push_back(layerName);
        }
      }
    } else if (obj->fixedtype == DWG_TYPE_TEXT) {
      Dwg_Entity_TEXT *txt = obj->tio.entity->tio.TEXT;
      if (txt && txt->text_value) {
        LatLon p = tm_to_wgs84(txt->ins_pt.y, txt->ins_pt.x, (double)dom);
        textCoords.push_back((float)p.lat);
        textCoords.push_back((float)p.lon);
        textContents.push_back(std::string(txt->text_value));
        textLayers.push_back(layerName);
      }
    }
  }
  dwg_free(&dwg_data);

  jclass resultCls = env->FindClass("com/antigravity/dwgviewer/DwgResult");
  jmethodID constructor = env->GetMethodID(
      resultCls, "<init>",
      "([F[I[Ljava/lang/String;[F[Ljava/lang/String;[Ljava/lang/String;)V");

  jfloatArray jLines = env->NewFloatArray(lineCoords.size());
  env->SetFloatArrayRegion(jLines, 0, lineCoords.size(), lineCoords.data());
  jintArray jColors = env->NewIntArray(lineColors.size());
  env->SetIntArrayRegion(jColors, 0, lineColors.size(), lineColors.data());
  jobjectArray jLayers =
      env->NewObjectArray(lineLayers.size(), env->FindClass("java/lang/String"),
                          env->NewStringUTF(""));
  for (size_t i = 0; i < lineLayers.size(); i++)
    env->SetObjectArrayElement(jLayers, i,
                               env->NewStringUTF(lineLayers[i].c_str()));
  jfloatArray jTxtCoords = env->NewFloatArray(textCoords.size());
  env->SetFloatArrayRegion(jTxtCoords, 0, textCoords.size(), textCoords.data());
  jobjectArray jTxtContent = env->NewObjectArray(
      textContents.size(), env->FindClass("java/lang/String"),
      env->NewStringUTF(""));
  for (size_t i = 0; i < textContents.size(); i++)
    env->SetObjectArrayElement(jTxtContent, i,
                               env->NewStringUTF(textContents[i].c_str()));
  jobjectArray jTxtLayers =
      env->NewObjectArray(textLayers.size(), env->FindClass("java/lang/String"),
                          env->NewStringUTF(""));
  for (size_t i = 0; i < textLayers.size(); i++)
    env->SetObjectArrayElement(jTxtLayers, i,
                               env->NewStringUTF(textLayers[i].c_str()));

  return env->NewObject(resultCls, constructor, jLines, jColors, jLayers,
                        jTxtCoords, jTxtContent, jTxtLayers);
}
