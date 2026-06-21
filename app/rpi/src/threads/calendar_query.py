from googleapiclient.discovery import build
from datetime import datetime, timezone

class CalendarQuery:
    def __init__(self, service):
        self.service = service

    def query(self):
        now = datetime.now(timezone.utc).isoformat()

        result = self.service.events().list(
            calendarId="primary",
            timeMin=now,
            maxResults=10,
            singleEvents=True,
            orderBy="startTime"
        ).execute()

        return result.get("items", [])