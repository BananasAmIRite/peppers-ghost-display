import re

def parse_lrc(lrc_string):
    """
    Parses an LRC string, rounding timestamps to the nearest second.
    Merges lyrics that land on the same second.
    """
    lyrics_by_second = {}
    # Regex captures [MM:SS.xx] or [MM:SS]
    timestamp_pattern = re.compile(r'\[(\d{2}):(\d{2}(?:\.\d{2})?)\]')
    
    for line in lrc_string.split('\n'):
        matches = timestamp_pattern.findall(line)
        text = timestamp_pattern.sub('', line).strip()
        
        # Skip empty lines that don't have lyrics
        if not text:
            continue
            
        for minutes, seconds in matches:
            total_seconds = (int(minutes) * 60) + float(seconds)
            nearest_second = round(total_seconds)
            
            # Append if second exists, otherwise create a new entry
            if nearest_second in lyrics_by_second:
                lyrics_by_second[nearest_second] += f" {text}"
            else:
                lyrics_by_second[nearest_second] = text
                
    # Format into a sorted list of dictionaries
    sorted_lyrics = [
        {"timestamp": sec, "text": lyrics_by_second[sec]} 
        for sec in sorted(lyrics_by_second.keys())
    ]
    
    return sorted_lyrics
