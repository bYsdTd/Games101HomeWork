# 完成的点
- 格式正确，代码正确编译，执行
- 参数插值实现，修改rasterize_triangle函数，用重心坐标分别计算了插值后的颜色，法线和uv，还有viewspace的坐标
- 实现Blinn-phong反射模型, phong_fragment_shader函数，用光照模型公式进行着色的计算
- 实现Texture mapping，texture_fragment_shader函数，kd参数改为从纹理获取的插值后的颜色
- 实现Bump mapping和Displacement mapping， bump_fragment_shader函数和displacement_fragment_shader函数
- 加载其他模型 bunny，输入图bunny
- 实现双线性插值进行纹理采样, 输出图cube_point点采样，cube_bilinear双线性采样