package org.julybangla.keyboard

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.os.Build
import android.util.SparseArray
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.WindowInsets

/**
 * The on-screen keyboard, drawn directly on a canvas (no UI library, so the APK stays
 * small). It only reports key presses; all typing logic is in [JulyImeService].
 */
@SuppressLint("ViewConstructor")
class KeyboardView internal constructor(context: Context, private val host: Host) : View(context) {

    interface Host {
        fun label(key: Key): String
        /** Small second label in the key's corner (the Shift character), or null. */
        fun hint(key: Key): String?
        /** Highlighted keys: Shift while active. */
        fun isActive(key: Key): Boolean
        /** Colour of the mode key label: blue for বাংলা, red for ক্লাসিক, grey for English. */
        fun modeColor(): Int
        fun onKey(key: Key)
        /** Returns true if the long press was used (the key is then not sent on release). */
        fun onLongPress(key: Key): Boolean
    }

    private class Cap(val key: Key, val rect: RectF)

    private var page: List<List<Key>> = Pages.letters
    private var caps: List<Cap> = emptyList()
    private var bottomInset = 0

    private val density = resources.displayMetrics.density
    private val keyGap = 3f * density
    private val corner = 6f * density

    private fun color(id: Int) = context.getColor(id)
    private val backgroundColor = color(R.color.kb_background)
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

    init {
        isHapticFeedbackEnabled = true
    }

    internal fun showPage(keys: List<List<Key>>) {
        if (page !== keys) {
            page = keys
            layoutCaps()
        }
        invalidate()
    }

    fun refresh() = invalidate()

    private fun rowHeightPx(): Float {
        val landscape = resources.configuration.orientation ==
            android.content.res.Configuration.ORIENTATION_LANDSCAPE
        return (if (landscape) 40f else 54f) * density
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = (rowHeightPx() * page.size + keyGap * 2).toInt() + bottomInset
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
        val rowHeight = rowHeightPx()
        val unit = (width - keyGap) / 10f
        val result = ArrayList<Cap>()
        page.forEachIndexed { r, row ->
            val rowUnits = row.sumOf { it.width.toDouble() }.toFloat()
            var x = keyGap / 2 + (10f - rowUnits) * unit / 2
            val top = keyGap + r * rowHeight
            for (key in row) {
                val w = key.width * unit
                result += Cap(key, RectF(x + keyGap / 2, top + keyGap / 2, x + w - keyGap / 2, top + rowHeight - keyGap / 2))
                x += w
            }
        }
        caps = result
        labelPaint.textSize = rowHeight * 0.40f
        hintPaint.textSize = rowHeight * 0.22f
    }

    override fun onDraw(canvas: Canvas) {
        canvas.drawColor(backgroundColor)
        for (cap in caps) {
            val key = cap.key
            val pressed = (0 until held.size()).any { held.valueAt(it) === cap }
            keyPaint.color = when {
                pressed -> color(R.color.kb_key_pressed)
                key.type == KeyType.ENTER -> color(R.color.july_blue)
                host.isActive(key) -> color(R.color.july_blue)
                key.type == KeyType.CHAR || key.type == KeyType.TEXT || key.type == KeyType.SPACE -> color(R.color.kb_key)
                else -> color(R.color.kb_key_special)
            }
            canvas.drawRoundRect(cap.rect, corner, corner, keyPaint)

            val onAccent = !pressed && (key.type == KeyType.ENTER || host.isActive(key))
            labelPaint.color = when {
                onAccent -> 0xFFFFFFFF.toInt()
                key.type == KeyType.MODE -> host.modeColor()
                else -> color(R.color.kb_text)
            }
            val label = host.label(key)
            labelPaint.textSize = if (key.type == KeyType.SPACE || label.length > 3) rowHeightPx() * 0.28f else rowHeightPx() * 0.40f
            val baseline = cap.rect.centerY() - (labelPaint.descent() + labelPaint.ascent()) / 2
            canvas.drawText(label, cap.rect.centerX(), baseline, labelPaint)

            host.hint(key)?.let {
                canvas.drawText(it, cap.rect.right - 4 * density, cap.rect.top + hintPaint.textSize + 1 * density, hintPaint)
            }
        }
    }

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
                if (cap.key.type == KeyType.DELETE) host.onKey(cap.key) // Delete acts on press
                postDelayed(longPressRunnable, 400)
                invalidate()
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                val id = event.getPointerId(event.actionIndex)
                val cap = held.get(id)
                held.remove(id)
                if (held.size() == 0) stopTimers()
                if (cap != null && !longPressDone) sendOnRelease(cap)
                invalidate()
            }
            MotionEvent.ACTION_CANCEL -> {
                held.clear()
                stopTimers()
                invalidate()
            }
        }
        return true
    }

    private fun sendOnRelease(cap: Cap) {
        if (cap.key.type != KeyType.DELETE) host.onKey(cap.key)
    }

    override fun onDetachedFromWindow() {
        stopTimers()
        super.onDetachedFromWindow()
    }
}
