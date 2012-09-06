Name
====

yifei -- a project named with the name of my daughter

功能包括：
1，unix system 系统调用等薄包装
2，类似libevent的一个跨linux/solars/FreeBSD/SunOS/Darwin等
	 unix system平台的一个异步事务驱动器，包括事件类型：
	 	i)	fd 句柄
	 	ii)	signal 信号
	 	iii)timer 定时器
	 	ix)	proc 子进程调用
	 	
3，独创的服务器各类运行实例通信交互模式工厂，已实现模式有：
		i)	父进程(非阻塞/异步) -> 子进程(非阻塞/异步)
		ii) 父线程(非阻塞/异步) -> 子线程(非阻塞/异步)
		iii)父线程(非阻塞/异步) -> 子线程(阻塞/同步)
		
Usage
====
此项目是一个工具库，可用于unix后台异步server的搭建

具体使用方法见 test/ 目录中的单元测试程序，主要有两个：
yf_driver_testor.cpp
yf_bridge_testor.cpp


Author
======

name: kevin_zhong
mail: qq2000zhong@gmail.com


