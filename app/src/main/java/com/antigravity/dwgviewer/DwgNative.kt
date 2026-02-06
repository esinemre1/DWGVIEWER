package com.antigravity.dwgviewer

// Data class to hold JNI results
class DwgResult(
    val lines: FloatArray,
    val lineColors: IntArray,
    val lineLayers: Array<String>,
    val textCoords: FloatArray,   // [lat1, lon1, lat2, lon2...]
    val textContents: Array<String>,
    val textLayers: Array<String>
)
