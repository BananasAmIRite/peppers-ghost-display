import re
import unicodedata
from anyascii import anyascii

try:
    import pykakasi
    _kks = pykakasi.kakasi()
except ImportError:
    _kks = None

try:
    from pypinyin import lazy_pinyin as _lazy_pinyin, Style as _PinyinStyle
except ImportError:
    _lazy_pinyin = None

# Hiragana, katakana (incl. half-width) -> text is Japanese, so kanji should use Japanese readings
_JAPANESE_KANA = re.compile(r'[぀-ヿㇰ-ㇿｦ-ﾟ]')
_HAN = re.compile(r'[㐀-䶿一-鿿豈-﫿]+')
_HANGUL = re.compile(r'[ᄀ-ᇿ㄰-㆏가-힯]+')


def _han_to_pinyin(m: re.Match) -> str:
    run = m.group()
    if _lazy_pinyin is None:
        return " ".join(anyascii(c).lower() for c in run).join("  ")
    # toneless pinyin; ü is kept here and folded to u by anyascii afterwards
    return " ".join(_lazy_pinyin(run, style=_PinyinStyle.NORMAL, v_to_u=True)).join("  ")


def romanize(text: str) -> str:
    """Convert any non-ASCII text to a romanized ASCII approximation."""
    if not text or text.isascii():
        return text

    # drop emoji/symbols (anyascii would turn them into shortcodes like ":hearts:")
    text = "".join(c for c in text if c.isascii() or unicodedata.category(c) != "So")

    # only text that went through the CJK converters (which insert spaces) gets spacing cleanup,
    # so other lines (e.g. English with curly quotes/accents) keep their original spacing
    needs_cleanup = bool(_HAN.search(text) or (_kks is not None and _JAPANESE_KANA.search(text)))

    if _kks is not None and _JAPANESE_KANA.search(text):
        text = " ".join(
            item["hepburn"] if not item["orig"].isascii() else item["orig"]
            for item in _kks.convert(text)
        )

    # Chinese: pypinyin converts whole runs so polyphonic characters get the right reading from
    # phrase context (银行 -> yin hang, 音乐 -> yin yue); anyascii reads each character in isolation
    text = _HAN.sub(_han_to_pinyin, text)
    # anyascii runs Hangul syllables together in CamelCase, so lowercase each word
    text = _HANGUL.sub(lambda m: anyascii(m.group()).lower(), text)

    text = anyascii(text)
    if needs_cleanup:
        text = re.sub(r' +', ' ', text)
        text = re.sub(r' ([.,!?:;)\]])', r'\1', text)
    return text.strip()
