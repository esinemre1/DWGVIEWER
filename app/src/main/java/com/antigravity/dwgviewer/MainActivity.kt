package com.antigravity.dwgviewer

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.antigravity.dwgviewer.databinding.ActivityMainBinding
import com.google.android.gms.maps.CameraUpdateFactory
import com.google.android.gms.maps.GoogleMap
import com.google.android.gms.maps.SupportMapFragment
import com.google.android.gms.maps.model.*
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
    
    // Store references to map objects for toggling
    private val lineObjects = mutableListOf<MapLine>() 
    private val textObjects = mutableListOf<MapText>()
    
    // Current settings
    private var selectedDom = 27

    data class MapLine(val polyline: Polyline, val layerName: String)
    data class MapText(val marker: Marker, val layerName: String)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupMap()
        setupUI()
        
        // Simulating auto-load for debug purpose if file provided? No, just wait for user.
        // But we prompt DOM immediately
        showDomSelectionDialog()
    }

    private fun setupMap() {
        val mapFragment = supportFragmentManager.findFragmentById(R.id.map) as SupportMapFragment
        mapFragment.getMapAsync { map ->
            googleMap = map
            map.mapType = GoogleMap.MAP_TYPE_SATELLITE
            map.uiSettings.isRotateGesturesEnabled = true
            map.uiSettings.isZoomControlsEnabled = false 
            map.uiSettings.isCompassEnabled = true
        }
    }

    private fun setupUI() {
        binding.btnLoadDwg.setOnClickListener { openFilePicker() }
        binding.btnLayers.setOnClickListener { showLayerManager() }
    }

    private fun showDomSelectionDialog() {
        val doms = arrayOf("27", "30", "33", "36", "39", "42", "45")
        AlertDialog.Builder(this)
            .setTitle("Select Zone (DOM)")
            .setItems(doms) { _, which ->
                selectedDom = doms[which].toInt()
                Toast.makeText(this, "Zone $selectedDom selected", Toast.LENGTH_SHORT).show()
            }
            .setCancelable(false)
            .show()
    }

    private val filePickerLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
        uri?.let {
            val tempFile = File(cacheDir, "temp.dwg")
            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(tempFile).use { output -> input.copyTo(output) }
            }
            loadDwg(tempFile.absolutePath)
        }
    }

    private fun openFilePicker() {
        filePickerLauncher.launch("*/*") 
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
        map.clear()
        lineObjects.clear()
        textObjects.clear()
        layerVisibility.clear()

        // LINES
        val lines = result.lines
        val colors = result.lineColors
        val layers = result.lineLayers
        
        // TEXTS
        val txtCoords = result.textCoords
        val txtContent = result.textContents
        val txtLayers = result.textLayers
        
        val builder = LatLngBounds.Builder()
        var hasPoints = false

        // 1. Process Lines
        val count = colors.size // assumes matched size
        for (i in 0 until count) {
            val idx = i * 4
            if (idx + 3 >= lines.size) break
            
            val p1 = LatLng(lines[idx].toDouble(), lines[idx+1].toDouble())
            val p2 = LatLng(lines[idx+2].toDouble(), lines[idx+3].toDouble())
            
            if (p1.latitude == 0.0) continue
            
            val layer = layers[i]
            layerVisibility[layer] = true
            
            val polyline = map.addPolyline(PolylineOptions()
                .add(p1, p2)
                .color(getAciColor(colors[i]))
                .width(2f)) 
            
            lineObjects.add(MapLine(polyline, layer))
            builder.include(p1)
            builder.include(p2)
            hasPoints = true
        }

        // 2. Process Text
        for (i in txtContent.indices) {
            val idx = i * 2
            val p = LatLng(txtCoords[idx].toDouble(), txtCoords[idx+1].toDouble())
            val text = txtContent[i]
            val layer = txtLayers[i]
            
            layerVisibility[layer] = true
            
            val marker = map.addMarker(MarkerOptions()
                .position(p)
                .icon(BitmapDescriptorFactory.fromBitmap(createCustomMarker(this, text)))
                .anchor(0.5f, 0.5f))
            
            if (marker != null) {
                textObjects.add(MapText(marker, layer))
            }
        }
        
        if (hasPoints) {
            try { map.moveCamera(CameraUpdateFactory.newLatLngBounds(builder.build(), 100)) } 
            catch (e: Exception) {}
        }
        
        Toast.makeText(this, "Loaded ${count} lines, ${txtContent.size} texts", Toast.LENGTH_SHORT).show()
    }
    
    // Helper to draw text as bitmap
    private fun createCustomMarker(context: Context, text: String): Bitmap {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)
        paint.textSize = 30f
        paint.color = Color.WHITE
        paint.setShadowLayer(5f, 0f, 0f, Color.BLACK) // Text outline/shadow for visibility
        paint.textAlign = Paint.Align.LEFT
        
        val baseline = -paint.ascent() 
        val width = (paint.measureText(text) + 10).toInt()
        val height = (baseline + paint.descent() + 5).toInt()
        
        val image = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(image)
        canvas.drawText(text, 5f, baseline, paint)
        return image
    }
    
    private fun showLayerManager() {
        val sortedLayers = layerVisibility.keys.sorted()
        val layerList = sortedLayers.toTypedArray()
        val checkedItems = BooleanArray(layerList.size) { layerVisibility[layerList[it]] ?: true }
        
        AlertDialog.Builder(this)
            .setTitle("Layer Visibility")
            .setMultiChoiceItems(layerList, checkedItems) { _, which, isChecked ->
                val layer = layerList[which]
                layerVisibility[layer] = isChecked
                updateLayerVisibility(layer, isChecked)
            }
            .setPositiveButton("Close", null)
            .show()
    }
    
    private fun updateLayerVisibility(layer: String, visible: Boolean) {
        lineObjects.filter { it.layerName == layer }.forEach { it.polyline.isVisible = visible }
        textObjects.filter { it.layerName == layer }.forEach { it.marker.isVisible = visible }
    }

    private fun getAciColor(aci: Int): Int {
        return when (aci) {
            1 -> Color.RED
            2 -> Color.YELLOW
            3 -> Color.GREEN
            4 -> Color.CYAN
            5 -> Color.BLUE
            6 -> Color.MAGENTA
            7 -> Color.WHITE
            else -> Color.LTGRAY 
        }
    }
}
