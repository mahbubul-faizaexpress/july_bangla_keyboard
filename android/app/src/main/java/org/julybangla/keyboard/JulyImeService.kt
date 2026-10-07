package org.julybangla.keyboard

import android.content.Context
import android.inputmethodservice.InputMethodService
import android.text.InputType
import android.view.KeyEvent
import android.view.View
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager

/**
 * The keyboard service. Typing follows the Windows text service exactly: the shared
 * engine decides the text, and this class only moves it into the editor as
 * composing text (`setComposingText`) and committed text (`commitText`).
 *
 * Privacy: nothing typed is stored, logged or sent anywhere; the only saved setting is
 * the input mode.
 */
class JulyImeService : InputMethodService(), KeyboardView.Host {

    private enum class Mode { ENGLISH, UNICODE, CLASSIC }
    private enum class Shift { OFF, ONCE, LOCKED }

    private var mode = Mode.UNICODE
    private var shift = Shift.OFF
    private var symbols = false
    private var lastShiftTap = 0L
    // Number, phone and password fields always get plain Latin characters.
    private var latinOnly = false

    private val unicodeComposer by lazy { Composer(classic = false) }
    private val classicComposer by lazy { Composer(classic = true) }
    private val composer get() = if (mode == Mode.CLASSIC) classicComposer else unicodeComposer

    // True while the editor shows our composing text.
    private var composingShown = false
    // Hardware keys whose key-down we consumed, so their key-up is consumed too.
    private val eatenKeys = HashSet<Int>()

    private var keyboard: KeyboardView? = null
    private val labels = HashMap<Int, String>()

    override fun onCreate() {
        super.onCreate()
        val saved = getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(KEY_MODE, null)
        mode = Mode.entries.firstOrNull { it.name == saved } ?: Mode.UNICODE
    }

    override fun onDestroy() {
        unicodeComposer.close()
        classicComposer.close()
        super.onDestroy()
    }

    override fun onCreateInputView(): View = KeyboardView(this, this).also { keyboard = it }

    override fun onStartInputView(info: EditorInfo, restarting: Boolean) {
        super.onStartInputView(info, restarting)
        dropComposition()
        shift = Shift.OFF
        symbols = isNumberField(info)
        latinOnly = symbols || isPasswordField(info)
        keyboard?.showPage(if (symbols) Pages.symbols else Pages.letters)
    }

    override fun onFinishInput() {
        endComposition()
        super.onFinishInput()
    }

    // The user moved the cursor or selected text: the open syllable is finished where it
    // is. The text is never rewritten (the lesson from the Windows text service).
    override fun onUpdateSelection(
        oldSelStart: Int, oldSelEnd: Int, newSelStart: Int, newSelEnd: Int,
        candidatesStart: Int, candidatesEnd: Int,
    ) {
        super.onUpdateSelection(oldSelStart, oldSelEnd, newSelStart, newSelEnd, candidatesStart, candidatesEnd)
        if (composingShown && candidatesStart >= 0 &&
            (newSelStart != candidatesEnd || newSelEnd != candidatesEnd)
        ) {
            endComposition()
        }
    }

    // --- Keyboard host -----------------------------------------------------------------

    override fun label(key: Key): String = when (key.type) {
        KeyType.CHAR -> charLabel(key, shiftOn())
        KeyType.TEXT -> key.text
        KeyType.SHIFT -> if (shift == Shift.LOCKED) "⇪" else "⇧"
        KeyType.DELETE -> "⌫"
        KeyType.MODE -> when (mode) {
            Mode.ENGLISH -> "EN"
            Mode.UNICODE -> "বাং"
            Mode.CLASSIC -> "ক্লা"
        }
        KeyType.SYMBOLS -> when {
            symbols && mode == Mode.ENGLISH -> "ABC"
            symbols -> "কখগ"
            mode == Mode.ENGLISH -> "?123"
            else -> "?১২৩"
        }
        KeyType.SPACE -> when (mode) {
            Mode.ENGLISH -> "English"
            Mode.UNICODE -> "জুলাই বাংলা"
            Mode.CLASSIC -> "ক্লাসিক"
        }
        KeyType.ENTER -> "↵"
    }

    override fun hint(key: Key): String? {
        if (key.type != KeyType.CHAR || mode == Mode.ENGLISH || latinOnly || symbols) return null
        val other = charLabel(key, !shiftOn())
        return other.takeIf { it != charLabel(key, shiftOn()) }
    }

    override fun isActive(key: Key) = key.type == KeyType.SHIFT && shift != Shift.OFF

    override fun onKey(key: Key) {
        when (key.type) {
            KeyType.CHAR -> typeChar(key)
            KeyType.TEXT -> {
                endComposition()
                currentInputConnection?.commitText(key.text, 1)
            }
            KeyType.SPACE -> {
                endComposition()
                currentInputConnection?.commitText(" ", 1)
            }
            KeyType.ENTER -> pressEnter()
            KeyType.DELETE -> pressDelete()
            KeyType.SHIFT -> {
                val now = System.currentTimeMillis()
                shift = when {
                    shift == Shift.ONCE && now - lastShiftTap < 400 -> Shift.LOCKED
                    shift == Shift.OFF -> Shift.ONCE
                    else -> Shift.OFF
                }
                lastShiftTap = now
            }
            KeyType.MODE -> setMode(nextMode())
            KeyType.SYMBOLS -> {
                symbols = !symbols
                keyboard?.showPage(if (symbols) Pages.symbols else Pages.letters)
            }
        }
        keyboard?.refresh()
    }

    override fun onLongPress(key: Key): Boolean {
        if (key.type != KeyType.MODE) return false
        endComposition()
        getSystemService(InputMethodManager::class.java).showInputMethodPicker()
        return true
    }

    // --- Typing ------------------------------------------------------------------------

    private fun typeChar(key: Key) {
        val shifted = shiftOn()
        if (shift == Shift.ONCE) shift = Shift.OFF
        val latin = if (shifted) ScanCodes.shifted(key.latin) else key.latin
        if (mode == Mode.ENGLISH || latinOnly || key.scan == 0) {
            endComposition()
            currentInputConnection?.commitText(latin.toString(), 1)
            return
        }
        val result = composer.pressKey(key.scan, shifted)
        apply(result)
        // Keys Bijoy does not map (such as , and .) type their own character, after any
        // text the engine committed.
        if (!result.eaten) currentInputConnection?.commitText(latin.toString(), 1)
    }

    private fun pressDelete() {
        if (composingShown) {
            val result = composer.backspace()
            if (result.eaten) {
                apply(result)
                return
            }
        }
        endComposition()
        sendDownUpKeyEvents(KeyEvent.KEYCODE_DEL)
    }

    private fun pressEnter() {
        endComposition()
        val info = currentInputEditorInfo
        val action = info?.imeOptions?.and(EditorInfo.IME_MASK_ACTION) ?: EditorInfo.IME_ACTION_NONE
        val noAction = info == null || (info.imeOptions and EditorInfo.IME_FLAG_NO_ENTER_ACTION) != 0 ||
            action == EditorInfo.IME_ACTION_NONE || action == EditorInfo.IME_ACTION_UNSPECIFIED
        if (noAction) sendKeyChar('\n') else currentInputConnection?.performEditorAction(action)
    }

    /** Moves an engine result into the editor (see [EditResult]). */
    private fun apply(result: EditResult) {
        val ic = currentInputConnection ?: return
        if (result.commit.isEmpty() && result.composition.isEmpty() && !composingShown) return
        ic.beginBatchEdit()
        if (result.commit.isNotEmpty()) {
            ic.commitText(result.commit, 1) // replaces the composing text
            composingShown = false
        }
        if (result.composition.isNotEmpty()) {
            ic.setComposingText(result.composition, 1)
            composingShown = true
        } else if (composingShown) {
            ic.commitText("", 1) // the syllable was deleted key by key
            composingShown = false
        }
        ic.endBatchEdit()
    }

    /** Leaves the open syllable in the text as it is and starts fresh. */
    private fun endComposition() {
        if (composingShown) currentInputConnection?.finishComposingText()
        dropComposition()
    }

    private fun dropComposition() {
        composingShown = false
        unicodeComposer.reset()
        classicComposer.reset()
    }

    // --- Hardware keyboard -------------------------------------------------------------

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (event.isCtrlPressed && event.isAltPressed && keyCode == KeyEvent.KEYCODE_B) {
            setMode(nextMode())
            eatenKeys += keyCode
            return true
        }
        if (mode == Mode.ENGLISH || latinOnly || event.isCtrlPressed || event.isAltPressed || event.isMetaPressed) {
            endComposition()
            return super.onKeyDown(keyCode, event)
        }
        if (keyCode == KeyEvent.KEYCODE_DEL) {
            val result = composer.backspace()
            if (!result.eaten) return super.onKeyDown(keyCode, event)
            apply(result)
            eatenKeys += keyCode
            return true
        }
        val scan = ScanCodes.forKeyCode(keyCode)
        if (scan == 0) {
            endComposition()
            return super.onKeyDown(keyCode, event)
        }
        val result = composer.pressKey(scan, event.isShiftPressed)
        apply(result)
        if (!result.eaten) return super.onKeyDown(keyCode, event)
        eatenKeys += keyCode
        return true
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean =
        eatenKeys.remove(keyCode) || super.onKeyUp(keyCode, event)

    // --- Modes -------------------------------------------------------------------------

    private fun nextMode() = when (mode) {
        Mode.ENGLISH -> Mode.UNICODE
        Mode.UNICODE -> Mode.CLASSIC
        Mode.CLASSIC -> Mode.ENGLISH
    }

    private fun setMode(newMode: Mode) {
        endComposition()
        mode = newMode
        getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putString(KEY_MODE, mode.name).apply()
        keyboard?.refresh()
    }

    private fun shiftOn() = shift != Shift.OFF

    private fun charLabel(key: Key, shifted: Boolean): String {
        val latin = if (shifted) ScanCodes.shifted(key.latin) else key.latin
        if (mode == Mode.ENGLISH || latinOnly) return latin.toString()
        val id = key.scan * 2 + if (shifted) 1 else 0
        return labels.getOrPut(id) { NativeEngine.keyLabel(key.scan, shifted) }.ifEmpty { latin.toString() }
    }

    private fun isNumberField(info: EditorInfo): Boolean = when (info.inputType and InputType.TYPE_MASK_CLASS) {
        InputType.TYPE_CLASS_NUMBER, InputType.TYPE_CLASS_PHONE, InputType.TYPE_CLASS_DATETIME -> true
        else -> false
    }

    private fun isPasswordField(info: EditorInfo): Boolean {
        val variation = info.inputType and InputType.TYPE_MASK_VARIATION
        return when (info.inputType and InputType.TYPE_MASK_CLASS) {
            InputType.TYPE_CLASS_TEXT -> variation == InputType.TYPE_TEXT_VARIATION_PASSWORD ||
                variation == InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD ||
                variation == InputType.TYPE_TEXT_VARIATION_WEB_PASSWORD
            InputType.TYPE_CLASS_NUMBER -> variation == InputType.TYPE_NUMBER_VARIATION_PASSWORD
            else -> false
        }
    }

    private companion object {
        const val PREFS = "july"
        const val KEY_MODE = "mode"
    }
}
