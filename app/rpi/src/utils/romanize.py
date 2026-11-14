import re
import unicodedata
from anyascii import anyascii

try:
    import pykakasi
    _kks = pykakasi.kakasi()
except ImportError:
    _kks = None

try:
    from pypinyin import lazy_pinyin as _lazy_pinyin, Style as _PinyinStyle, load_single_dict
    # pypinyin falls back to a character's first listed reading when no phrase matches; for 长 that's
    # zhang ("grow"), but in lyrics it's far more often chang ("long"), e.g. 很长. Phrases like
    # 长大 (zhang da) are still matched by the phrase dictionary.
    load_single_dict({ord('长'): 'cháng,zhǎng', ord('長'): 'cháng,zhǎng'})
except ImportError:
    _lazy_pinyin = None

# Script ranges
_KANA = re.compile(r'[぀-ヿㇰ-ㇿｦ-ﾟ]')     # hiragana, katakana (incl. half-width)
_HAN = re.compile(r'[㐀-䶿一-鿿豈-﫿]+')     # CJK ideographs (hanzi / kanji)
_HANGUL = re.compile(r'[ᄀ-ᇿ㄰-㆏가-힯]+')  # Korean

LANG_ASCII = "ascii"      # nothing to romanize
LANG_JAPANESE = "ja"
LANG_CHINESE = "zh"
LANG_KOREAN = "ko"
LANG_OTHER = "other"      # Cyrillic, Greek, accented Latin, etc. -> generic transliteration

# Japanese almost always has a substantial share of kana alongside kanji; Chinese text occasionally
# borrows a stray の for style, so require kana to be a meaningful fraction of the CJK characters.
_JAPANESE_KANA_RATIO = 0.1


def detect_language(text: str) -> str:
    """Identify the language of `text` from the scripts it uses."""
    if not text or text.isascii():
        return LANG_ASCII

    kana = len(_KANA.findall(text))
    han = sum(len(m) for m in _HAN.findall(text))
    hangul = sum(len(m) for m in _HANGUL.findall(text))

    if kana and kana / (kana + han) >= _JAPANESE_KANA_RATIO:
        return LANG_JAPANESE
    if han:
        return LANG_CHINESE
    if hangul:
        return LANG_KOREAN
    return LANG_OTHER


def _romanize_japanese(text: str) -> str:
    if _kks is None:
        return text
    # keep ASCII segments (English words, punctuation) as-is, Hepburn for everything else
    return " ".join(
        item["orig"] if item["orig"].isascii() else item["hepburn"]
        for item in _kks.convert(text)
    )


def _romanize_chinese(text: str) -> str:
    # pypinyin converts whole runs so polyphonic characters get the right reading from
    # phrase context (银行 -> yin hang, 音乐 -> yin yue); anyascii reads each character in isolation
    def convert(m: re.Match) -> str:
        run = m.group()
        if _lazy_pinyin is None:
            syllables = [anyascii(c).lower() for c in run]
        else:
            # toneless pinyin; ü is kept here and folded to u by anyascii afterwards
            syllables = _lazy_pinyin(run, style=_PinyinStyle.NORMAL, v_to_u=True)
        return f" {' '.join(syllables)} "
    return _HAN.sub(convert, text)


def _romanize_korean(text: str) -> str:
    # anyascii runs Hangul syllables together in CamelCase ("BamPyeonJi"), so lowercase each word
    return _HANGUL.sub(lambda m: anyascii(m.group()).lower(), text)


def romanize(text: str, lang: str | None = None) -> str:
    """
    Convert non-ASCII text to a romanized ASCII approximation.

    The language is identified first (see detect_language), then the matching romanizer is used.
    Pass `lang` to override detection, e.g. with the language detected over a whole song's
    lyrics, so a kanji-only line in a Japanese song still gets Japanese readings.
    """
    if not text or text.isascii():
        return text

    if lang is None:
        lang = detect_language(text)

    # drop emoji/symbols (anyascii would turn them into shortcodes like ":hearts:")
    text = "".join(c for c in text if c.isascii() or unicodedata.category(c) != "So")

    # Hangul is unambiguous, so handle it regardless of the overall language (e.g. Korean line in a J-pop song)
    text = _romanize_korean(text)

    cjk = lang in (LANG_JAPANESE, LANG_CHINESE)
    if lang == LANG_JAPANESE:
        text = _romanize_japanese(text)
    elif lang == LANG_CHINESE:
        text = _romanize_chinese(text)

    # generic transliteration for whatever is left (Cyrillic, accents, curly quotes, leftover kanji...)
    text = anyascii(text)

    # the CJK converters insert spaces between words/syllables; tidy those up. Other text keeps
    # its original spacing (e.g. English lyrics with curly quotes or accents)
    if cjk:
        text = re.sub(r' +', ' ', text)
        text = re.sub(r' ([.,!?:;)\]])', r'\1', text)
    return text.strip()
