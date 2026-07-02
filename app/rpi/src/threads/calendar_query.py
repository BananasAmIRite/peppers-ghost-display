from google.oauth2 import service_account
from googleapiclient.discovery import build

from datetime import datetime, timezone
import struct
import lib.comms.comms as comms
import os
from dotenv import load_dotenv

SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]
class CalendarQuery:
    def __init__(self, mgr, service, calendar_ids):
        self.mgr = mgr
        self.service = service
        self.calendar_ids = calendar_ids

    def query(self):
        # compute and transmit current timestamp
        # current UTC timestamp (seconds)
        now_utc_dt = datetime.now(timezone.utc)
        now_utc_ts = int(now_utc_dt.timestamp())

        # timezone offset (seconds)
        now_local_dt = datetime.now().astimezone()
        tz_offset_seconds = int(now_local_dt.utcoffset().total_seconds())

        timestamp_payload = struct.pack("<Qi", now_utc_ts, tz_offset_seconds)
        self.mgr.send_uart_message(comms.CAL_SET_DATE_TIME, timestamp_payload)


        # calendar query
        now = datetime.now(timezone.utc).isoformat()
        all_events = []

        for calendar_id in self.calendar_ids:
            result = self.service.events().list(
                calendarId=calendar_id,
                timeMin=now,
                maxResults=10,
                singleEvents=True,
                orderBy="startTime"
            ).execute()

            all_events.extend(result.get("items", []))

        def start_time(event):
            # Handles both timed and all-day events
            return event["start"].get("dateTime", event["start"].get("date"))


        # combine all events
        all_events.sort(key=start_time)

        # compile into json
        events_json = []
        for event in all_events:
            start_raw = event["start"].get("dateTime", event["start"].get("date"))
            end_raw = event["end"].get("dateTime", event["end"].get("date"))

            # Parse timestamps
            if "T" in start_raw:
                start_dt = datetime.fromisoformat(start_raw.replace("Z", "+00:00"))
                start_ts = int(start_dt.timestamp())
            else:
                # all-day event (date only)
                start_dt = datetime.fromisoformat(start_raw)
                start_ts = int(start_dt.replace(tzinfo=timezone.utc).timestamp())

            if "T" in end_raw:
                end_dt = datetime.fromisoformat(end_raw.replace("Z", "+00:00"))
                end_ts = int(end_dt.timestamp())
            else:
                end_dt = datetime.fromisoformat(end_raw)
                end_ts = int(end_dt.replace(tzinfo=timezone.utc).timestamp())

            event_obj = {
                "name": event.get("summary", "Untitled Event"),
                "timestampStart": start_ts,
                "timestampEnd": end_ts,
            }
            events_json.append(event_obj)

        spi_payload_bytes = str(events_json).encode("utf-8")
        payload = (
            struct.pack(
                "<I",
                len(spi_payload_bytes)
            )
            + spi_payload_bytes
        )

        print(payload)

        self.mgr.send_spi_message(comms.CAL_SET_EVENTS, payload)


        return all_events

def get_service():
    SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]

    creds = service_account.Credentials.from_service_account_file(
        "../service_account.json",
        scopes=SCOPES
    )

    return build("calendar", "v3", credentials=creds)


if __name__ == "__main__":
    load_dotenv()
    service = get_service()

    calendar = CalendarQuery(None, service, os.getenv("CALENDAR_IDS").split(","))

    calendar.query()