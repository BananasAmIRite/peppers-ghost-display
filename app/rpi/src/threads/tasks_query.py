
import os
import requests
from dotenv import load_dotenv
from enum import Enum
from concurrent.futures import ThreadPoolExecutor
import struct
import lib.comms.comms as comms


class TaskStatus(Enum):
    NOT_STARTED = 0
    IN_PROGRESS = 1
    WAITING = 2
    DONE = 3

def status_str_to_id(status):
    if status == "Not started":
        return 0
    elif status == "In progress" or status == "To be checked":
        return 1
    elif status == "Waiting":
        return 2
    elif status == "Done" or status == "Archived":
        return 3
    else:
        print(f"Couldn't parse status {status}")
        return 0 # should not happen :P


class TasksQuery:
    def __init__(self, mgr, secret, ntn_view_id):
        self.mgr = mgr
        self.secret = secret
        self.ntn_view_id = ntn_view_id

    def query(self):
        url = f"https://api.notion.com/v1/views/{self.ntn_view_id}/queries"

        payload = { "page_size": 50 }
        headers = {
            "Notion-Version": "2026-03-11",
            "Authorization": f"Bearer {self.secret}",
            "Content-Type": "application/json"
        }

        response = requests.post(url, 
                                json=payload, 
                                headers=headers)
        
        json = response.json()

        if not json["results"]: return
        with ThreadPoolExecutor(max_workers=5) as executor:
            results = executor.map(self.query_page, map(lambda a: a["id"], json["results"]))
            # prune any "None"'s
            spi_payload_json = str([a for a in list(results) if a is not None])
            spi_payload_bytes = spi_payload_json.encode("utf-8")
            payload = (
                struct.pack(
                    "<I",
                    len(spi_payload_bytes)
                )
                + spi_payload_bytes
            )

            self.mgr.send_spi_message(comms.TASKS_ADD, payload)
        
    def query_page(self, page_id):
        url = f"https://api.notion.com/v1/pages/{page_id}"
        headers = {
            "Notion-Version": "2026-03-11",
            "Authorization": f"Bearer {self.secret}",
            "Content-Type": "application/json"
        }

        response = requests.get(url, headers=headers)

        json = response.json()

        print(json)

        if not json["properties"]:
            return None # some issues occurred idk

        # status
        status_str = json["properties"]["Status"]['status']['name']
        status_id = status_str_to_id(status_str)

        # name
        name = json["properties"]["Name"]["title"][0]["text"]["content"]

        # id
        id = json["id"]

        return {"name": name, "status": status_id, "id": id, "action": 'UPDATE'}
    

if __name__ == '__main__':
    load_dotenv()
    secret = os.getenv("NOTION_CLIENT_SECRET")
    notion_view_id = os.getenv("NOTION_VIEW_ID")
    tasksQuerier = TasksQuery(None, secret, notion_view_id)
    tasksQuerier.query()
