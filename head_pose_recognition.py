"""
头部姿态识别 - 游戏视角控制
使用MediaPipe 0.10.x API进行面部网格检测
按 'q' 键退出，按 'r' 键将当前头部姿态设置为参考（中性）姿态
"""

import cv2
import mediapipe as mp
import numpy as np
import time
import sys
import os

# 设置控制台编码为UTF-8
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', line_buffering=True)
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', line_buffering=True)

# MediaPipe 0.10.x 的新API导入
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# FPS计算
prev_time = 0

# 头部姿态历史跟踪器
head_history = None

# 参考姿态（中性姿态）
reference_euler = None
current_euler = None

# 方向检测阈值（角度，单位：度）
pitch_threshold = 10.0  # 上/下
yaw_threshold = 10.0    # 左/右
roll_threshold = 10.0   # 倾斜

# 打开摄像头
print("[INFO] 正在打开摄像头...")

# 尝试不同的摄像头索引
cam_index = 1
cap = cv2.VideoCapture(cam_index, cv2.CAP_DSHOW) if sys.platform == 'win32' else cv2.VideoCapture(cam_index)

if not cap.isOpened():
    print(f"[WARN] 索引 {cam_index} 打开失败，尝试索引1...")
    cam_index = 1
    cap = cv2.VideoCapture(cam_index, cv2.CAP_DSHOW) if sys.platform == 'win32' else cv2.VideoCapture(cam_index)
    
    if not cap.isOpened():
        print("[ERROR] 错误：无法打开摄像头")
        print("[INFO] 请检查：")
        print("  1. 摄像头是否已连接")
        print("  2. 摄像头驱动是否正常")
        print("  3. 是否有其他程序占用摄像头")
        exit()

print(f"[SUCCESS] 摄像头打开成功！使用的索引: {cam_index}")
print("=" * 50)
print("使用说明：")
print("  - 将面部对准摄像头")
print("  - 按 'r' 键设置当前姿态为参考（中性）姿态")
print("  - 按 'q' 键退出程序")
print("=" * 50)

# ============================================================================
# 头部姿态识别类
# ============================================================================

class HeadPoseHistory:
    """头部姿态历史跟踪器"""
    def __init__(self, max_frames=30):
        self.max_frames = max_frames
        self.euler_angles = []  # 欧拉角历史 [(pitch, yaw, roll), ...]
        self.timestamps = []     # 时间戳历史

    def add_frame(self, euler_angles, timestamp):
        """添加新帧数据"""
        self.euler_angles.append(euler_angles)
        self.timestamps.append(timestamp)
        
        # 保持最大长度
        if len(self.euler_angles) > self.max_frames:
            self.euler_angles.pop(0)
            self.timestamps.pop(0)

    def get_smoothed_euler(self, window_size=5):
        """获取平滑后的欧拉角"""
        if len(self.euler_angles) < window_size:
            return self.euler_angles[-1] if self.euler_angles else np.array([0, 0, 0])
        
        # 计算最近window_size帧的平均值
        recent_angles = self.euler_angles[-window_size:]
        avg_pitch = np.mean([angle[0] for angle in recent_angles])
        avg_yaw = np.mean([angle[1] for angle in recent_angles])
        avg_roll = np.mean([angle[2] for angle in recent_angles])
        
        return np.array([avg_pitch, avg_yaw, avg_roll])

    def get_stability(self, window_size=10):
        """获取姿态稳定性（方差越小越稳定）"""
        if len(self.euler_angles) < window_size:
            return 1.0
        
        recent_angles = self.euler_angles[-window_size:]
        variances = []
        for i in range(3):  # pitch, yaw, roll
            values = [angle[i] for angle in recent_angles]
            variances.append(np.var(values))
        
        return np.mean(variances)


def rotation_vector_to_euler(rvec):
    """
    将旋转向量转换为欧拉角（俯仰、偏航、滚转）
    返回角度值（单位：度）
    """
    # 将旋转向量转换为旋转矩阵
    R, _ = cv2.Rodrigues(rvec)

    # 从旋转矩阵计算欧拉角
    sy = np.sqrt(R[0, 0] * R[0, 0] + R[1, 0] * R[1, 0])
    singular = sy < 1e-6

    if not singular:
        pitch = np.arctan2(R[2, 1], R[2, 2])
        yaw = np.arctan2(-R[2, 0], sy)
        roll = np.arctan2(R[1, 0], R[0, 0])
    else:
        pitch = np.arctan2(-R[1, 2], R[1, 1])
        yaw = np.arctan2(-R[2, 0], sy)
        roll = 0

    # 转换为角度
    pitch = np.degrees(pitch)
    yaw = np.degrees(yaw)
    roll = np.degrees(roll)

    return np.array([pitch, yaw, roll], dtype=np.float64)


def euler_to_direction(euler, reference_euler=None):
    """
    将欧拉角转换为可读的方向描述
    如果提供参考欧拉角，则计算相对方向
    返回：(方向文本, 相对角度, 游戏控制值)
    """
    if reference_euler is None:
        # 绝对方向
        pitch, yaw, roll = euler
    else:
        # 相对于参考姿态
        pitch = euler[0] - reference_euler[0]
        yaw = euler[1] - reference_euler[1]
        roll = euler[2] - reference_euler[2]

    direction = []

    # 俯仰角（上/下）
    if pitch > pitch_threshold:
        direction.append("↓ 向下看")
    elif pitch < -pitch_threshold:
        direction.append("↑ 向上看")

    # 偏航角（左/右）
    if yaw > yaw_threshold:
        direction.append("← 向左转")
    elif yaw < -yaw_threshold:
        direction.append("→ 向右转")

    # 滚转角（倾斜）
    if roll > roll_threshold:
        direction.append("↙ 向左倾斜")
    elif roll < -roll_threshold:
        direction.append("↘ 向右倾斜")

    if not direction:
        direction.append("● 居中")

    # 返回游戏控制用的数值（范围-1到1）
    # 注意：游戏控制中，pitch通常是上下（Y轴），yaw是左右（X轴）
    game_pitch = np.clip(pitch / 45.0, -1, 1) if abs(pitch) < 45 else np.sign(pitch)
    game_yaw = np.clip(yaw / 45.0, -1, 1) if abs(yaw) < 45 else np.sign(yaw)
    game_roll = np.clip(roll / 45.0, -1, 1) if abs(roll) < 45 else np.sign(roll)

    return " ".join(direction), (pitch, yaw, roll), (game_pitch, game_yaw, game_roll)


def get_face_landmark_indices():
    """返回用于姿态估计的关键点索引"""
    # MediaPipe面部网格关键点索引 - 选择分布在整个面部的关键点以提高精度
    # 包括：面部轮廓、眉毛、眼睛、鼻子、嘴巴等区域的关键点
    return {
        # 鼻子区域
        'nose_tip': 1,           # 鼻尖
        'nose_bridge_1': 168,    # 鼻根（眉心）
        'nose_bridge_2': 197,    # 鼻梁中部
        'nose_bottom': 2,        # 鼻子底部

        # 下巴和下巴轮廓
        'chin': 152,             # 下巴尖
        'chin_left': 172,        # 下巴左侧
        'chin_right': 397,       # 下巴右侧

        # 眼睛 - 左眼
        'left_eye_left': 33,     # 左眼左角
        'left_eye_right': 133,   # 左眼右角（内眼角）
        'left_eye_top': 159,     # 左眼上眼睑中部
        'left_eye_bottom': 145,  # 左眼下眼睑中部

        # 眼睛 - 右眼
        'right_eye_left': 362,   # 右眼左角（内眼角）
        'right_eye_right': 263,  # 右眼右角
        'right_eye_top': 386,    # 右眼上眼睑中部
        'right_eye_bottom': 374, # 右眼下眼睑中部

        # 眉毛 - 左侧
        'left_eyebrow_inner': 107,   # 左眉内侧
        'left_eyebrow_outer': 70,    # 左眉外侧

        # 眉毛 - 右侧
        'right_eyebrow_inner': 336,  # 右眉内侧
        'right_eyebrow_outer': 300,  # 右眉外侧

        # 嘴巴
        'mouth_left': 61,        # 左嘴角
        'mouth_right': 291,      # 右嘴角
        'mouth_top': 13,         # 上唇中部
        'mouth_bottom': 14,      # 下唇中部

        # 面部轮廓（脸颊）
        'left_cheek': 123,       # 左脸颊
        'right_cheek': 352,      # 右脸颊

        # 额头
        'forehead': 10,          # 前额中部

        # 耳朵区域（近似）
        'left_ear': 234,         # 左耳附近
        'right_ear': 454,        # 右耳附近
    }


# ============================================================================
# 下载面部网格模型文件
# ============================================================================

model_url = "https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task"
model_path = "face_landmarker.task"

if not os.path.exists(model_path):
    print("[INFO] 正在下载面部网格模型文件（约10MB），请稍候...")
    try:
        import urllib.request
        urllib.request.urlretrieve(model_url, model_path)
        print("[SUCCESS] 模型下载完成！")
    except Exception as e:
        print(f"[ERROR] 下载失败：{e}")
        print("[INFO] 请手动下载模型文件放到程序同目录下")
        print("[INFO] 下载地址：", model_url)
        exit()
else:
    print("[INFO] 模型文件已存在")

# ============================================================================
# 初始化面部网格检测器
# ============================================================================

print("[INFO] 正在初始化面部检测器...")

try:
    base_options = python.BaseOptions(model_asset_path=model_path)
    options = vision.FaceLandmarkerOptions(
        base_options=base_options,
        num_faces=1,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5,
        output_face_blendshapes=False,
        output_facial_transformation_matrixes=True
    )
    detector = vision.FaceLandmarker.create_from_options(options)
    print("[SUCCESS] 面部检测器初始化成功")
except Exception as e:
    print(f"[ERROR] 检测器初始化失败：{e}")
    exit()

# ============================================================================
# 相机内参矩阵（用于PnP解算）
# ============================================================================

# 获取摄像头分辨率
frame_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
frame_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

if frame_width == 0 or frame_height == 0:
    frame_width = 640
    frame_height = 480

print(f"[INFO] 摄像头分辨率: {frame_width}x{frame_height}")

# 3D模型点（基于MediaPipe关键点近似，单位：毫米）
# 使用28个关键点分布在整个面部以提高精度
model_points = np.array([
    # 鼻子区域 (1-4)
    (0.0, 0.0, 0.0),           # 1. nose_tip: 鼻尖
    (0.0, 50.0, -50.0),       # 2. nose_bridge_1: 鼻根（眉心） - 在鼻尖上方稍后
    (0.0, 25.0, -25.0),       # 3. nose_bridge_2: 鼻梁中部
    (0.0, -50.0, -30.0),        # 4. nose_bottom: 鼻子底部 - 在鼻尖下方

    # 下巴和下巴轮廓 (5-7)
    (0.0, -330.0, -65.0),      # 5. chin: 下巴尖
    (-60.0, -300.0, -70.0),    # 6. chin_left: 下巴左侧
    (60.0, -300.0, -70.0),     # 7. chin_right: 下巴右侧

    # 眼睛 - 左眼 (8-11)
    (-225.0, 170.0, -135.0),   # 8. left_eye_left: 左眼左角
    (-125.0, 160.0, -120.0),   # 9. left_eye_right: 左眼右角（内眼角）
    (-175.0, 140.0, -130.0),   # 10. left_eye_top: 左眼上眼睑中部
    (-175.0, 190.0, -130.0),   # 11. left_eye_bottom: 左眼下眼睑中部

    # 眼睛 - 右眼 (12-15)
    (125.0, 160.0, -120.0),    # 12. right_eye_left: 右眼左角（内眼角）
    (225.0, 170.0, -135.0),    # 13. right_eye_right: 右眼右角
    (175.0, 140.0, -130.0),    # 14. right_eye_top: 右眼上眼睑中部
    (175.0, 190.0, -130.0),    # 15. right_eye_bottom: 右眼下眼睑中部

    # 眉毛 - 左侧 (16-17)
    (-125.0, 100.0, -110.0),   # 16. left_eyebrow_inner: 左眉内侧
    (-200.0, 110.0, -125.0),   # 17. left_eyebrow_outer: 左眉外侧

    # 眉毛 - 右侧 (18-19)
    (125.0, 100.0, -110.0),    # 18. right_eyebrow_inner: 右眉内侧
    (200.0, 110.0, -125.0),    # 19. right_eyebrow_outer: 右眉外侧

    # 嘴巴 (20-23)
    (-150.0, -150.0, -125.0),  # 20. mouth_left: 左嘴角
    (150.0, -150.0, -125.0),   # 21. mouth_right: 右嘴角
    (0.0, -130.0, -100.0),     # 22. mouth_top: 上唇中部
    (0.0, -180.0, -110.0),     # 23. mouth_bottom: 下唇中部

    # 面部轮廓（脸颊） (24-25)
    (-200.0, -50.0, -90.0),    # 24. left_cheek: 左脸颊
    (200.0, -50.0, -90.0),     # 25. right_cheek: 右脸颊

    # 额头 (26)
    (0.0, 200.0, -80.0),       # 26. forehead: 前额中部

    # 耳朵区域（近似） (27-28)
    (-300.0, -50.0, -50.0),    # 27. left_ear: 左耳附近
    (300.0, -50.0, -50.0),     # 28. right_ear: 右耳附近
], dtype=np.float64)

# 相机内参矩阵
focal_length = frame_width
center = (frame_width / 2, frame_height / 2)
camera_matrix = np.array([
    [focal_length, 0, center[0]],
    [0, focal_length, center[1]],
    [0, 0, 1]
], dtype=np.float64)

# 畸变系数（假设无镜头畸变）
dist_coeffs = np.zeros((4, 1), dtype=np.float64)

# 初始化历史跟踪器
head_history = HeadPoseHistory(max_frames=30)

# 用于控制台输出控制（避免重复打印）
last_print_time = 0
print_interval = 0.3  # 每0.3秒打印一次

print("[INFO] 开始头部姿态识别...")
print("=" * 50)

# ============================================================================
# 主循环
# ============================================================================

while cap.isOpened():
    success, image = cap.read()
    if not success:
        print("[ERROR] 无法读取视频帧")
        break

    # 计算FPS
    curr_time = time.time()
    fps = 1.0 / (curr_time - prev_time) if prev_time > 0 else 0
    prev_time = curr_time

    # 镜像图像以获得更自然的体验
    image = cv2.flip(image, 1)
    
    # 获取图像尺寸
    h, w = image.shape[:2]
    
    # 将BGR图像转换为RGB
    image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    
    # 转换为MediaPipe Image对象
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=image_rgb)

    # 处理图像并检测面部
    try:
        detection_result = detector.detect(mp_image)
    except Exception as e:
        print(f"[ERROR] 检测错误：{e}", flush=True)
        continue

    # 初始化状态变量
    pose_detected = False
    direction_text = "未检测到面部"
    relative_angles = None
    game_controls = (0, 0, 0)
    euler_angles = None

    # 处理检测到的面部
    if detection_result.face_landmarks:
        face_landmarks = detection_result.face_landmarks[0]  # 只取第一张脸

        # 获取选定关键点的图像坐标
        landmark_indices = get_face_landmark_indices()
        image_points = []
        
        for idx in landmark_indices.values():
            landmark = face_landmarks[idx]
            x = int(landmark.x * w)
            y = int(landmark.y * h)
            image_points.append([x, y])
            
            # 在图像上标记这些关键点
            cv2.circle(image, (x, y), 5, (0, 255, 255), -1)
        
        image_points = np.array(image_points, dtype=np.float64)
        
        # 使用PnP算法求解旋转和平移向量
        success, rvec, tvec = cv2.solvePnP(
            model_points,
            image_points,
            camera_matrix,
            dist_coeffs,
            flags=cv2.SOLVEPNP_ITERATIVE
        )
        
        if success:
            # 将旋转向量转换为欧拉角
            euler_angles = rotation_vector_to_euler(rvec)
            current_euler = euler_angles
            
            # 更新历史记录
            head_history.add_frame(euler_angles, curr_time)
            
            # 获取平滑后的欧拉角（减少抖动）
            smoothed_euler = head_history.get_smoothed_euler(window_size=3)
            
            # 获取相对于参考姿态的方向
            if reference_euler is not None:
                direction_text, relative_angles, game_controls = euler_to_direction(smoothed_euler, reference_euler)
            else:
                direction_text, relative_angles, game_controls = euler_to_direction(smoothed_euler)
            
            pose_detected = True
            
            # 绘制3D坐标轴（用于可视化）
            axis_length = 100
            axis_points = np.float32([
                [0, 0, 0],
                [axis_length, 0, 0],
                [0, axis_length, 0],
                [0, 0, -axis_length]
            ]).reshape(-1, 3)
            
            img_points, _ = cv2.projectPoints(
                axis_points,
                rvec,
                tvec,
                camera_matrix,
                dist_coeffs
            )
            
            # 绘制坐标轴
            origin = tuple(map(int, img_points[0].ravel()))
            cv2.line(image, origin, tuple(map(int, img_points[1].ravel())), (0, 0, 255), 3)   # X轴（红色）
            cv2.line(image, origin, tuple(map(int, img_points[2].ravel())), (0, 255, 0), 3)   # Y轴（绿色）
            cv2.line(image, origin, tuple(map(int, img_points[3].ravel())), (255, 0, 0), 3)   # Z轴（蓝色）
            
            # 在面部周围绘制边界框（可选）
            # 计算所有关键点的最小/最大坐标
            all_x = [int(lm.x * w) for lm in face_landmarks]
            all_y = [int(lm.y * h) for lm in face_landmarks]
            if all_x and all_y:
                min_x, max_x = min(all_x), max(all_x)
                min_y, max_y = min(all_y), max(all_y)
                cv2.rectangle(image, (min_x-10, min_y-10), (max_x+10, max_y+10), (255, 0, 0), 2)
    
    # ========================================================================
    # 显示信息
    # ========================================================================
    
    # 显示FPS
    cv2.putText(image, f"FPS: {int(fps)}", (10, 30),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)
    
    # 显示检测状态
    if pose_detected:
        cv2.putText(image, "Face Detected", (10, 55),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    else:
        cv2.putText(image, "No Face Detected", (10, 55),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)
    
    # 显示方向文本
    cv2.putText(image, direction_text, (10, 80),
               cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    
    # 显示参考姿态状态
    if reference_euler is not None:
        ref_status = "Reference: SET"
        ref_color = (0, 255, 0)
    else:
        ref_status = "Reference: NOT SET (Press 'r')"
        ref_color = (0, 0, 255)
    cv2.putText(image, ref_status, (10, 110),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, ref_color, 1)
    
    # 显示欧拉角（绝对角度）
    if euler_angles is not None:
        angle_text = f"Angles: P:{euler_angles[0]:.1f} Y:{euler_angles[1]:.1f} R:{euler_angles[2]:.1f}"
        cv2.putText(image, angle_text, (10, 135),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 255), 1)
    
    # 显示相对角度（相对于参考姿态）
    if relative_angles is not None and reference_euler is not None:
        rel_text = f"Relative: P:{relative_angles[0]:.1f} Y:{relative_angles[1]:.1f} R:{relative_angles[2]:.1f}"
        cv2.putText(image, rel_text, (10, 155),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 0), 1)
    
    # 显示游戏控制值
    if pose_detected:
        game_text = f"Game: X:{game_controls[1]:.2f} Y:{game_controls[0]:.2f} Z:{game_controls[2]:.2f}"
        cv2.putText(image, game_text, (10, 175),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 255), 1)
    
    # 显示提示信息
    cv2.putText(image, "Press 'q' to quit | 'r' to set reference", (10, h - 10),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    # 显示稳定性指示器（可选）
    if head_history and len(head_history.euler_angles) > 10:
        stability = head_history.get_stability()
        if stability < 5:
            stability_text = "Stable"
            stability_color = (0, 255, 0)
        elif stability < 15:
            stability_text = "Moving"
            stability_color = (0, 255, 255)
        else:
            stability_text = "Unstable"
            stability_color = (0, 0, 255)
        cv2.putText(image, stability_text, (w - 100, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, stability_color, 1)
    
    # 控制台输出（避免过多打印）
    if pose_detected and curr_time - last_print_time > print_interval:
        if reference_euler is not None and relative_angles is not None:
            print(f"[方向] {direction_text} | 相对角度: P:{relative_angles[0]:.1f} Y:{relative_angles[1]:.1f} R:{relative_angles[2]:.1f}")
            print(f"[游戏控制] X轴:{game_controls[1]:.2f} Y轴:{game_controls[0]:.2f} Z轴:{game_controls[2]:.2f}")
        elif euler_angles is not None:
            print(f"[绝对角度] P:{euler_angles[0]:.1f} Y:{euler_angles[1]:.1f} R:{euler_angles[2]:.1f}")
        last_print_time = curr_time
    
    # 显示处理后的图像
    cv2.imshow('Head Pose Recognition - Game Control', image)
    
    # 按键处理
    key = cv2.waitKey(5) & 0xFF
    if key == ord('q'):
        print("\n[INFO] 正在退出...")
        break
    elif key == ord('r'):
        if current_euler is not None:
            reference_euler = current_euler.copy()
            print(f"\n[参考姿态已设置] 俯仰={reference_euler[0]:.1f}, 偏航={reference_euler[1]:.1f}, 滚转={reference_euler[2]:.1f}")
            print("-" * 50)
        else:
            print("\n[警告] 无法设置参考姿态: 未检测到面部")

# 释放资源
try:
    detector.close()
except Exception as e:
    print(f"[WARN] 关闭检测器时出错（可忽略）: {e}")

cap.release()
cv2.destroyAllWindows()
print("[INFO] 程序结束")