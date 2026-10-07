package org.julybangla.keyboard

import android.annotation.SuppressLint
import android.content.Context
import android.content.res.Configuration
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.drawable.ColorDrawable
import android.os.Build
import android.util.SparseArray
import android.view.Gravity
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.WindowInsets
import android.view.WindowManager
import android.widget.PopupWindow

/**
 * The on-screen keyboard, drawn directly on a canvas (no UI library, so the APK stays
 * small) in the style of the iPhone keyboard: white letter keys with a soft shadow on a
 * grey tray, grey function keys, and an enlarged preview above a pressed letter. It only
 * reports key presses; all typing logic is in [JulyImeService].
 */
@SuppressLint("ViewConstructor")
class KeyboardView internal constructor(context: Context, private val host: Host) : View(context) {

    interface Host {
        fun label(key: Key): String
        /** Small second label in the key's corner (the Shift character), or null. */
        fun hint(key: Key): String?
        /** Shift while on, Return while the field has an action (Go, Search, Send…). */
        fun isActive(key: Key): Boolean
        /** Colour of the mode key label: blue for বাংলা, red for ক্লাসিক, grey for English. */
        fun modeColor(): Int
        fun onKey(key: Key)
        /** Returns true if the long press was used (the key is then not sent on release). */
        fun onLongPress(key: Key): Boolean
    }

    private class Cap(val key: Key, val rect: RectF)

    private var page: List<List<Key>> = Pages.banglaLetters
    private var caps: List<Cap> = emptyList()
    private var bottomInset = 0

    private val density = resources.displayMetrics.density
    private fun dp(value: Float) = value * density

    private fun color(id: Int) = context.getColor(id)
    private val keyPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val labelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply { textAlign = Paint.Align.CENTER }
    private val hintPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        textAlign = Paint.Align.RIGHT
        color = color(R.color.kb_hint)
    }

    // Pointer id -> key held by that pointer.
    private val held = SparseArray<Cap>()
    private var longPressDone = false
    private var repeating: Cap? = null

    private val preview = KeyPreview(context)

    init {
        isHapticFeedbackEnabled = true
    }

    internal fun showPage(keys: List<List<Key>>) {
        if (page !== keys) {
            val rowsChanged = page.size != keys.size
            page = keys
            if (rowsChanged) requestLayout()
            layoutCaps()
        }
        invalidate()
    }

    fun refresh() = invalidate()

    // --- Geometry (iPhone proportions) -----------------------------------------------------

    private val landscape get() = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
    private val rowPitch get() = dp(if (landscape) 40f else 54f)
    private val gapX get() = dp(6f)
    private val gapY get() = dp(if (landscape) 7f else 11f)
    private val sidePad get() = dp(3f)
    private val topPad get() = dp(8f)
    private val bottomPad get() = dp(4f)
    private val corner get() = dp(5f)

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = (topPad + rowPitch * page.size - gapY + bottomPad * 2).toInt() + bottomInset
        setMeasuredDimension(width, height)
    }

    override fun onApplyWindowInsets(insets: WindowInsets): WindowInsets {
        // Keep the bottom row clear of the gesture/navigation bar (edge-to-edge on Android 15+).
        val bottom = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            insets.getInsets(WindowInsets.Type.navigationBars()).bottom
        } else {
            0
        }
        if (bottom != bottomInset) {
            bottomInset = bottom
            requestLayout()
        }
        return insets
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) = layoutCaps()

    private fun layoutCaps() {
        if (width == 0) return
        val unit = (width - 2 * sidePad) / 10f
        val keyHeight = rowPitch - gapY
        val result = ArrayList<Cap>()
        page.forEachIndexed { r, row ->
            val rowUnits = row.sumOf { it.width.toDouble() }.toFloat()
            // Shorter rows are centred, like the iPhone's middle letter row.
            var x = sidePad + (10f - rowUnits) * unit / 2
            val top = topPad + r * rowPitch
            for (key in row) {
                val w = key.width * unit
                result += Cap(key, RectF(x + gapX / 2, top, x + w - gapX / 2, top + keyHeight))
                x += w
            }
        }
        caps = result
        hintPaint.textSize = keyHeight * 0.24f
    }

    // --- Drawing ---------------------------------------------------------------------------

    private fun isLetter(key: Key) = key.type == KeyType.CHAR || key.type == KeyType.TEXT

    override fun onDraw(canvas: Canvas) {
        canvas.drawColor(color(R.color.kb_background))
        val keyHeight = rowPitch - gapY
        for (cap in caps) {
            val key = cap.key
            val pressed = (0 until held.size()).any { held.valueAt(it) === cap }
            val active = host.isActive(key)

            val fill: Int
            var textColor = color(R.color.kb_text)
            when {
                key.type == KeyType.ENTER && active -> {
                    fill = color(R.color.july_blue)
                    textColor = Color.WHITE
                }
                key.type == KeyType.SHIFT && active -> {
                    fill = Color.WHITE
                    textColor = Color.BLACK
                }
                isLetter(key) || key.type == KeyType.SPACE ->
                    fill = if (pressed && key.type == KeyType.SPACE) color(R.color.kb_key_pressed) else color(R.color.kb_key)
                else -> fill = if (pressed) color(R.color.kb_key) else color(R.color.kb_key_special)
            }
            if (key.type == KeyType.MODE) textColor = host.modeColor()

            // Soft shadow under each key, then the key.
            keyPaint.color = color(R.color.kb_key_shadow)
            canvas.drawRoundRect(cap.rect.left, cap.rect.top + dp(1f), cap.rect.right, cap.rect.bottom + dp(1f), corner, corner, keyPaint)
            keyPaint.color = fill
            canvas.drawRoundRect(cap.rect, corner, corner, keyPaint)

            val label = host.label(key)
            labelPaint.color = textColor
            labelPaint.textSize = when {
                isLetter(key) && label.length <= 2 -> keyHeight * 0.52f
                key.type == KeyType.SHIFT || key.type == KeyType.DELETE -> keyHeight * 0.46f
                else -> keyHeight * 0.36f
            }
            val baseline = cap.rect.centerY() - (labelPaint.descent() + labelPaint.ascent()) / 2
            canvas.drawText(label, cap.rect.centerX(), baseline, labelPaint)

            host.hint(key)?.let {
                canvas.drawText(it, cap.rect.right - dp(4f), cap.rect.top + hintPaint.textSize + dp(1f), hintPaint)
            }
        }
    }

    // --- Touch -----------------------------------------------------------------------------

    private fun capAt(x: Float, y: Float): Cap? {
        caps.firstOrNull { it.rect.contains(x, y) }?.let { return it }
        // Touches in the gaps go to the nearest key.
        return caps.minByOrNull {
            val dx = (it.rect.centerX() - x)
            val dy = (it.rect.centerY() - y)
            dx * dx + dy * dy
        }
    }

    private val longPressRunnable = Runnable {
        val cap = held.size().takeIf { it == 1 }?.let { held.valueAt(0) } ?: return@Runnable
        if (cap.key.type == KeyType.DELETE) {
            repeating = cap
            post(repeatRunnable)
        } else if (host.onLongPress(cap.key)) {
            longPressDone = true
            held.clear()
            preview.hide()
            invalidate()
        }
    }

    private val repeatRunnable = object : Runnable {
        override fun run() {
            val cap = repeating ?: return
            host.onKey(cap.key)
            postDelayed(this, 50)
        }
    }

    private fun stopTimers() {
        removeCallbacks(longPressRunnable)
        removeCallbacks(repeatRunnable)
        repeating = null
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val index = event.actionIndex
                val cap = capAt(event.getX(index), event.getY(index)) ?: return true
                // Fast two-thumb typing: a new touch sends the keys still held, in order.
                for (i in 0 until held.size()) sendOnRelease(held.valueAt(i))
                held.clear()
                stopTimers()
                longPressDone = false
                held.put(event.getPointerId(index), cap)
                performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP)
                if (isLetter(cap.key)) showPreview(cap) else preview.hide()
                if (cap.key.type == KeyType.DELETE) host.onKey(cap.key) // Delete acts on press
                postDelayed(longPressRunnable, 400)
                invalidate()
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                val id = event.getPointerId(event.actionIndex)
                val cap = held.get(id)
                held.remove(id)
                if (held.size() == 0) {
                    stopTimers()
                    preview.hide()
                }
                if (cap != null && !longPressDone) sendOnRelease(cap)
                invalidate()
            }
            MotionEvent.ACTION_CANCEL -> {
                held.clear()
                stopTimers()
                preview.hide()
                invalidate()
            }
        }
        return true
    }

    private fun sendOnRelease(cap: Cap) {
        if (cap.key.type != KeyType.DELETE) host.onKey(cap.key)
    }

    private fun showPreview(cap: Cap) {
        if (windowToken == null) return
        val origin = IntArray(2).also { getLocationInWindow(it) }
        val width = maxOf(cap.rect.width() * 1.4f, dp(44f))
        val height = (rowPitch - gapY) * 1.3f
        val x = origin[0] + cap.rect.centerX() - width / 2
        val y = origin[1] + cap.rect.top - height - dp(6f)
        preview.show(this, host.label(cap.key), x.toInt(), y.toInt(), width.toInt(), height.toInt())
    }

    override fun onDetachedFromWindow() {
        stopTimers()
        preview.hide()
        super.onDetachedFromWindow()
    }

    /** The enlarged key shown above a pressed letter, as on the iPhone. */
    private class KeyPreview(context: Context) {
        private val view = PreviewView(context)
        private val popup = PopupWindow(view).apply {
            setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
            isClippingEnabled = false
            isTouchable = false
            isFocusable = false
            animationStyle = 0
        }

        fun show(anchor: View, label: String, x: Int, y: Int, width: Int, height: Int) {
            view.label = label
            view.invalidate()
            try {
                if (popup.isShowing) {
                    popup.update(x, y, width, height)
                } else {
                    popup.width = width
                    popup.height = height
                    popup.showAtLocation(anchor, Gravity.NO_GRAVITY, x, y)
                }
            } catch (_: WindowManager.BadTokenException) {
                // The keyboard window is closing; no preview.
            }
        }

        fun hide() {
            if (popup.isShowing) popup.dismiss()
        }
    }

    private class PreviewView(context: Context) : View(context) {
        var label = ""
        private val density = resources.displayMetrics.density
        private val fill = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = context.getColor(R.color.kb_preview) }
        private val shadow = Paint(Paint.ANTI_ALIAS_FLAG).apply { color = context.getColor(R.color.kb_key_shadow) }
        private val text = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            textAlign = Paint.Align.CENTER
            color = context.getColor(R.color.kb_text)
        }

        override fun onDraw(canvas: Canvas) {
            val r = 8f * density
            val inset = 1f * density
            canvas.drawRoundRect(inset, inset * 2, width - inset, height.toFloat(), r, r, shadow)
            canvas.drawRoundRect(inset, inset, width - inset, height - inset, r, r, fill)
            text.textSize = height * 0.5f
            val baseline = height / 2f - (text.descent() + text.ascent()) / 2
            canvas.drawText(label, width / 2f, baseline, text)
        }
    }
}
