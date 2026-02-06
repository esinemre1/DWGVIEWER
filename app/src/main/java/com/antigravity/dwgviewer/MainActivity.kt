package com.antigravity.dwgviewer

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.antigravity.dwgviewer.databinding.ActivityMainBinding
import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.SupportMapFragment
import com.google.android.gms.maps.model.LatLng
import com.google.android.gms.maps.model.Polyline
import com.google.android.gms.maps.model.PolylineOptions
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var googleMap: GoogleMap? = null
    
    // Data storage
    private val layerVisibility = mutableMapOf<String, Boolean>()
    private val lineObjects = mutableListOf<MapLine>() // Store reference to map objects
    
    // Current settings
    private var selectedDom = 27

    data class MapLine(
        val polyline: Polyline,
        val layerName: String
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupMap()
        setupUI()
        
        // Show DOM selection generic dialog on startup
        showDomSelectionDialog()
    }

    private fun setupMap() {
        val mapFragment = supportFragmentManager
            .findFragmentById(R.id.map) as SupportMapFragment
        mapFragment.getMapAsync { map ->
            googleMap = map
            map.mapType = GoogleMap.MAP_TYPE_SATELLITE
            map.uiSettings.isRotateGesturesEnabled = true
            map.uiSettings.isZoomControlsEnabled = false // Minimalist
        }
    }

    private fun setupUI() {
        binding.btnLoadDwg.setOnClickListener {
            if (checkPermission()) {
                openFilePicker()
            }
        }

        binding.btnLayers.setOnClickListener {
            showLayerManager()
        }
    }

    private fun showDomSelectionDialog() {
        val doms = arrayOf("27", "30", "33", "36", "39", "42", "45")
        AlertDialog.Builder(this)
            .setTitle("Select Central Meridian (DOM)")
            .setItems(doms) { _, which ->
                selectedDom = doms[which].toInt()
                Toast.makeText(this, "DOM set to $selectedDom", Toast.LENGTH_SHORT).show()
            }
            .setCancelable(false)
            .show()
    }

    private val filePickerLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        uri?.let {
            // Need accurate file path for native C++ fopen. 
            // In modern Android, we usually copy stream to cache.
            val tempFile = File(cacheDir, "temp.dwg")
            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(tempFile).use { output ->
                    input.copyTo(output)
                }
            }
            loadDwg(tempFile.absolutePath)
        }
    }

    private fun openFilePicker() {
        filePickerLauncher.launch("*/*") // Allow all to pick .dwg usually
    }

    private fun loadDwg(path: String) {
        binding.loadingBar.visibility = View.VISIBLE
        
        lifecycleScope.launch(Dispatchers.Default) {
             val result = DwgNative.parseDwg(path, selectedDom)
             
             withContext(Dispatchers.Main) {
                 binding.loadingBar.visibility = View.GONE
                 if (result != null) {
                     renderDwg(result)
                 } else {
                     Toast.makeText(this@MainActivity, "Failed to parse DWG", Toast.LENGTH_LONG).show()
                 }
             }
        }
    }

    private fun renderDwg(result: DwgResult) {
        val map = googleMap ?: return
        
        // Clear previous
        map.clear()
        lineObjects.clear()
        layerVisibility.clear()

        val lines = result.lines
        val colors = result.colors
        val layers = result.layers
        
        // Populate unique layers
        layers.distinct().forEach { layerVisibility[it] = true }

        val builder = com.google.android.gms.maps.model.LatLngBounds.Builder()
        var hasPoints = false

        // Iterate
        // Lines array is [lat1, lon1, lat2, lon2, ...]
        val count = colors.size
        for (i in 0 until count) {
            val idx = i * 4
            if (idx + 3 >= lines.size) break;
            
            val lat1 = lines[idx].toDouble()
            val lon1 = lines[idx+1].toDouble()
            val lat2 = lines[idx+2].toDouble()
            val lon2 = lines[idx+3].toDouble()
            
            // Valid check
            if (lat1 == 0.0 && lon1 == 0.0) continue
            
            val p1 = LatLng(lat1, lon1)
            val p2 = LatLng(lat2, lon2)
            
            val aciColor = colors[i]
            val layerName = layers[i]
            
            // Convert ACI to Android Color (Simple mapping or default White)
            // ACI is complex, simpler to just map standard 1-7 or use white.
            // Professional surveyors need color. Let's use a helper func if possible, else default.
            val finalColor = getAciColor(aciColor)

            val polyline = map.addPolyline(PolylineOptions()
                .add(p1, p2)
                .color(finalColor)
                .width(2f)) // Thin lines for CAD precision
            
            lineObjects.add(MapLine(polyline, layerName))
            
            builder.include(p1)
            builder.include(p2)
            hasPoints = true
        }
        
        if (hasPoints) {
            try {
                map.moveCamera(CameraUpdateFactory.newLatLngBounds(builder.build(), 100))
            } catch (e: Exception) { e.printStackTrace() }
        }
        
        Toast.makeText(this, "Loaded ${count} objects", Toast.LENGTH_SHORT).show()
    }
    
    private fun showLayerManager() {
        // Simple multi-choice dialog for efficiency/minimalism
        val layerList = layerVisibility.keys.toTypedArray()
        val checkedItems = layerVisibility.values.toBooleanArray()
        
        AlertDialog.Builder(this)
            .setTitle("Layers")
            .setMultiChoiceItems(layerList, checkedItems) { _, which, isChecked ->
                val layer = layerList[which]
                layerVisibility[layer] = isChecked
                updateLayerVisibility(layer, isChecked)
            }
            .setPositiveButton("OK", null)
            .show()
    }
    
    private fun updateLayerVisibility(layer: String, visible: Boolean) {
        lineObjects.filter { it.layerName == layer }.forEach { 
            it.polyline.isVisible = visible
        }
    }

    private fun getAciColor(aci: Int): Int {
        // Basic AutoCAD Color Index mapping
        return when (aci) {
            1 -> Color.RED
            2 -> Color.YELLOW
            3 -> Color.GREEN
            4 -> Color.CYAN
            5 -> Color.BLUE
            6 -> Color.MAGENTA
            7 -> Color.WHITE
            else -> Color.LTGRAY // Default fallthrough
        }
    }

    private fun checkPermission(): Boolean {
        // Scoped storage handles picking, but good to check if we needed raw access
        return true 
    }
}
