# Hello 🔥
该仓库用作 "UNIX 环境高级编程" 的课程大作业, 有两个 Task, 一个是编写 disk simulator, 另一个是实现 filesystem 的部分功能. 后续有时间可以扩展功能, 我主要参考 Youtube 上 Dr Jonas Birch 关于文件系统的项目: [Filesystem from scratch](https://www.youtube.com/watch?v=d8flm9qT5O0).

![](.img/zelda.png)

# TODO
计划内容, 这里列出不完全版本, 后续更新:
- ✅ `dattach`, 初始化 disk 结构体, 获取 low level 文件描述符
- ✅ `ddetach`, 释放 disk 资源
- ✅ `dshow`, 打印 disk 信息
- ✅ `dread`, 读取一个 block
- ✅ `dwrite`, 写入一个 block
- ✅ `dreads`, 读取一组 blocks
- ✅ `dwrites`, 写入一组 blocks
- ✅ `bm_getbit`, 获取某位
- ✅ `bm_setbit`, 设置某位
- ✅ `bm_unsetbit`, 取消某位
- ✅ `bm_clearmap`, 清理整个 bitmap
- ✅ `get_inode_per_block`, 获取每个 block 能容纳的 inode 节点数量
- ✅ `bl_create`, 创建一个 block
- ✅ `bl_set_data`, 设置 block 的 data 字段
- ✅ `bl_get_data`, 获取 block 的 data 字段
- ✅ `bl_alloc`, 查阅并更新 block bitmap, 分配一个可用的 block number (置为 1 表示已占用)
- ✅ `bl_free`, 查阅并更新 block bitmap, 释放一个 block number (置为 0 表示未占用)
- ✅ `fs_format`, 用 fs 中定义的一些常量初始化 disk (操作磁盘文件)
- ✅ `fs_mount`, 读取 disk 文件信息, 初始化 filesystem 结构体
- ✅ `fs_unmount`, 释放 filesystem 结构体
- ✅ `fs_show`, 打印 filesystem 结构体信息
- ✅ `ino_init`, 清零一个 inode
- ✅ `ino_alloc`, 查阅并更新 inode bitmap, 分配一个可用的 inode number (置为 1 表示已占用)
- ✅ `ino_free`, 查阅并更新 inode bitmap, 释放一个可用的 inode number (置为 0 表示未占用)
- ✅ `ino_read`, 用 inode number 从磁盘读取一个 inode 信息
- ✅ `ino_write`, 向磁盘写入一个 inode 信息到指定 inode number
- `ino_alloc_block`
- `ino_get_block`
- `ino_free_block`
- `ino_free_all_blocks`
- `ino_show`
- `ino_is_valid`
- `ino_get_block_count`


# Task1
- 目标: 完成用户态环境下的磁盘模拟功能, 提供磁盘基本信息查询与格式化功能。
- 实现约束：利用一个大文件来模拟磁盘块设备，基于固定分片大小实现文件系统的超级块区、inode节点区、数据分片区的管理，具备磁盘格式化、文件系统查询（如：fdisk k - l）功能，支持文件的inode节点和对应数据分片的分配、回收。
- 提交内容：可编译的源码，可在linux环境中运行的程序；设计文档（包括模块框架设计，重要流程图），核心数据结构，运行时截图（操作相关结果）。

# Task2
- 目标：完整文件系统命令行操作接口，并形成命令任务下发，与作业1的文件系统存储引擎集成，构建完整的模拟文件系统。
- 形式要求：基于C/C++，提供上述接口，并编译为一个可执行程序。
- 实现约束：
  1）需支持多线程环境，采用单生产者多消费者模型；完成ls，cat，rm，copy几个典型命令，持续读写压力测试，运行时间不低于12小时，读写文件不低于50个，观察CPU，内存，磁盘开销，并将压测分析输出到报告中。
- 提交内容：可编译的源码（存储引擎和测试程序源代码），可执行文件和SO库文件；设计文档（包括模块框架设计，重要流程图），核心数据结构，运行时截图（操作相关结果）。
