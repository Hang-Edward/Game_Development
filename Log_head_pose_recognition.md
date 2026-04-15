# Log of Head Pose Recognition Program Modifications

This file records changes made to the head pose recognition program.

## 2026-04-15: Initial Creation

### Description

Created initial head pose recognition program with the following features:

- Real-time head pose estimation using webcam and MediaPipe Face Mesh
- Head direction detection (pitch, yaw, roll)
- Reference pose reset with 'r' key
- Quit with 'q' key
- Real-time console output of head direction changes
- Visual feedback with 3D axes overlay

### Technical Details

- Uses OpenCV for video capture and image processing
- Uses MediaPipe for facial landmark detection
- Uses solvePnP for 3D pose estimation
- Converts rotation vectors to Euler angles for intuitive direction output
- Threshold-based direction classification (up/down, left/right, tilt)

### Files Created/Modified

1. `head_pose_recognition.py` - Main program
2. `requirements.txt` - List of required Python packages

### Dependencies

The program requires the following Python packages:

- opencv-python>=4.5.0
- mediapipe>=0.10.0
- numpy>=1.19.0

Install with:

```bash
pip install -r requirements.txt
```

### Usage Instructions

1. Connect a webcam to your computer
2. Run the program: `python head_pose_recognition.py`
3. Position your face in front of the camera
4. Press 'r' to set your current head pose as the reference (neutral) position
5. Move your head - the program will output direction changes in the console
6. Press 'q' to quit

### Notes

- The program assumes a standard webcam with no lens distortion
- For better accuracy, camera calibration is recommended
- The 3D model points are approximate; for precise applications, custom calibration with a known head model is needed
- Direction thresholds can be adjusted in the code (pitch_threshold, yaw_threshold, roll_threshold)

### Future Improvements

- Add camera calibration support
- Implement smoother angle filtering
- Add game integration examples
- Support for multiple camera resolutions
- Configurable sensitivity settings

## 2026-04-15: 中文注释和错误修复

### 修改描述

1. 将所有英文注释和文档字符串翻译为中文
2. 修复了MediaPipe导入兼容性问题
3. 将所有控制台输出信息翻译为中文
4. 改进了方向描述的中文表达

### 技术细节

- 保留了完整的算法逻辑和功能
- 改进了错误处理信息的中文表达
- 方向描述：向下看、向上看、向左转、向右转、向左倾斜、向右倾斜
- 阈值设置保持不变（俯仰、偏航、滚转各10度）

### 使用说明更新

1. 运行程序：`python head_pose_recognition.py`
2. 按 'r' 键将当前头部姿态设置为参考（中性）姿态
3. 移动头部 - 控制台将实时输出中文方向变化
4. 按 'q' 键退出程序

### 注意事项

- 程序窗口标题使用中文字符"头部姿态识别"
- 方向输出使用中文，便于理解
- 保持原有的精度和实时性

## 2026-04-15: 提高头部姿态识别精度

### 修改描述

1. 增加用于PnP姿态估计的关键点数量，从6个增加到28个
2. 关键点分布在整个面部：鼻子、下巴、眼睛、眉毛、嘴巴、脸颊、额头、耳朵区域
3. 启用MediaPipe的面部变换矩阵输出（备用方案）
4. 改进了3D模型点的坐标定义

### 技术细节

- 使用28个关键点进行PnP解算，提高姿态估计精度
- 关键点选择覆盖整个面部，提供更全面的几何约束
- 3D模型点基于标准头部模型近似，单位毫米
- 保留了原有的平滑处理和历史跟踪功能
- 变换矩阵作为备用方案（已启用但未在主流程中使用）

### 精度改进

- 更多的关键点提供更强的几何约束，减少姿态估计误差
- 分布在整个面部的关键点对头部旋转更敏感
- 平滑算法减少抖动，提高稳定性

### 使用说明

- 运行方式不变：`python head_pose_recognition.py`
- 按 'r' 键设置参考姿态，'q' 键退出
- 控制台输出方向变化和游戏控制值

### 注意事项

- 3D模型点为近似值，实际精度可能因个体面部差异而变化
- 对于更高精度需求，建议进行相机标定和个人化面部模型标定
- 更多关键点可能增加计算量，但现代CPU/GPU通常可以实时处理
