# 130 × 60 × 70 mm 可打印外壳

这是参照用户提供的圆角矩形设备照片制作的两件式外壳。顶部保持完整平面；前壳正面左侧为电子纸显示窗，右下为 35 mm 按键区，中间设有内部隔板，后盖可拆下以放入元件。

## 文件

- [`build_enclosure.py`](build_enclosure.py)：FreeCAD 参数化构建脚本。
- [`output/enclosure_130x60x70.FCStd`](output/enclosure_130x60x70.FCStd)：可编辑 FreeCAD 模型，包含 `Parameters (mm)` 参数表。
- [`output/enclosure_130x60x70.step`](output/enclosure_130x60x70.step)：前壳和后盖的装配 STEP。
- [`output/front_shell_print.stl`](output/front_shell_print.stl)：前壳打印方向 STL。
- [`output/back_cover_print.stl`](output/back_cover_print.stl)：后盖打印方向 STL。
- [`output/validation.json`](output/validation.json)：几何和网格检查结果。

## 结构和尺寸

- 装配包络：`130 × 60 × 70 mm`（长 × 深 × 高）。
- 壁厚：4 mm；外侧圆角半径 8 mm。
- 左侧显示窗：`84.8 × 63.6 mm`，对应 QYEG0420BNS830 的显示区域，圆角半径 5.5 mm。
- 右下按键窗：`35 × 35 mm`，按用户实测的 8 Push Buttons V1.02 最大外尺寸建立。
- 显示区和按键区之间是 2.2 mm 的内部隔板；按键窗右侧保留 4 mm 外壁。
- QYEG0420BNS830 商品页给出的整屏外形为 `91 × 77 × 1.2 mm`。由于外壳高度固定为 70 mm，模型把整屏作为隐藏参考件放入 FCStd，居中时上下各超出约 3.5 mm；装配时需要让屏幕外置于前面板，或后续把外壳高度改为至少 85 mm。
- 前壳从后方敞开；后盖厚 4 mm，带 2 mm 定位唇和 0.4 mm 单边配合余量。
- 后盖使用 4 个 M3 通孔，孔径 3.4 mm；螺钉轴向沿外壳深度方向，建议使用 4 颗 M3×12 mm 自攻螺钉或根据实际紧固方案加热熔铜螺母。
- 当前没有加入 USB、天线、音频、排针或电源开孔，因为尚未提供元件在壳体内的精确位置。脚本中的参数可继续扩展这些开孔。

## 打印和装配

1. 前壳使用 `front_shell_print.stl`，大正面朝打印平台，模型已将最低 Z 对齐到 0。
2. 后盖使用 `back_cover_print.stl` 平放打印，定位唇朝上或按切片软件需要翻面。
3. 建议 0.2 mm 层高、4 道外壁、25% 左右填充；圆角外壁和后盖均不需要支撑。实际参数按打印机和材料调整。
4. 将元件从后方放入前壳，整理线缆后放入后盖，对准定位唇和四个孔位，再锁紧 M3 螺钉。
5. 这是通用容纳壳，打印前应在 FreeCAD 中按真实 PCB、屏幕和接口位置补充开孔，并用实物确认插拔、卡扣和温升。

## 重新生成

在项目根目录执行：

```sh
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c "import runpy; runpy.run_path('packages/hardware/enclosure/build_enclosure.py', run_name='__main__')"
```

修改脚本顶部的 `P` 参数后重新运行，会更新 FCStd、STEP、STL 和 `validation.json`。
