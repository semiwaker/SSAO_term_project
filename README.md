# SSDO 课程大作业

## 程序说明

这个程序实现了对黑龙模型的SSDO渲染和摄像头的自由移动。
渲染方式和摄像头的移动通过键盘和鼠标来控制。

## 操作方式

+ WASD：（与视线方向）水平移动摄像头，W前进，S后退，S左移，D右移。
+ QE：以视线为轴，旋转摄像头，Q逆时针旋转，E顺时针旋转。
+ 空格与左Shift：垂直移动摄像头，空格上升，左Shift下降。
+ 移动鼠标：改变摄像头方向，使摄像头往鼠标移动方向转向。
+ 数字1~4：4种不同输出模式。1 打开遮蔽和一次弹射（默认），2 打开遮蔽，关闭一次弹射，3 查看一次弹射， 4 查看AO值。
+ 数字8、9、0：3种不同遮蔽计算方式。8 无遮蔽， 9 SSAO， 0 SSDO。
+ F1：截图，存储在当前目录下。

## 代码说明

+ `main.cpp` 主程序，窗口环境设置和键鼠控制。
+ `camera.h` `camera.cpp` 摄像头的计算。
+ `scene.h` `scene.cpp` 模型载入与渲染流程。主要渲染过程部分在SSDORenderer类中。
+ `rgbe.h` `rgbe.cpp` 从[http://www.graphics.cornell.edu/~bjw/rgbe.html](http://www.graphics.cornell.edu/~bjw/rgbe.html)获得并修改的用于处理RGBE格式环境纹理的程序。
+ `utils.h` `utils.cpp` `GLenv.h` 辅助程序。
+ GLFW Glad AssImp GLM FreeImage `stb_image.h` 这个程序使用的开源库。

着色器代码

+ baseline.fs baseline.vs baseline_normals.fs baseline_tangent.vs 基准渲染管线，用于对照。
+ capture.vs capture.fs 从SphericalMap制作CubeMap用于加速SSDO direct的计算。
+ skybox.vs skybox.fs 渲染背景的管线。
+ shadow.vs shadow.fs 渲染shadow map的管线。
+ geometry.vs geometry.fs 渲染屏幕空间上几何信息的管线。
+ quad.vs 在屏幕空间上渲染的通用Vertex Shader。
+ ssao.fs 计算SSAO遮蔽值。（其功能在ssdo.fs里也有实现）
+ ssdo.fs 计算SSDO直接光照遮蔽值。
+ indirect.fs 计算SSDO间接光照遮蔽值。
+ blur.fs 计算直接光照遮蔽值平均模糊。（其实可以并入lighting.fs但是出于尽量接近一般写法没有这么做）
+ lighting.fs 计算光照明，同时对将一次弹射和shadow map做平均模糊。
+ stencil.vs stencil.fs 计算龙模型的掩模，用于分离背景和模型，同时减少边缘的伪迹。

## 程序运行说明

程序运行需要的文件有：

+ assimp.dll FreeImage.dll glfw3.dll
+ model\dragon文件夹中所有文件，黑龙模型和纹理素材。
+ model\table_mountain_1_2k.hdr 背景的HDRI图片。
+ shaders文件夹中的着色器代码。
