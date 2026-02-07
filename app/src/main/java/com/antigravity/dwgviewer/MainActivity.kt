package com.antigravity.dwgviewer

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.widget.TextView

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        // Simple functional check
        findViewById<TextView>(R.id.hello_text).text = "Build Success!"
    }
}
