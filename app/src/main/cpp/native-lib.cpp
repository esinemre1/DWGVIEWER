#include <jni.h>
#include <string>
#include <vector>
#include <cmath>
#include <android/log.h>

// Mock LibreDWG includes for compilation logic
// #include <dwg.h> 
// In a real build, the above include is active. 
// For this generation, I will define minimal needed structs if headers aren't present,
// but I will write the code assuming the standard LibreDWG API.

extern "C" {
    #include "dwg.h"
    #include "dwg_api.h"
}

#define TAG "DWG_ENGINE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

/* 
 * Minimal Gauss-Kruger / Transverse Mercator to WGS84 implementation.
 * Using standard 6-degree or 3-degree zone logic based on DOM (Central Meridian).
 * Input: Easting (Y), Northing (X), CM (DOM)
 * Output: Lat, Lon (Degrees)
 * Ellipsoid: GRS80 (practically identical to WGS84 for this precision)
 */

struct LatLon {
    double lat;
    double lon;
};

// WGS84 Ellipsoid constants
const double a = 6378137.0;
const double f = 1.0 / 298.257223563;
const double b = a * (1.0 - f);
const double e2 = (a*a - b*b) / (a*a);
const double e_prime_2 = (a*a - b*b) / (b*b);

double to_rad(double deg) { return deg * M_PI / 180.0; }
double to_deg(double rad) { return rad * 180.0 / M_PI; }

LatLon tm_to_wgs84(double easting, double northing, double cm) {
    // Standard TM parameters (often used in Turkey for DOM 27/30/33/36 3-degree)
    // False Easting is usually 500,000 for 3-degree zones in Turkey (ITRF/ED50) or UTM
    double k0 = 1.0; // Gauss-Kruger uses 1.0. UTM uses 0.9996. Surveyor asked for "Gauss-Kruger".
                     // Note: ITRF96 3-degree usually uses k0=1.0. 
                     // If it's UTM 6-degree, it's 0.9996. 
                     // User said "Dynamic Gauss-Kruger", usually implies k0=1.0 for 3-degree zones.
    
    double false_easting = 500000.0;
    double false_northing = 0.0;
    
    // Calculations
    double M = (northing - false_northing) / k0;
    double mu = M / (a * (1.0 - e2/4.0 - 3.0*e2*e2/64.0 - 5.0*pow(e2,3)/256.0));
    
    double e1 = (1.0 - sqrt(1.0 - e2)) / (1.0 + sqrt(1.0 - e2));
    double J1 = (3.0*e1/2.0 - 27.0*pow(e1,3)/32.0);
    double J2 = (21.0*e1*e1/16.0 - 55.0*pow(e1,4)/32.0);
    double J3 = (151.0*pow(e1,3)/96.0);
    double J4 = (1097.0*pow(e1,4)/512.0);
    
    double phi1 = mu + J1*sin(2.0*mu) + J2*sin(4.0*mu) + J3*sin(6.0*mu) + J4*sin(8.0*mu);
    
    double C1 = e_prime_2 * pow(cos(phi1), 2);
    double T1 = pow(tan(phi1), 2);
    double N1 = a / sqrt(1.0 - e2 * pow(sin(phi1), 2));
    double R1 = a * (1.0 - e2) / pow((1.0 - e2 * pow(sin(phi1), 2)), 1.5);
    double D = (easting - false_easting) / (N1 * k0);
    
    double lat_rad = phi1 - (N1 * tan(phi1) / R1) * (D*D/2.0 - (5.0 + 3.0*T1 + 10.0*C1 - 4.0*C1*C1 - 9.0*e_prime_2)*pow(D,4)/24.0 + (61.0 + 90.0*T1 + 298.0*C1 + 45.0*T1*T1 - 252.0*e_prime_2 - 3.0*C1*C1)*pow(D,6)/720.0);
    double lon_rad = (D - (1.0 + 2.0*T1 + C1)*pow(D,3)/6.0 + (5.0 - 2.0*C1 + 28.0*T1 - 3.0*C1*C1 + 8.0*e_prime_2 + 24.0*T1*T1)*pow(D,5)/120.0) / cos(phi1);
    
    return { to_deg(lat_rad), cm + to_deg(lon_rad) };
}

// ---------------- JNI Implementation ----------------

extern "C" JNIEXPORT jobject JNICALL
Java_com_antigravity_dwgviewer_DwgNative_parseDwg(
        JNIEnv* env,
        jobject /* this */,
        jstring pathStr,
        jint dom) {
    
    const char* filename = env->GetStringUTFChars(pathStr, 0);
    
    // 1. Initialize DWG
    Dwg_Data dwg_data = {0};
    int success = dwg_read_file(filename, &dwg_data);
    env->ReleaseStringUTFChars(pathStr, filename);
    
    if (success != 0) {
        LOGE("Failed to read DWG file code: %d", success);
        return nullptr; 
    }

    // Vectors to hold results temporarily
    std::vector<float> lineCoords; // x1, y1, x2, y2
    std::vector<int> colors;
    std::vector<std::string> layers;
    
    // 2. Iterate Objects
    // Using LibreDWG API access
    for (unsigned int i = 0; i < dwg_data.num_objects; i++) {
        Dwg_Object* obj = &dwg_data.object[i];
        
        // Handle Lines (LINE)
        if (obj->fixedtype == DWG_TYPE_LINE) {
            Dwg_Entity_LINE* line = obj->tio.entity->tio.LINE;
            if (!line) continue;
            
            // Get Coords
            double x1 = line->start.x;
            double y1 = line->start.y;
            double x2 = line->end.x;
            double y2 = line->end.y;
            
            // Transform
            LatLon p1 = tm_to_wgs84(y1, x1, (double)dom); // Note: Surveyors usually swap X/Y. X is North, Y is East.
            LatLon p2 = tm_to_wgs84(y2, x2, (double)dom);
            
            // Store WGS84 lat/lng
            lineCoords.push_back((float)p1.lat);
            lineCoords.push_back((float)p1.lon);
            lineCoords.push_back((float)p2.lat);
            lineCoords.push_back((float)p2.lon);
            
            // Get Props
            // Color: ACI. If 256, use layer color. Simplification: just take entity color or raw 0.
            int color = obj->tio.entity->color; 
            colors.push_back(color);
            
            // Layer
            char* layerName = "0";
            if (obj->tio.entity->layer) {
                layerName = obj->tio.entity->layer->name;
            }
            layers.push_back(std::string(layerName ? layerName : "Default"));
        }
        
        // Lightweight Polyline (LWPOLYLINE) - Common in mapping
        else if (obj->fixedtype == DWG_TYPE_LWPOLYLINE) {
             Dwg_Entity_LWPOLYLINE* pl = obj->tio.entity->tio.LWPOLYLINE;
             if (!pl) continue;
             
             // Iterate vertices
             for (int v = 0; v < pl->num_points - 1; v++) {
                 double x1 = pl->points[v].x;
                 double y1 = pl->points[v].y;
                 double x2 = pl->points[v+1].x;
                 double y2 = pl->points[v+1].y;
                 
                 LatLon p1 = tm_to_wgs84(y1, x1, (double)dom);
                 LatLon p2 = tm_to_wgs84(y2, x2, (double)dom);
                 
                 lineCoords.push_back((float)p1.lat);
                 lineCoords.push_back((float)p1.lon);
                 lineCoords.push_back((float)p2.lat);
                 lineCoords.push_back((float)p2.lon);
                 
                 colors.push_back(obj->tio.entity->color);
                 char* layerName = "0";
                 if (obj->tio.entity->layer) layerName = obj->tio.entity->layer->name;
                 layers.push_back(std::string(layerName ? layerName : "Default"));
             }
        }
    }
    
    dwg_free(&dwg_data);

    // 3. Construct Java Result Object
    // We need to return a Data Class: DwgResult(float[] lines, int[] colors, String[] layers)
    
    // Find class
    jclass resultInfoCls = env->FindClass("com/antigravity/dwgviewer/DwgResult");
    jmethodID constructor = env->GetMethodID(resultInfoCls, "<init>", "([F[I[Ljava/lang/String;)V");

    // Float Array
    jfloatArray jLines = env->NewFloatArray(lineCoords.size());
    env->SetFloatArrayRegion(jLines, 0, lineCoords.size(), lineCoords.data());
    
    // Int Array
    jintArray jColors = env->NewIntArray(colors.size());
    env->SetIntArrayRegion(jColors, 0, colors.size(), colors.data());
    
    // String Array
    jobjectArray jLayers = env->NewObjectArray(layers.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for (size_t i = 0; i < layers.size(); i++) {
        env->SetObjectArrayElement(jLayers, i, env->NewStringUTF(layers[i].c_str()));
    }
    
    // Create Object
    jobject resultObj = env->NewObject(resultInfoCls, constructor, jLines, jColors, jLayers);
    return resultObj;
}
