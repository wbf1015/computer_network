使用规则：

假定：client端发送图片 server端接收图片；本来最简单的是client发送给server，server回ACK给client。

但是现在要使用路由程序，那么则：client发送给路由程序，路由程序给服务端，同理服务端给路由程序，路由程序给客户端。所以说在代码中：client和server都发送到路由段，同理，接收端也接收路由的信息。

看一看测试程序就能懂是怎么用的了

测试程序结果：

![](G:\code\computerNetwork\大作业\routeruse\pic\1.png)