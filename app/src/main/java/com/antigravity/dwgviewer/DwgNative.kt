package com.antigravity.dwgviewer

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

// Native interface
object DwgNative {
    init {
        try {
            System.loadLibrary("dwg-engine")
        } catch (e: UnsatisfiedLinkError) {
            e.printStackTrace()
        }
    }

    external fun parseDwg(filePath: String, dom: Int): DwgResult?
}

// Data class to hold JNI results
// lines array structure: [lat1, lon1, lat2, lon2, lat3, lon3, lat4, lon4, ...]
// One line segment corresponds to 4 floats.
class DwgResult(
    val lines: FloatArray,
    val colors: IntArray,
    val layers: Array<String>
)
