# BFJetHotKey

本项目是一个为了解决战地无法将固定翼俯仰偏航绑定在鼠标按键上的曲线救国方案

## 按键映射表格
| 映射鼠标按键 | 虚拟码 | 机动 | 被映射小键盘按键 |
| :--- | :--- | :--- | :--- |
| 鼠标左键 (Left Button) | VK_LBUTTON | 左翻滚 | Numpad 4 |
| 鼠标右键 (Right Button) | VK_RBUTTON | 右翻滚 | Numpad 6 |
| 鼠标侧键 1 (XButton1) | VK_XBUTTON1 | 俯 | Numpad 8 |
| 鼠标侧键 2 (XButton2) | VK_XBUTTON2 | 仰 | Numpad 5 |

## 使用说明
如需修改映射请自行修改源码

### 编译命令
```
g++ main.cpp app.o -o BFJetKeyMapping.exe -lgdi32 -lshell32 -mwindows
```
