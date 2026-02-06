package com.antigravity.dwgviewer

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.antigravity.dwgviewer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        // Maps logic commented out to match dependencies
        // setupMap()
    }
    
    /*
    private fun setupMap() {
        ...
    }
    */
}
