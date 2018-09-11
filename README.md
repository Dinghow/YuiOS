# YuiOS项目文档

## 一. 项目概述

1. 我们的项目基于Orange's的样例代码，主要参考资料为《Orange'S 一个操作系统的实现》，在上面进行了添加，删除和修改，同时参考了部分往届学长的代码
2. 项目的命令行系统参考自ubuntu，我们对其进行了模仿
3. 在样例代码的基础上，我们主要实现的功能有：多用户登录与权限管理、多级文件系统、命令行历史及补全功能、两个用户级应用（井字棋、扫雷）

## 二. 系统工作流程

从软盘引导—>在软盘中查找`Loader.bin`—>加载`loader.bin`—>跳转`loader.bin`中的代码开始执  —> 在软盘中查找系统内核`kernel.bin`—>进保护模式 —>加载`kernel.bin`—>跳转`kernel.bin`中的代码开始执  —>更新`GDT` —> 初始化`IDT`—> 初始化`TSS`—>跳入系统主函数 —> 启动系统进程 —>开启时钟中断 —>开始进程调度 —>(系统开始运转)。

## 三. 开发成果综述

### 1. 用户登录及权限：

- 普通用户登录：启动系统，开机动画显示过后，会要求进行用户登录，我们设置了两个普通用户，分别为（dinghow,boo），账号密码相同，使用普通用户登录：

![](https://github.com/Dinghow/YuiOS/raw/master/img/3.png)

- 管理员登录及操作：输入`sudo`指令可获取管理员权限（管理员密码默认为admin)，管理员可以修改普通用户信息

- 用户切换：输入`su`指令可以实现用户切换

![](https://github.com/Dinghow/YuiOS/raw/master/img/4.png)

### 2. 文件系统：

我们将Orange‘S原本的扁平化文件系统改为了用户分区、多级的文件系统，不同用户拥有不同分区，且只能访问自己的分区，在各自分区下的文件系统可以创建文件夹

- 新建文件：输入`mkdir`指令新建文件，格式为`mkdir [filename] [content]`
- 写文件：输入`wt`或`wt+`编辑文件，`wt`指令是直接覆盖原内容，`wt+`可以添加至原内容后
- 删除文件：输入`del`指令删除文件，格式为`del [filename]`
- 新建文件夹：输入`mkdir`指令新建文件夹，文件夹名会自动加上*以做区分
- 切换目录：输入`cd`指令切换目录，格式为`cd [foldername]`，可以进入对应文件夹，使用`cd ..`指令可以返回上一级目录
- 删除文件夹：使用`del`指令删除文件夹
- 查看所有文件：使用`ls`指令查看当前目录下所有文件

![](https://github.com/Dinghow/YuiOS/raw/master/img/6.png)

### 3. 进程管理：

* 显示正在运行的所有进程

  ![1536599150281](https://github.com/Dinghow/YuiOS/raw/master/img/1536599150281.png)

* 对用户进程的操作（终止进程，暂停进程，唤醒进程）（这里将进程分为系统进程【系统进程不可操作】和用户进程【用户进程可以操作】）

  ![1536599217929](assets/1536599217929.png)![1536599251821](https://github.com/Dinghow/YuiOS/raw/master/img/1536599251821.png)

  ![1536599292373](https://github.com/Dinghow/YuiOS/raw/master/img/1536599292373.png)

### 4. 应用开发：

#### 4.1 游戏：井字棋

**游戏规则**：启动游戏井字棋后，首先显示出棋盘界面，然后用户可以输入坐标命令`x y`(x,y范围为1到3,用空格隔开)落子。每一次玩家落子后，电脑AI会跟一颗子，直到有一方获胜或棋盘下满。在游戏过程中玩家可以通过输入`q`来退出游戏。

![1536599583363](https://github.com/Dinghow/YuiOS/raw/master/img/1536599583363.png)

![   ](https://github.com/Dinghow/YuiOS/raw/master/img/2.png)

#### 4.2 游戏：扫雷

**游戏规则**：启动游戏扫雷后，首先会展示出当前的游戏界面，然后用户可以输入坐标命令`x y`(x,y范围为1到9,用空格隔开)来揭开该坐标及其周围无雷的区域。若该坐标有雷，则游戏结束，若玩家最终揭开全部无雷区域，则玩家获胜。在游戏过程中玩家可以通过输入`q`来退出游戏。

![](https://github.com/Dinghow/YuiOS/raw/master/img/1.png)



### 5. Linux命令行功能：

* 上下键[↑，↓]：切换上一条执行过的或下一条指令。

  ![上下](https://github.com/Dinghow/YuiOS/raw/master/img/上下.gif)

* Tab键：“操作”或“文件名”智能补全，允许用户输入当前目录下的部分文件名，自动匹配到唯一文件并补全命令行。 如：ki+[tab] => kill， vim f+[tab] => vim file

![tab](https://github.com/Dinghow/YuiOS/raw/master/img/tab.gif)

## 四. 操作指南

用户登录过后输入`help`指令可以查看所有命令及用法：



| 操作         | 指南                                          |
| :----------- | :-------------------------------------------- |
| help | 显示帮助窗口 |
| sudo | 获得管理员权限 |
| adduser \[username][password] | 添加用户username，并设置其账户密码 |
| deluser \[username][password] | 输入username及对应的密码来删除用户 |
| su \[username] [password] | 输入username及对应的密码来切换至对应的用户 |
| ls | 显示文件列表 |
| mkdir \[foldername] | 创建文件夹foldername |
| cd \[foldername] | 打开文件夹foldername |
| rd \[filename] | 读取文件filename |
| mkfile \[filename] [content] | 创建文件filename，并为其添加内容content |
| wt+ \[filename] [content] | 编辑文件filename，并为其添加内容content |
| wt \[filename] [content] | 编辑文件filename，并用content覆盖原内容 |
| del \[filename] | 删除文件filename |
| proc         | 显示正在内存运行的进程表，类似于任务管理器    |
| kill [pid]   | 如：kill 8  表示结束pid值为8的进程            |
| pause [pid]  | 如：pause 8 表示暂停pid值为8的进程            |
| resume [pid] | 如：resume8 表示恢复运行pid值为8的进程        |
| tictactoe    | 启动游戏井字棋                                |
| boom         | 启动游戏扫雷                                  |
| 上下键[↑，↓] | 同标准Linux一样切换上一条执行过的或下一条指令 |
| Tab键 | 同标准LInux一样对“操作”，“文件名”智能补全 |



## 五. 环境说明

### 1. 开发环境

- 平台： ubuntu 14.04.5 (32bit)
- 语言： C语言、汇编语言
- 工具：gedit, gcc, make,bochs 2.6.8



### 2. 运行环境

 bochs 2.6.8