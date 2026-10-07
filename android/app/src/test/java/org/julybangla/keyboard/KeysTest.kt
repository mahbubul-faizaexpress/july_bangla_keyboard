package org.julybangla.keyboard

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class KeysTest {
    @Test
    fun scanCodesMatchThePcKeyboard() {
        assertEquals(0x02, ScanCodes.forChar('1'))
        assertEquals(0x0B, ScanCodes.forChar('0'))
        assertEquals(0x10, ScanCodes.forChar('q'))
        assertEquals(0x19, ScanCodes.forChar('p'))
        assertEquals(0x1E, ScanCodes.forChar('a'))
        assertEquals(0x22, ScanCodes.forChar('g'))
        assertEquals(0x26, ScanCodes.forChar('l'))
        assertEquals(0x2B, ScanCodes.forChar('\\'))
        assertEquals(0x2C, ScanCodes.forChar('z'))
        assertEquals(0x32, ScanCodes.forChar('m'))
        assertEquals(0x34, ScanCodes.forChar('.'))
        assertEquals(0, ScanCodes.forChar(' '))
    }

    @Test
    fun shiftedCharacters() {
        assertEquals('!', ScanCodes.shifted('1'))
        assertEquals('$', ScanCodes.shifted('4'))
        assertEquals('|', ScanCodes.shifted('\\'))
        assertEquals('Q', ScanCodes.shifted('q'))
    }

    @Test
    fun pagesFitTenUnits() {
        // Every row is ten units wide, except the iPhone-style centred middle row (a-l).
        for (page in listOf(Pages.banglaLetters, Pages.symbols, Pages.moreSymbols)) {
            for (row in page) assertEquals(10f, row.sumOf { it.width.toDouble() }.toFloat(), 0.001f)
        }
        val english = Pages.englishLetters.map { row -> row.sumOf { it.width.toDouble() }.toFloat() }
        assertEquals(listOf(10f, 9f, 10f, 10f), english)
    }

    @Test
    fun banglaPageHasEveryBijoyLetterKey() {
        val keys = Pages.banglaLetters.flatten().filter { it.type == KeyType.CHAR }
        for (c in "qwertyuiopasdfghjkl\\zxcvbnm") assertTrue(keys.any { it.latin == c && !it.forceShift })
        assertTrue(keys.any { it.latin == '7' && it.forceShift }) // ঁ
    }

    @Test
    fun decodesEngineResults() {
        val r = EditResult.decode("1কি\u0000ক")
        assertTrue(r.eaten)
        assertEquals("কি", r.commit)
        assertEquals("ক", r.composition)

        val empty = EditResult.decode("0\u0000")
        assertFalse(empty.eaten)
        assertEquals("", empty.commit)
        assertEquals("", empty.composition)
    }
}
