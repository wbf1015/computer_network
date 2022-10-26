可能会遇到的问题及解决方法：

1、C4996报错，总之就是让你去把函数什么的换成最新的：插入一下代码就可以

```c++
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
```

2、无法打开源文件"xxxxxx.h",反正就是各种预编译的文件找不到。比如最经典的：无法打开源文件"stdafx.h"

解决方案：在解决方案里右键.cpp，选择属性----》C/C++-----》常规----》附加包含目录-------》添加$(ProjectDir)即可，点击应用、确定

然后删掉引用的预编译头文件。



网上靠谱的实现socket多线程的代码汇总：

19级物联网学长李世阳的 缺点：没有实现传递发送日期的功能

[(90条消息) C++ SOCKET多线程编程实现聊天小程序_NKU丨阳的博客-CSDN博客](https://blog.csdn.net/NKU_Yang/article/details/109455048)

他的github：https://github.com/NKU-Yang/

[(90条消息) c++ socket 多线程 网络聊天室_我不是萧海哇~~~~的博客-CSDN博客_c++ socket 多线程](https://blog.csdn.net/qq_45662588/article/details/116267585)

[(90条消息) C++语言实现网络聊天程序（基于TCP/IP协议的SOCKET编程）超详细（代码+解析）_爱编程的小云的博客-CSDN博客_c++聊天程序](https://blog.csdn.net/m0_48660921/article/details/122382490)



