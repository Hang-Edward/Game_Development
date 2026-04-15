import cv2
import mediapipe as mp
import numpy as np
import urllib.request
import os
import time
import sys

# 设置控制台编码为UTF-8
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', line_buffering=True)
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', line_buffering=True)

# MediaPipe 0.10.33 的新API导入（不使用solutions）
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# FPS计算
prev_time = 0

# 手势历史跟踪器
hand_histories = {}  # hand_id -> HandHistory

# 手势打印记录（避免重复打印）
gesture_print_history = {}  # gesture_key -> last_print_time

# 打开摄像头
print("[INFO] 正在打开摄像头...")
# 尝试不同索引和不同的后端
# 根据用户之前的修改，尝试索引1（可能外接摄像头是索引1）
cam_index = 1

# 先尝试默认方式
cap = cv2.VideoCapture(cam_index)
if not cap.isOpened():
    print(f"[WARN] 索引 {cam_index} 打开失败，尝试使用DShow后端...")
    # 尝试使用DShow后端
    cap = cv2.VideoCapture(cam_index, cv2.CAP_DSHOW)
    if not cap.isOpened():
        print(f"[WARN] DShow后端也失败，尝试索引0...")
        # 尝试索引0
        cap = cv2.VideoCapture(0)
        if not cap.isOpened():
            print(f"[WARN] 索引0失败，尝试索引0使用DShow...")
            cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
            if not cap.isOpened():
                print("[ERROR] 错误：无法打开摄像头")
                print("[INFO] 请检查：")
                print("  1. 摄像头是否已连接")
                print("  2. 摄像头驱动是否正常")
                print("  3. 是否有其他程序占用摄像头（如Zoom、Teams、微信等）")
                print("  4. 尝试重启电脑或重新插拔摄像头")
                exit()

print(f"[SUCCESS] 摄像头打开成功！使用的索引: {cam_index}")
print("=" * 50)
print("使用说明：")
print("  - 将手放在摄像头前")
print("  - 按 'q' 键退出程序")
print("  - 按 'r' 键重置检测")
print("=" * 50)

# ============================================================================
# 手势识别系统增强 - 数据结构和辅助函数
# ============================================================================

# 手势检测参数配置
GESTURE_CONFIG = {
    'enable_complex_gestures': True,
    'history_frames': 30,
    'velocity_threshold': 0.2,           # 降低速度阈值，更容易检测运动
    'distance_threshold': 0.15,          # 增加距离阈值，双手手势更宽松
    'angle_threshold': 0.3,              # 降低角度阈值，更容易检测旋转
    'finger_together_threshold': 0.08,   # 增加手指并拢阈值，手刀手势更宽松
}

# 手部历史数据（用于运动跟踪）
class HandHistory:
    def __init__(self, max_frames=30):
        self.max_frames = max_frames
        self.positions = []  # 手掌中心位置历史 (x, y)
        self.orientations = []  # 手掌方向历史 (3D法向量)
        self.timestamps = []  # 时间戳历史
        self.landmarks_history = []  # 手部关键点历史（用于手指轮拨检测）

    def add_frame(self, position, orientation, timestamp, landmarks=None):
        # 添加新帧数据，保持最大长度
        self.positions.append(position)
        self.orientations.append(orientation)
        self.timestamps.append(timestamp)
        if landmarks is not None:
            self.landmarks_history.append(landmarks)
            if len(self.landmarks_history) > self.max_frames:
                self.landmarks_history.pop(0)
        if len(self.positions) > self.max_frames:
            self.positions.pop(0)
            self.orientations.pop(0)
            self.timestamps.pop(0)

    def get_recent_landmarks(self, n=10):
        # 获取最近n个手部关键点
        return self.landmarks_history[-n:] if len(self.landmarks_history) >= n else self.landmarks_history

    def get_velocity(self):
        # 计算平均速度（单位：像素/秒？归一化坐标/秒）
        if len(self.positions) < 2:
            return 0
        total_distance = 0
        total_time = self.timestamps[-1] - self.timestamps[0]
        if total_time == 0:
            return 0
        for i in range(1, len(self.positions)):
            dx = self.positions[i][0] - self.positions[i-1][0]
            dy = self.positions[i][1] - self.positions[i-1][1]
            total_distance += (dx**2 + dy**2)**0.5
        return total_distance / total_time

    def get_direction(self):
        # 计算平均运动方向 (dx, dy) 单位向量
        if len(self.positions) < 2:
            return (0, 0)
        start_pos = self.positions[0]
        end_pos = self.positions[-1]
        dx = end_pos[0] - start_pos[0]
        dy = end_pos[1] - start_pos[1]
        length = (dx**2 + dy**2)**0.5
        if length == 0:
            return (0, 0)
        return (dx/length, dy/length)

    def get_recent_positions(self, n=5):
        # 获取最近n个位置
        return self.positions[-n:] if len(self.positions) >= n else self.positions

    def get_recent_orientations(self, n=5):
        # 获取最近n个方向
        return self.orientations[-n:] if len(self.orientations) >= n else self.orientations

# 几何计算辅助函数
def calculate_palm_center(hand_landmarks):
    """计算手掌中心点（手腕和掌心的平均）"""
    # 使用手掌基部的几个点计算中心：手腕(0), 食指基部(5), 中指基部(9), 无名指基部(13), 小指基部(17)
    points = [hand_landmarks[0], hand_landmarks[5], hand_landmarks[9],
              hand_landmarks[13], hand_landmarks[17]]
    x_avg = sum(p.x for p in points) / len(points)
    y_avg = sum(p.y for p in points) / len(points)
    return (x_avg, y_avg)

def calculate_palm_orientation(hand_landmarks):
    """计算手掌方向（3D法向量）"""
    # 使用手腕、食指基部、小指基部计算平面法向量
    wrist = np.array([hand_landmarks[0].x, hand_landmarks[0].y, hand_landmarks[0].z])
    index_base = np.array([hand_landmarks[5].x, hand_landmarks[5].y, hand_landmarks[5].z])
    pinky_base = np.array([hand_landmarks[17].x, hand_landmarks[17].y, hand_landmarks[17].z])

    v1 = index_base - wrist
    v2 = pinky_base - wrist
    normal = np.cross(v1, v2)
    norm = np.linalg.norm(normal)
    if norm > 0:
        normal = normal / norm
    return normal

def calculate_finger_spread(hand_landmarks):
    """计算手指分开程度（平均指尖距离）"""
    fingertips = [hand_landmarks[4], hand_landmarks[8], hand_landmarks[12],
                  hand_landmarks[16], hand_landmarks[20]]
    # 计算指尖间的平均距离
    total_distance = 0
    count = 0
    for i in range(len(fingertips)):
        for j in range(i+1, len(fingertips)):
            dx = fingertips[i].x - fingertips[j].x
            dy = fingertips[i].y - fingertips[j].y
            total_distance += (dx**2 + dy**2)**0.5
            count += 1
    return total_distance / count if count > 0 else 0

def are_fingers_together(hand_landmarks, threshold=None):
    """检测手指是否并拢（手刀姿势）"""
    if threshold is None:
        threshold = GESTURE_CONFIG['finger_together_threshold']

    finger_tips = [hand_landmarks[4], hand_landmarks[8], hand_landmarks[12],
                   hand_landmarks[16], hand_landmarks[20]]
    # 检查任意两个指尖距离是否超过阈值
    for i in range(len(finger_tips)):
        for j in range(i+1, len(finger_tips)):
            dx = finger_tips[i].x - finger_tips[j].x
            dy = finger_tips[i].y - finger_tips[j].y
            if (dx**2 + dy**2)**0.5 > threshold:
                return False
    return True

def is_palm_open(hand_landmarks):
    """检测手掌是否张开（现有逻辑）"""
    wrist = hand_landmarks[0]
    thumb_tip = hand_landmarks[4]
    index_tip = hand_landmarks[8]
    middle_tip = hand_landmarks[12]
    ring_tip = hand_landmarks[16]
    pinky_tip = hand_landmarks[20]

    return (thumb_tip.y < wrist.y and index_tip.y < wrist.y and
            middle_tip.y < wrist.y and ring_tip.y < wrist.y and pinky_tip.y < wrist.y)

def is_fist(hand_landmarks):
    """检测是否握拳（现有逻辑）"""
    index_tip = hand_landmarks[8]
    middle_tip = hand_landmarks[12]
    ring_tip = hand_landmarks[16]
    pinky_tip = hand_landmarks[20]
    index_pip = hand_landmarks[6]
    middle_pip = hand_landmarks[10]
    ring_pip = hand_landmarks[14]
    pinky_pip = hand_landmarks[18]

    return (index_tip.y > index_pip.y and middle_tip.y > middle_pip.y and
            ring_tip.y > ring_pip.y and pinky_tip.y > pinky_pip.y)

def calculate_wrist_angle(hand_landmarks):
    """计算手腕角度（手腕-手掌方向）"""
    wrist = np.array([hand_landmarks[0].x, hand_landmarks[0].y, hand_landmarks[0].z])
    middle_mcp = np.array([hand_landmarks[9].x, hand_landmarks[9].y, hand_landmarks[9].z])
    vector = middle_mcp - wrist
    # 计算与垂直方向的夹角（简化）
    vertical = np.array([0, -1, 0])  # 图像坐标系Y向下
    norm_vector = np.linalg.norm(vector)
    norm_vertical = np.linalg.norm(vertical)
    if norm_vector > 0 and norm_vertical > 0:
        vector = vector / norm_vector
        cos_angle = np.dot(vector, vertical / norm_vertical)
        return np.arccos(np.clip(cos_angle, -1.0, 1.0))
    return 0

# ============================================================================
# 复杂手势检测函数
# ============================================================================



def detect_fist(hand_landmarks, hand_history):
    """检测握拳手势 - 要求握拳状态稳定持续"""
    # 条件1: 当前帧握拳
    if not is_fist(hand_landmarks):
        return False

    # 条件2: 握拳状态稳定性检查（避免瞬时误识别）
    # 获取最近的手部关键点历史
    recent_landmarks = hand_history.get_recent_landmarks(n=5)  # 检查最近5帧
    if len(recent_landmarks) < 3:  # 至少需要3帧数据
        return False

    # 检查最近几帧是否都是握拳状态
    fist_frames = 0
    for landmarks in recent_landmarks:
        if is_fist(landmarks):
            fist_frames += 1

    # 要求至少80%的帧是握拳状态
    fist_ratio = fist_frames / len(recent_landmarks)
    if fist_ratio < 0.8:
        return False

    # 所有条件满足，检测到稳定的握拳手势
    return True


def detect_cut(hand_landmarks, hand_history):
    """检测切手势（手掌朝任意方向滑动）"""
    if len(hand_history.positions) < 10:
        return False

    # 条件1: 平滑连续运动
    velocity = hand_history.get_velocity()
    if velocity < 0.1 or velocity > 1.0:  # 速度适中
        return False

    # 条件2: 任意方向运动（移除水平方向限制）

    # 条件3: 手掌保持相对稳定姿势（近似张开）
    if not is_palm_open(hand_landmarks):
        return False

    # 条件4: 移动足够距离
    if len(hand_history.positions) >= 2:
        start_pos = hand_history.positions[0]
        end_pos = hand_history.positions[-1]
        total_distance = ((end_pos[0] - start_pos[0])**2 + (end_pos[1] - start_pos[1])**2)**0.5
        if total_distance > 0.15:
            return True

    return False





def detect_two_hands_together(detection_result):
    """检测双手合十 - 手掌相对贴合"""
    if len(detection_result.hand_landmarks) != 2:
        return False

    # 获取左右手信息
    hand_landmarks = detection_result.hand_landmarks
    handedness = detection_result.handedness if hasattr(detection_result, 'handedness') else None

    # 识别左右手
    left_hand_idx = None
    right_hand_idx = None

    for i in range(len(hand_landmarks)):
        if handedness and i < len(handedness):
            category = handedness[i][0]
            if hasattr(category, 'category_name'):
                label = category.category_name
            elif hasattr(category, 'display_name'):
                label = category.display_name
            else:
                label = f"hand_{i}"

            if 'Left' in str(label):
                left_hand_idx = i
            elif 'Right' in str(label):
                right_hand_idx = i

    # 如果无法区分左右手，使用默认索引
    if left_hand_idx is None or right_hand_idx is None:
        left_hand_idx = 0
        right_hand_idx = 1

    left_hand = hand_landmarks[left_hand_idx]
    right_hand = hand_landmarks[right_hand_idx]

    # 检查1: 手腕位置接近（合十时手腕位置相近）
    left_wrist_x = left_hand[0].x
    right_wrist_x = right_hand[0].x
    left_wrist_y = left_hand[0].y
    right_wrist_y = right_hand[0].y

    wrist_x_distance = abs(left_wrist_x - right_wrist_x)
    wrist_y_distance = abs(left_wrist_y - right_wrist_y)

    # 合十时手腕水平距离应很小，垂直距离适中
    if wrist_x_distance > 0.1:  # 手腕水平距离不能太大
        return False
    if wrist_y_distance > 0.2:  # 手腕垂直距离不能太大
        return False

    # 检查2: 手掌方向相对（面对面）
    orient_left = calculate_palm_orientation(left_hand)
    orient_right = calculate_palm_orientation(right_hand)

    # 计算手掌方向点积来判断相对方向
    # 如果手掌相对（面对面），点积应该为负值
    dot_product = np.dot(orient_left, orient_right)
    if dot_product > -0.5:  # 点积不够负，说明手掌不够相对
        return False

    # 检查3: 手掌距离接近（合十时手掌贴合）
    # 使用手掌中心点计算距离
    left_palm_center = calculate_palm_center(left_hand)
    right_palm_center = calculate_palm_center(right_hand)

    palm_distance = ((left_palm_center[0] - right_palm_center[0])**2 +
                     (left_palm_center[1] - right_palm_center[1])**2)**0.5

    if palm_distance > 0.15:  # 手掌中心距离不能太大
        return False

    # 检查4: 手指方向 - 合十时手指大致向上
    # 检查左手中指方向（从手腕到中指指尖）
    left_wrist = np.array([left_hand[0].x, left_hand[0].y, left_hand[0].z])
    left_middle_tip = np.array([left_hand[12].x, left_hand[12].y, left_hand[12].z])
    left_finger_vector = left_middle_tip - left_wrist

    # 检查右手中指方向
    right_wrist = np.array([right_hand[0].x, right_hand[0].y, right_hand[0].z])
    right_middle_tip = np.array([right_hand[12].x, right_hand[12].y, right_hand[12].z])
    right_finger_vector = right_middle_tip - right_wrist

    # 合十时手指应大致向上（Y分量为负，因为图像坐标系Y向下）
    left_finger_upward = left_finger_vector[1] < -0.05  # 向上
    right_finger_upward = right_finger_vector[1] < -0.05  # 向上

    if not (left_finger_upward and right_finger_upward):
        return False

    # 所有条件满足，检测到双手合十
    return True

# 手势显示颜色和标签映射
COMPLEX_GESTURE_DISPLAY = {
    'cut': {'name': '切', 'color': (0, 255, 255), 'emoji': '[⇄]'},
    'fist': {'name': '握拳', 'color': (200, 0, 0), 'emoji': '[👊]'},
    'two_hands_together': {'name': '双手合十', 'color': (0, 255, 255), 'emoji': '[🙏]'},
}

# ============================================================================

# 下载模型文件（如果不存在）
model_url = "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
model_path = "hand_landmarker.task"

if not os.path.exists(model_path):
    print("[INFO] 正在下载模型文件（约20MB），请稍候...")
    try:
        urllib.request.urlretrieve(model_url, model_path)
        print("[SUCCESS] 模型下载完成！")
    except Exception as e:
        print(f"[ERROR] 下载失败：{e}")
        print("[INFO] 请手动下载模型文件放到程序同目录下")
        print("[INFO] 下载地址：", model_url)
        exit()
else:
    print("[INFO] 模型文件已存在")

# 初始化手部检测器 - 使用正确的参数名
try:
    base_options = python.BaseOptions(model_asset_path=model_path)
    options = vision.HandLandmarkerOptions(
        base_options=base_options,
        num_hands=2,
        min_hand_detection_confidence=0.5,  # 修正参数名
        min_hand_presence_confidence=0.5,   # 添加这个参数
        min_tracking_confidence=0.5
    )
    detector = vision.HandLandmarker.create_from_options(options)
    print("[SUCCESS] 手部检测器初始化成功")
except Exception as e:
    print(f"[ERROR] 检测器初始化失败：{e}")
    print("[INFO] 尝试使用默认参数...")
    try:
        # 尝试使用最简单的配置
        options = vision.HandLandmarkerOptions(
            base_options=base_options,
            num_hands=2
        )
        detector = vision.HandLandmarker.create_from_options(options)
        print("[SUCCESS] 使用默认参数初始化成功")
    except Exception as e2:
        print(f"[ERROR] 仍然失败：{e2}")
        exit()

# 定义手部连接关系（21个关键点的连接）
HAND_CONNECTIONS = [
    (0, 1), (1, 2), (2, 3), (3, 4),  # 拇指
    (0, 5), (5, 6), (6, 7), (7, 8),  # 食指
    (0, 9), (9, 10), (10, 11), (11, 12),  # 中指
    (0, 13), (13, 14), (14, 15), (15, 16),  # 无名指
    (0, 17), (17, 18), (18, 19), (19, 20),  # 小指
    (5, 9), (9, 13), (13, 17)  # 手掌连接
]



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

    # 处理图像并检测手部
    try:
        detection_result = detector.detect(mp_image)
    except Exception as e:
        print(f"[ERROR] 检测错误：{e}", flush=True)
        continue

    # 绘制手部关键点和连接线
    if detection_result.hand_landmarks:
        # 更新手部历史数据
        for hand_idx, hand_landmarks in enumerate(detection_result.hand_landmarks):
            # 生成手部ID：使用手部标签（如果可用）或索引
            hand_id = f"hand_{hand_idx}"
            try:
                if detection_result.handedness and len(detection_result.handedness) > hand_idx:
                    # handedness 是一个列表，每个元素是一个 ClassificationResult
                    # 获取第一个分类的类别名称
                    category = detection_result.handedness[hand_idx][0]
                    if hasattr(category, 'category_name'):
                        hand_id = category.category_name  # "Left" 或 "Right"
                    elif hasattr(category, 'display_name'):
                        hand_id = category.display_name  # 备用属性名
            except Exception as e:
                # 如果获取手部标签失败，使用默认ID
                pass

            # 获取或创建历史跟踪器
            if hand_id not in hand_histories:
                hand_histories[hand_id] = HandHistory(max_frames=GESTURE_CONFIG['history_frames'])

            # 计算手掌特征
            palm_center = calculate_palm_center(hand_landmarks)
            palm_orientation = calculate_palm_orientation(hand_landmarks)

            # 更新历史数据
            hand_histories[hand_id].add_frame(palm_center, palm_orientation, curr_time, hand_landmarks)

        # 进行手势检测和绘制
        for hand_idx, hand_landmarks in enumerate(detection_result.hand_landmarks):
            # 绘制连接线
            for connection in HAND_CONNECTIONS:
                start_idx, end_idx = connection
                start_point = hand_landmarks[start_idx]
                end_point = hand_landmarks[end_idx]
                start_x = int(start_point.x * w)
                start_y = int(start_point.y * h)
                end_x = int(end_point.x * w)
                end_y = int(end_point.y * h)
                cv2.line(image, (start_x, start_y), (end_x, end_y), (0, 255, 0), 2)
            
            # 绘制关键点
            for idx, landmark in enumerate(hand_landmarks):
                x = int(landmark.x * w)
                y = int(landmark.y * h)
                # 指尖用红色标记
                if idx in [4, 8, 12, 16, 20]:
                    cv2.circle(image, (x, y), 8, (0, 0, 255), -1)
                    # 显示指尖标签
                    if idx == 4:
                        cv2.putText(image, "Thumb", (x-20, y-10), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 255), 1)
                    elif idx == 8:
                        cv2.putText(image, "Index", (x-20, y-10), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 255), 1)
                    elif idx == 12:
                        cv2.putText(image, "Middle", (x-25, y-10), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 255), 1)
                    elif idx == 16:
                        cv2.putText(image, "Ring", (x-20, y-10), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 255), 1)
                    elif idx == 20:
                        cv2.putText(image, "Pinky", (x-20, y-10), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 255), 1)
                else:
                    cv2.circle(image, (x, y), 4, (0, 255, 0), -1)

            # 不再显示静态手势，专注动态手势检测

        # 复杂手势检测
        if GESTURE_CONFIG['enable_complex_gestures']:
            complex_gestures_detected = []

            # 单手复杂手势检测
            for hand_idx, hand_landmarks in enumerate(detection_result.hand_landmarks):
                # 获取手部ID（与之前一致）
                hand_id = f"hand_{hand_idx}"
                try:
                    if detection_result.handedness and len(detection_result.handedness) > hand_idx:
                        category = detection_result.handedness[hand_idx][0]
                        if hasattr(category, 'category_name'):
                            hand_id = category.category_name
                        elif hasattr(category, 'display_name'):
                            hand_id = category.display_name
                except Exception as e:
                    pass

                history = hand_histories.get(hand_id)
                if not history:
                    continue

                # 检测各种复杂手势
                if detect_cut(hand_landmarks, history):
                    complex_gestures_detected.append(('cut', hand_id))
                elif detect_fist(hand_landmarks, history):
                    complex_gestures_detected.append(('fist', hand_id))

            # 双手复杂手势检测 - 只保留双手合十
            if detect_two_hands_together(detection_result):
                complex_gestures_detected.append(('two_hands_together', 'both'))

            # 显示复杂手势结果
            for i, (gesture_key, hand_id) in enumerate(complex_gestures_detected):
                display_info = COMPLEX_GESTURE_DISPLAY.get(gesture_key)
                if display_info:
                    gesture_text = f"{display_info['emoji']} {display_info['name']}"
                    if hand_id not in ['both', 'Left', 'Right']:
                        gesture_text = f"{hand_id}: {gesture_text}"
                    y_position = 140 + i * 30
                    cv2.putText(image, gesture_text, (10, y_position),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.7, display_info['color'], 2)

                    # 输出到控制台（避免重复打印）
                    print_key = f"{gesture_key}_{hand_id}"
                    last_print_time = gesture_print_history.get(print_key, 0)
                    if curr_time - last_print_time > 0.5:  # 至少间隔0.5秒
                        print(f"[手势检测] {display_info['name']} ({hand_id})", flush=True)
                        gesture_print_history[print_key] = curr_time

    # 显示FPS
    cv2.putText(image, f"FPS: {int(fps)}", (10, 80),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)
    
    # 显示检测状态
    if detection_result.hand_landmarks:
        hand_count = len(detection_result.hand_landmarks)
        cv2.putText(image, f"Hands Detected: {hand_count}", (10, 110),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    else:
        cv2.putText(image, "No hand detected", (10, 110),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)
    
    # 显示提示信息
    cv2.putText(image, "Press 'q' to quit", (10, h - 10),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    # 显示处理后的图像
    cv2.imshow('Hand Tracking - MediaPipe 0.10.33', image)

    # 按键处理
    key = cv2.waitKey(5) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('r'):
        print("[INFO] 重置检测器...")
        try:
            detector.close()
        except Exception as e:
            print(f"[WARN] 关闭检测器时出错（可忽略）: {e}")
        try:
            detector = vision.HandLandmarker.create_from_options(options)
            print("[SUCCESS] 重置完成")
        except Exception as e:
            print(f"[ERROR] 重新创建检测器失败: {e}")
            print("[INFO] 程序将继续使用旧的检测器")

# 释放资源
try:
    detector.close()
except Exception as e:
    print(f"[WARN] 关闭检测器时出错（可忽略）: {e}")

cap.release()
cv2.destroyAllWindows()
print("[INFO] 程序结束")