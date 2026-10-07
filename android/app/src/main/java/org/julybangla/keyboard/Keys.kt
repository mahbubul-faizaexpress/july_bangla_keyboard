package org.julybangla.keyboard

import android.view.KeyEvent

/**
 * The engine identifies keys by PC/AT set-1 scan code, as on Windows, so the on-screen
 * keys and hardware keys are translated to the scan code of the matching US key.
 */
internal object ScanCodes {
    private const val ROW_DIGITS = "1234567890-="
    private const val ROW_TOP = "qwertyuiop[]"
    private const val ROW_HOME = "asdfghjkl;'`"
    private const val ROW_BOTTOM = "zxcvbnm,./"

    /** Scan code for an unshifted US key character, or 0. */
    fun forChar(c: Char): Int = when (c) {
        in ROW_DIGITS -> 0x02 + ROW_DIGITS.indexOf(c)
        in ROW_TOP -> 0x10 + ROW_TOP.indexOf(c)
        in ROW_HOME -> 0x1E + ROW_HOME.indexOf(c)
        '\\' -> 0x2B
        in ROW_BOTTOM -> 0x2C + ROW_BOTTOM.indexOf(c)
        else -> 0
    }

    /** Scan code for a hardware key, or 0 for keys the engine never handles. */
    fun forKeyCode(keyCode: Int): Int = when (keyCode) {
        in KeyEvent.KEYCODE_A..KeyEvent.KEYCODE_Z -> forChar('a' + (keyCode - KeyEvent.KEYCODE_A))
        KeyEvent.KEYCODE_0 -> forChar('0')
        in KeyEvent.KEYCODE_1..KeyEvent.KEYCODE_9 -> forChar('1' + (keyCode - KeyEvent.KEYCODE_1))
        KeyEvent.KEYCODE_MINUS -> forChar('-')
        KeyEvent.KEYCODE_EQUALS -> forChar('=')
        KeyEvent.KEYCODE_LEFT_BRACKET -> forChar('[')
        KeyEvent.KEYCODE_RIGHT_BRACKET -> forChar(']')
        KeyEvent.KEYCODE_SEMICOLON -> forChar(';')
        KeyEvent.KEYCODE_APOSTROPHE -> forChar('\'')
        KeyEvent.KEYCODE_GRAVE -> forChar('`')
        KeyEvent.KEYCODE_BACKSLASH -> forChar('\\')
        KeyEvent.KEYCODE_COMMA -> forChar(',')
        KeyEvent.KEYCODE_PERIOD -> forChar('.')
        KeyEvent.KEYCODE_SLASH -> forChar('/')
        else -> 0
    }

    private const val PLAIN = "1234567890-=[];'`\\,./"
    private const val SHIFTED = "!@#$%^&*()_+{}:\"~|<>?"

    /** The character a US key gives with Shift. */
    fun shifted(c: Char): Char = PLAIN.indexOf(c).let { if (it >= 0) SHIFTED[it] else c.uppercaseChar() }
}

enum class KeyType { CHAR, TEXT, SHIFT, DELETE, MODE, SYMBOLS, MORE, SPACE, ENTER }

/**
 * One on-screen key. [width] is in key units; each row is 10 units wide. A CHAR key with
 * [forceShift] always types its Shift character (the ঁ key, Bijoy Shift+7).
 */
class Key internal constructor(
    val type: KeyType,
    val latin: Char = ' ',
    val text: String = "",
    val width: Float = 1f,
    val forceShift: Boolean = false,
) {
    val scan: Int = if (type == KeyType.CHAR) ScanCodes.forChar(latin) else 0
}

/**
 * On-screen pages, laid out like the iPhone keyboard: four rows, digits and signs on the
 * "123" and "#+=" pages. The Bangla letter page keeps every Bijoy letter key in its PC
 * position; ৎ/ঃ (the \ key) ends the middle row and ঁ (Shift+7) sits beside m.
 */
internal object Pages {
    private fun chars(row: String) = row.map { Key(KeyType.CHAR, latin = it) }
    private fun texts(row: String, width: Float = 1f) = row.split(' ').map { Key(KeyType.TEXT, text = it, width = width) }

    private val bottomRow = listOf(
        Key(KeyType.SYMBOLS, width = 1.25f),
        Key(KeyType.MODE, width = 1.25f),
        Key(KeyType.SPACE, width = 5f),
        Key(KeyType.ENTER, width = 2.5f),
    )

    val englishLetters: List<List<Key>> = listOf(
        chars("qwertyuiop"),
        chars("asdfghjkl"),
        listOf(Key(KeyType.SHIFT, width = 1.5f)) + chars("zxcvbnm") + Key(KeyType.DELETE, width = 1.5f),
        bottomRow,
    )

    val banglaLetters: List<List<Key>> = listOf(
        chars("qwertyuiop"),
        chars("asdfghjkl\\"),
        listOf(Key(KeyType.SHIFT, width = 1f)) + chars("zxcvbnm") +
            Key(KeyType.CHAR, latin = '7', forceShift = true) + Key(KeyType.DELETE, width = 1f),
        bottomRow,
    )

    private val punctuationRow = texts(". , ? ! ' ।", width = 7f / 6f)

    val symbols: List<List<Key>> = listOf(
        chars("1234567890"),
        texts("- / : ; ( ) ৳ & @ \""),
        listOf(Key(KeyType.MORE, width = 1.5f)) + punctuationRow + Key(KeyType.DELETE, width = 1.5f),
        bottomRow,
    )

    val moreSymbols: List<List<Key>> = listOf(
        texts("[ ] { } # % ^ * + ="),
        texts("_ \\ | ~ < > $ ` ‘ ’"),
        listOf(Key(KeyType.MORE, width = 1.5f)) + punctuationRow + Key(KeyType.DELETE, width = 1.5f),
        bottomRow,
    )
}
