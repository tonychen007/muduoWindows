# muduoWindows
porting muduo to Windows

Origin code is by Chenshuo https://github.com/chenshuo/muduo

Already existed version is  from https://github.com/kevin-gjm/muduo-win/


Some pits:
1. Windows has no fcntl to set CLOEXEC, and use SetHandleInformation instead
2. Current using select to multiplexing. It is better to use IOCP, and have to replace recv, send with WSARecv. Maybe do it later
3. Windows has timerfd, using WM_TIMER seems no work. So, timerQueue is rewritten by Kevin
