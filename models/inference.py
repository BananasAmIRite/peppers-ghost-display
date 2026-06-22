from picamera2 import Picamera2
from picamera2.devices import IMX500
import numpy as np
import cv2
import time

MODEL_PATH = "/usr/share/imx500-models/yolov8n-hand.rpk"

imx = IMX500(MODEL_PATH)

picam2 = Picamera2()

# Use a simple RGB config (important for imshow)
#config = picam2.create_preview_configuration(
#    main={"size": (640, 480), "format": "RGB888"}
#)

#picam2.configure(config)
picam2.start()

print("Running IMX500 pose + imshow... Ctrl+C to stop")

COCO = [
    "Nose","L Eye","R Eye","L Ear","R Ear",
    "L Shoulder","R Shoulder","L Elbow","R Elbow",
    "L Wrist","R Wrist","L Hip","R Hip",
    "L Knee","R Knee","L Ankle","R Ankle"
]

try:
    while True:
#        frame = picam2.capture_array()
#        metadata = picam2.capture_metadata()
#         print(metadata.keys())
        request = picam2.capture_request()

        frame = request.make_array("main")
        metadata = request.get_metadata()

        print(frame.shape)

        request.release()
        if "CnnOutputTensor" not in metadata:
            continue

        detections = imx.get_outputs(metadata)

        if detections:
            print("detection")
  #           break
            boxes = detections[0]
            scores = detections[1]
            keypoints = detections[3]

            best = int(np.argmax(scores))
            if scores[best] > 0.2:
                kps = keypoints[best].reshape(21, 3)
                # print(x, y)
                # draw skeleton
                for i, (x, y, c) in enumerate(kps):
                    if c > 0.25:
                        x *= 2
                        y *= 1.5
                        print(x, y)
                        cv2.circle(frame, (int(x), int(y)), 3, (0,255,0), -1)
                        cv2.putText(frame, str(i), (int(x)+2, int(y)+2),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.4,
                                    (0,255,0), 1)

        cv2.imshow("IMX500 Pose", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

        time.sleep(0.01)

except KeyboardInterrupt:
    print("Stopping...")

finally:
    picam2.stop()
    cv2.destroyAllWindows()
