"""
webcam_yolo_pose.py

Real-time webcam inference for a custom Ultralytics YOLO hand-pose model
(trained on, or compatible with, the hand-keypoints dataset:
https://docs.ultralytics.com/datasets/pose/hand-keypoints/).

This is a single-stage model -- one forward pass gives you boxes + 21
keypoints per hand directly, no separate palm-detector stage needed.
The Ultralytics package handles preprocessing, NMS, and keypoint decoding
internally; this script just opens the webcam, feeds it frames, and draws
the results.

kpt_shape: [21, 3]  (x, y, visibility) -- same 21-point layout as MediaPipe:
  0       wrist
  1-4     thumb  (cmc, mcp, ip, tip)
  5-8     index  (mcp, pip, dip, tip)
  9-12    middle (mcp, pip, dip, tip)
  13-16   ring   (mcp, pip, dip, tip)
  17-20   pinky  (mcp, pip, dip, tip)

Install:
    pip install ultralytics opencv-python

Usage:
    python webcam_yolo_pose.py --model best.pt
    python webcam_yolo_pose.py --model best.onnx --camera 1 --conf 0.6
    python webcam_yolo_pose.py --model best.pt --device cuda

Press 'q' or ESC to quit.
"""

import argparse
import time

import cv2 as cv
from ultralytics import YOLO

# Same 21-point skeleton ordering as MediaPipe / hand-keypoints.yaml.
HAND_CONNECTIONS = [
    (0, 1), (1, 2), (2, 3), (3, 4),        # thumb
    (0, 5), (5, 6), (6, 7), (7, 8),        # index
    (0, 9), (9, 10), (10, 11), (11, 12),   # middle
    (0, 13), (13, 14), (14, 15), (15, 16),  # ring
    (0, 17), (17, 18), (18, 19), (19, 20),  # pinky
]


def parse_args():
    parser = argparse.ArgumentParser(description="Webcam inference for a custom YOLO hand-pose model")
    parser.add_argument("--model", type=str, default="best.pt",
                         help="Path to your trained model: .pt, .onnx, .engine, etc. (default: best.pt)")
    parser.add_argument("--camera", type=int, default=0,
                         help="Webcam device index (default: 0)")
    parser.add_argument("--conf", type=float, default=0.5,
                         help="Detection confidence threshold (default: 0.5)")
    parser.add_argument("--iou", type=float, default=0.45,
                         help="NMS IoU threshold (default: 0.45)")
    parser.add_argument("--max-hands", type=int, default=2,
                         help="Max hands to keep per frame (default: 2)")
    parser.add_argument("--imgsz", type=int, default=640,
                         help="Inference image size, must match/be compatible with training (default: 640)")
    parser.add_argument("--device", type=str, default=None,
                         help="'cpu', 'cuda', 'cuda:0', or 'mps'. Default: let Ultralytics auto-pick.")
    parser.add_argument("--kpt-conf", type=float, default=0.5,
                         help="Per-keypoint visibility threshold below which a point is hidden (default: 0.5)")
    parser.add_argument("--mirror", action="store_true", default=True,
                         help="Flip the camera horizontally (selfie view). Default on.")
    parser.add_argument("--no-mirror", dest="mirror", action="store_false")
    return parser.parse_args()


def draw(frame, result, kpt_conf_thresh):
    """Draw boxes + hand skeleton from one Ultralytics pose Result onto frame, in place."""
    boxes = result.boxes
    kpts = result.keypoints
    if boxes is None or kpts is None or len(boxes) == 0:
        return frame

    xyxy = boxes.xyxy.cpu().numpy()
    confs = boxes.conf.cpu().numpy()
    kp_xy = kpts.xy.cpu().numpy()  # (N, 21, 2)
    kp_conf = kpts.conf.cpu().numpy() if kpts.conf is not None else None  # (N, 21) or None

    for i in range(len(xyxy)):
        x1, y1, x2, y2 = xyxy[i].astype(int)
        cv.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv.putText(frame, f"hand {confs[i]:.2f}", (x1, max(y1 - 8, 0)),
                   cv.FONT_HERSHEY_DUPLEX, 0.5, (0, 255, 0), 1)

        pts = kp_xy[i]
        visible = (kp_conf[i] >= kpt_conf_thresh) if kp_conf is not None else [True] * len(pts)

        # for a, b in HAND_CONNECTIONS:
        #     if visible[a] and visible[b]:
        #         cv.line(frame, tuple(pts[a].astype(int)), tuple(pts[b].astype(int)), (255, 255, 255), 2)
        for j, p in enumerate(pts):
            if visible[j]:
                cv.circle(frame, tuple(p.astype(int)), 4, (0, 0, 255), -1)

    return frame


def main():
    args = parse_args()

    print(f"Loading model: {args.model}")
    model = YOLO(args.model)

    cap = cv.VideoCapture(args.camera)
    if not cap.isOpened():
        raise RuntimeError(f"Could not open camera index {args.camera}")

    print("Webcam started. Press 'q' or ESC to quit.")
    prev_time = time.time()
    fps = 0.0

    while True:
        ok, frame = cap.read()
        if not ok:
            print("No frame grabbed, stopping.")
            break

        if args.mirror:
            frame = cv.flip(frame, 1)

        results = model.predict(
            frame,
            conf=args.conf,
            iou=args.iou,
            imgsz=args.imgsz,
            max_det=args.max_hands,
            device=args.device,
            verbose=False,
        )
        frame = draw(frame, results[0], args.kpt_conf)

        now = time.time()
        fps = 0.9 * fps + 0.1 * (1.0 / max(now - prev_time, 1e-6))
        prev_time = now
        cv.putText(frame, f"FPS: {fps:.1f}", (8, 22), cv.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        cv.imshow("YOLO Hand Pose", frame)
        key = cv.waitKey(1) & 0xFF
        if key == ord('q') or key == 27:  # 27 = ESC
            break

    cap.release()
    cv.destroyAllWindows()


if __name__ == "__main__":
    main()