package com.antigravity.dwgviewer

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.widget.TextView

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        // Minimal logic to verify build
        findViewById<TextView>(R.id.hello_text).text = "Factory Reset Build"
    }
}
