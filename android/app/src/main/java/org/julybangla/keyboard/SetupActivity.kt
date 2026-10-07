package org.julybangla.keyboard

import android.app.Activity
import android.content.ComponentName
import android.content.Intent
import android.os.Bundle
import android.os.SystemClock
import android.provider.Settings
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.Button

/** Two-step setup (enable, then choose the keyboard) and a box to try typing. */
class SetupActivity : Activity() {

    private lateinit var enable: Button
    private lateinit var choose: Button

    private val imeId by lazy { ComponentName(this, JulyImeService::class.java).flattenToShortString() }
    private val imm by lazy { getSystemService(InputMethodManager::class.java) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_setup)
        enable = findViewById(R.id.enable)
        choose = findViewById(R.id.choose)
        enable.setOnClickListener { startActivity(Intent(Settings.ACTION_INPUT_METHOD_SETTINGS)) }
        choose.setOnClickListener { imm.showInputMethodPicker() }
        if (savedInstanceState == null) showSplash()
    }

    // The remembrance splash, as on Windows: 5 seconds, a tap closes it after 3.5 seconds
    // (so it can be read), then it fades out. Only when the app is opened, not on rotation.
    private fun showSplash() {
        val splash = findViewById<View>(R.id.splash)
        val shownAt = SystemClock.uptimeMillis()
        val hide = Runnable {
            splash.animate().alpha(0f).setDuration(350).withEndAction { splash.visibility = View.GONE }
        }
        splash.visibility = View.VISIBLE
        splash.alpha = 0f
        splash.animate().alpha(1f).setDuration(350)
        splash.postDelayed(hide, 5000)
        splash.setOnClickListener {
            if (SystemClock.uptimeMillis() - shownAt >= 3500) {
                splash.removeCallbacks(hide)
                hide.run()
            }
        }
    }

    override fun onResume() {
        super.onResume()
        updateSteps()
    }

    // The picker is a dialog over this activity, so focus returns here after a choice.
    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) updateSteps()
    }

    private fun updateSteps() {
        val enabled = imm.enabledInputMethodList.any { it.id == imeId }
        val selected = Settings.Secure.getString(contentResolver, Settings.Secure.DEFAULT_INPUT_METHOD) == imeId
        enable.text = getString(R.string.step_enable) + if (enabled) "   " + getString(R.string.status_enabled) else ""
        choose.text = getString(R.string.step_choose) + if (selected) "   " + getString(R.string.status_selected) else ""
        choose.isEnabled = enabled
    }
}
