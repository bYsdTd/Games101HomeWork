
- 提交格式正确
- 包围盒求交
- BVH查找
- SAH查找

SAH算法
遍历x，y，z轴，分别执行下面的计算
计算出轴向的最大宽度，然后分成B个桶，按照桶排序，把对应的object放入不同的桶。
遍历B-1种分桶的方法，找出C值最小的分割方法，C值的计算方法如下：
C = boundsA.surfaceArea/totalBounds.surfaceArea * objCountA + boundsB.surfaceArea/totalBounds.surfaceArea * objCountB
按照分割方法，把object分成left和right