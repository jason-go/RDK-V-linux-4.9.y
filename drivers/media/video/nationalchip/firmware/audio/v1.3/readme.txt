* v1.2音频固件支持芯片：
** 3211
** gx6605s
** sirius
** taurus
* 增加mpeg.mini固件,较少flash花销，只支持mpeg12a/aac + ad decode, dts bypass, eac3/ac3 bypass
* all.bin为加密固件，all.fw.bin为非加密固件
* sirius的All固件比较特殊
** 需要编译前反大小端
** irdeto和vmxplus的加密Key不一样,所以得两个固件
** all.sirius.irdeto.bin 用于 irdeto
** all.sirius.vmxplus.bin 用于 vmx+
